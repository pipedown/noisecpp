//
//  query.cpp
//  noise
//
//  Created by Damien Katz on 12/27/15.
//  Copyright © 2015 Damien Katz. All rights reserved.
//



#include <sstream>
#include "proto/records.pb.h"
#include "noise.h"
#include "query.h"
#include "results.h"
#include "key_builder.h"
#include "stems.h"

namespace_Noise



class ExactWordMatchFilter : public QueryRuntimeFilter {
    unique_ptr<rocksdb::Iterator> iter_;
    KeyBuilder key_build_;
    uint64_t stemmed_offset_;
    std::string suffix_;
    uint64_t suffix_offset_;
public:
    ExactWordMatchFilter(unique_ptr<rocksdb::Iterator>& iter,
                         StemmedWord& stemmed_word,
                         KeyBuilder& key_build)
    : iter_(std::move(iter)),
    key_build_(key_build),
    stemmed_offset_(stemmed_word.stemmed_offset),
    suffix_(stemmed_word.suffix, stemmed_word.suffix_len),
    suffix_offset_(stemmed_word.suffix_offset)
    {
        key_build_.PushWord(stemmed_word.stemmed,
                            stemmed_word.stemmed_len);
    }

    ExactWordMatchFilter& operator=(ExactWordMatchFilter const&) = delete;

    virtual unique_ptr<DocResult> FirstResult(uint64_t startId) {
        // build full key
        key_build_.PushDocSeq(startId);

        // seek in index to GTE entry
        iter_->Seek(key_build_.key());

        //revert
        key_build_.PopDocSeq();

        return NextResult();
    }

    virtual unique_ptr<DocResult> NextResult() {

        unique_ptr<DocResult> dr(new DocResult);

        while(iter_->Valid()) {
            // all out of stuff to return. This (maybe?) can never be invalid
            // except for io error.

            rocksdb::Slice key = iter_->key(); // start off as full path

            if (!key.starts_with(key_build_.key()))
                // prefix no longer matches. all out of stuff to return.
                return nullptr;

            // remove prefix from key so we have the doc seq
            key.remove_prefix(key_build_.key().size());

            std::string data = iter_->value().ToString();

            records::payload payload;
            if(!payload.ParseFromArray(data.c_str(), (int)data.length()))
                throw std::runtime_error("Couldn't parse proto buf");

            for (auto aos_wis : payload.arrayoffsets_to_wordinfos()) {
                for (auto wi : aos_wis.wordinfos()) {
                    if (stemmed_offset_ == wi.stemmedoffset() &&
                        suffix_offset_ == wi.suffixoffset() &&
                        suffix_ == wi.suffixtext()) {
                        // we have a candidate document to return
                        auto& arrayoffsets = aos_wis.arrayoffsets();
                        dr->array_paths.emplace_back(arrayoffsets.begin(),
                                                     arrayoffsets.end());
                        char* end = (char*)key.data() + key.size();
                        dr->seq = std::strtoul(key.data(), &end, 10);
                        goto loopdone;
                    }
                }
            }
        loopdone:
            iter_->Next();

            if (dr->array_paths.size() != 0)
                return dr;
        }
        return nullptr;
    }

};

class SentenceMatchFilter : public QueryRuntimeFilter {
    SentenceMatchFilter(unique_ptr<std::list<unique_ptr<QueryRuntimeFilter> > >);
};


class AndFilter : public QueryRuntimeFilter {
    unique_ptr<std::vector<unique_ptr<QueryRuntimeFilter> > > filters_;
    std::vector<unique_ptr<QueryRuntimeFilter> >::iterator filter_;
    size_t match_array_depth_;
public:
    AndFilter(unique_ptr<std::vector<unique_ptr<QueryRuntimeFilter> > >& filters,
              size_t match_array_depth)
    : filters_(std::move(filters)), filter_(filters_->begin()),
    match_array_depth_(match_array_depth) {}

    virtual unique_ptr<DocResult> FirstResult(uint64_t advance) {
        auto base_result = filter_->get()->FirstResult(advance);
        return std::move(Result(base_result));
    }

    virtual unique_ptr<DocResult> NextResult() {
        auto base_result = filter_->get()->NextResult();
        return std::move(Result(base_result));
    }

    unique_ptr<DocResult> Result(unique_ptr<DocResult>& base_result) {
        size_t matchesNeededCount = filters_->size();
        while (true) {
            if (base_result == nullptr)
                return nullptr;
            base_result->TruncateArrayPaths(match_array_depth_);
            filter_++;
            if (filter_ == filters_->end())
                filter_ = filters_->begin();
            auto next_result = filter_->get()->FirstResult(base_result->seq);
            if (next_result == nullptr)
                return nullptr;
            next_result->TruncateArrayPaths(match_array_depth_);
            if (base_result->seq == next_result->seq) {
                // got a potential match. intersect paths
                if (base_result->IntersectArrayPaths(*next_result)) {
                    // intersection exists
                    if (--matchesNeededCount == 0)
                        // got all the matches needed, return result
                        return std::move(base_result);
                } else {
                    //no way this doc is a match. get next candidate doc
                    base_result = filter_->get()->NextResult();
                    matchesNeededCount = filters_->size();
                }
            } else {
                // no way this doc is a match. we already have next candidate
                base_result = std::move(next_result);
                matchesNeededCount = filters_->size();
            }
        }
    }
};



class SnapshopIteratorCreator {
    rocksdb::ReadOptions options_;
    rocksdb::DB* db_;
public:
    SnapshopIteratorCreator(rocksdb::DB* db) : db_(db) {
        options_.snapshot = db_->GetSnapshot();
    }
    ~SnapshopIteratorCreator() {
        db_->ReleaseSnapshot(options_.snapshot);
    }

    unique_ptr<rocksdb::Iterator> NewIterator() {
        return unique_ptr<rocksdb::Iterator>(db_->NewIterator(options_));
    }

};



/*

 Bool
 = Compare ("&&" Compare)*

 Compare
 = Field "==" Integer
 / Field "[" Bool "]"
 / Factor

 Factor
 = "(" Bool ")"
 / "[" Bool "]"

 Field
 = [a-z]+

 Integer
 = [0-9]+

 */


class Parser {
    const std::string& query_str_;
    const char* next_;
    KeyBuilder kb_;
    SnapshopIteratorCreator& ic_;

    class parse_exception : public std::runtime_error {
    public:
        parse_exception(const std::string& what) : std::runtime_error(what) {}
    };

public:
    Parser(const std::string& query_str, SnapshopIteratorCreator& ic)
    : query_str_(query_str), ic_(ic)
    {
        next_ = query_str_.c_str();
    }


    unique_ptr<QueryRuntimeFilter> BuildFilter(std::string* parse_error) {
        try {
            WS();
            return Bool();
        } catch (parse_exception& e) {
            *parse_error = e.what();
            return nullptr;
        }
    }

private:

    void WS() {
        while (*next_ == ' '  ||
               *next_ == '\n' ||
               *next_ == '\t' ||
               *next_ == '\r') {
            next_++;
        }
    }

    void Error(const std::string& what) {
        throw parse_exception(what);
    }

    bool Consume(const char* token) {
        const char* next = next_;
        while (*token != 0 && *token == *next) {
            token++; next++;
        }
        if (*token == 0) {
            next_ = next;
            WS();
            return true;
        }
        return false;
    }

    bool CouldConsume(const char* token) {
        const char* next = next_;
        if (Consume(token)) {
            next_ = next; // unconsume
            return true;
        } else {
            return false;
        }

    }

    bool ConsumeField(std::string* name) {
        bool result = false;
        while (isalpha(*next_)) {
            name->push_back(*next_++);
            result = true;
        }
        WS();
        return result;
    }

    bool ConsumeStringLiteral(std::string* lit) {
        // Does not unescape yet
        const char* next = next_;
        if (*next_ == '"') {
            next_++;
            while (*next_ != 0 && *next_ != '"') {
                lit->push_back(*next_++);
            }
            if (*next_ == '"') {
                next_++;
                WS();
                return true;
            } else {
                next_ = next;
                Error("Expected \"");
                return false;
            }
        }
        return false;
    }

    unique_ptr<QueryRuntimeFilter> Bool() {
        unique_ptr<QueryRuntimeFilter> left = Compare();
        unique_ptr<std::vector<unique_ptr<QueryRuntimeFilter> > > filters;
        while (Consume("&")) {
            if (!filters) {
                filters = unique_ptr<std::vector<unique_ptr<QueryRuntimeFilter> > >(
                                                                                    new std::vector<unique_ptr<QueryRuntimeFilter> >);
                filters->push_back(std::move(left));
            }
            filters->push_back(Compare());
        }
        if (filters)
            return unique_ptr<QueryRuntimeFilter>(new AndFilter(filters,
                                                                kb_.ArrayDepth()));
        else
            return left;
    }

    unique_ptr<QueryRuntimeFilter> Compare() {
        std::string field;
        if (ConsumeField(&field)) {
            if (Consume(".")) {
                kb_.PushObjectKey(field);
                unique_ptr<QueryRuntimeFilter> ret(Compare());
                kb_.PopObjectKey();
                return ret;
            } else if (Consume("=")) {
                std::string literal;
                if (!ConsumeStringLiteral(&literal))
                    Error("value");

                Stems stems(literal);
                unique_ptr<std::vector<unique_ptr<QueryRuntimeFilter> > > filters(
                                                                                  new std::vector<unique_ptr<QueryRuntimeFilter> >);

                kb_.PushObjectKey(field);
                while (stems.HasMore()) {
                    StemmedWord stem = stems.Next();
                    unique_ptr<rocksdb::Iterator> itor(ic_.NewIterator());
                    filters->emplace_back(new ExactWordMatchFilter(itor, stem, kb_));
                }
                kb_.PopObjectKey();
                if (filters->size() == 1)
                    return std::move(filters->front());
                else
                    return unique_ptr<QueryRuntimeFilter>(new AndFilter(filters,
                                                                        kb_.ArrayDepth()));
            } else if (CouldConsume("[")) {
                kb_.PushObjectKey(field);
                unique_ptr<QueryRuntimeFilter> ret(Array());
                kb_.PopObjectKey();
                return ret;
            }
            Error("Expected comparison or array operator");
        }
        return Factor();
    }

    unique_ptr<QueryRuntimeFilter> Array() {
        if (!Consume("["))
            Error("[");
        kb_.PushArray();
        unique_ptr<QueryRuntimeFilter> filter = Bool();
        kb_.PopArray();
        if (!Consume("]"))
            Error("]");
        return filter;
    }

    unique_ptr<QueryRuntimeFilter> Factor() {
        if (Consume("(")) {
            unique_ptr<QueryRuntimeFilter> filter = Bool();
            if (!Consume(")"))
                Error(")");
            return filter;
        } else if (CouldConsume("[")) {
            return Array();
        }
        Error("Missing expression");
        return nullptr;
    }
};



unique_ptr<Results> Query::GetMatches(const std::string& query,
                                      Index& index,
                                      std::string* parse_error) {
    SnapshopIteratorCreator ic(index.GetDB());
    Parser p(query, ic);
    unique_ptr<QueryRuntimeFilter> filter(p.BuildFilter(parse_error));
    if (filter)
        return unique_ptr<Results> (new Results(filter));
    else
        return nullptr;
}



namespace_Noise_end
