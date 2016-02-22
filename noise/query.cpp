//
//  query.cpp
//  noise
//
//  Created by Damien Katz on 12/27/15.
//  Copyright © 2015 Damien Katz. All rights reserved.
//

#include <iostream>
#include <stack>
#include <string>
#include <set>
#include <list>
#include <memory>
#include <sstream>
#include "records.pb.h"
#include <rocksdb/db.h>
#include "query.hpp"
#include "results.hpp"
#include "stemmed_key.h"
#include "porter.h"

namespace_Noise

using std::unique_ptr;

class ASTLiteral;

static const std::string emptystr("");

/*
 'sometext and 23hit'

 sometext
 uint64 stemmedOffset 0;
 int64 suffixOffset  8
 string suffixText;  " "

 and
 uint64 stemmedOffset 9;
 int64 suffixOffset  12
 string suffixText;  " "

 hit
 uint64 stemmedOffset 15;
 int64 suffixOffset  13
 string suffixText;  " "

 */



class ExactWordMatchFilter : public QueryRuntimeFilter {
    size_t stemmed_offset_;
    std::string suffix_;
    size_t suffix_offset_;

    StemmedKeyBuilder key_build_;

    unique_ptr<rocksdb::Iterator> iter_;
public:
    ExactWordMatchFilter(unique_ptr<rocksdb::Iterator>& iter,
                         StemmedWord& stemmed_word,
                         StemmedKeyBuilder& key_build)
        : iter_(std::move(iter)), stemmed_offset_(stemmed_word.stemmed_offset),
          suffix_(stemmed_word.suffix, stemmed_word.suffix_len),
          key_build_(key_build), suffix_offset_(stemmed_word.suffix_offset)
    {
        key_build_.PushWord(stemmed_word.stemmed,
                                 stemmed_word.stemmed_len);
    }

    ExactWordMatchFilter& operator=(ExactWordMatchFilter const&) = delete;

    virtual unique_ptr<DocResult> FirstResult(uint64_t startId) {
        // build full key
        key_build_.PushDocSeq(startId);

        // seek in index to GTE entry
        iter_->Seek(key_build_.Key());

        //revert
        key_build_.Pop(StemmedKeyBuilder::DocSeq);

        return NextResult();
    }

    virtual unique_ptr<DocResult> NextResult() {

        unique_ptr<DocResult> dr(new DocResult);

        while(iter_->Valid()) {
            // all out of stuff to return. This (maybe?) can never be invalid
            // except for io error.

            rocksdb::Slice key = iter_->key(); // start off as full path

            if (!key.starts_with(key_build_.Key()))
                // prefix no longer matches. all out of stuff to return.
                return nullptr;

            // remove prefix from id as it should be
            key.remove_prefix(key_build_.Key().size());

            std::string data = iter_->value().ToString();

            std::cout << "data:" << data << " len: " << data.length() << "\n";

            records::payload payload;
            bool b = payload.ParseFromArray(data.c_str(),
                                   (int)data.length());

            for (auto aos_wis : payload.arrayoffsets_to_wordinfos()) {
                for (auto wi : aos_wis.wordinfos()) {
                    std::cout << "stemmedoffset: " << wi.stemmedoffset() <<
                                " suffixoffset: " << wi.suffixoffset() <<
                                " suffixtext: " << wi.suffixtext() <<"\n";
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
                // record the doc id before we return
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

    virtual std::unique_ptr<DocResult> FirstResult(uint64_t advance) {
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
    SnapshopIteratorCreator(rocksdb::DB* db)
        : db_(db)
    {
        options_.snapshot = db_->GetSnapshot();
    }
    ~SnapshopIteratorCreator() {
        db_->ReleaseSnapshot(options_.snapshot);
    }

    std::unique_ptr<rocksdb::Iterator> NewIterator() {
        return std::unique_ptr<rocksdb::Iterator>(db_->NewIterator(options_));
    }

};

std::unique_ptr<QueryRuntimeFilter> WalkAST(SnapshopIteratorCreator& ic,
                                            StemmedKeyBuilder& key,
                                            ASTNode* node,
                                            size_t array_depth) {
    switch (node->type) {
        case ASTNode::EQUALS: {
            assert(node->children.size() == 2);
            ASTNode* fieldname;
            ASTNode* textliteral;
            if (node->children[0]->type == ASTNode::FIELD) {
                fieldname = node->children[0].get();
                textliteral = node->children[1].get();
            } else {
                fieldname = node->children[1].get();
                textliteral = node->children[0].get();
            }
            Stems stems(textliteral->value);

            unique_ptr<std::vector<unique_ptr<QueryRuntimeFilter> > > filters(
                new std::vector<unique_ptr<QueryRuntimeFilter> >);
            key.PushObjectKey(fieldname->value);
            while (stems.HasMore()) {
                StemmedWord stem = stems.Next();
                std::unique_ptr<rocksdb::Iterator> itor(ic.NewIterator());
                filters->emplace_back(new ExactWordMatchFilter(itor, stem, key));
            }
            key.Pop(StemmedKeyBuilder::ObjectKey);
            if (filters->size() == 1)
                return std::move(filters->front());
            else
                return unique_ptr<QueryRuntimeFilter>(new AndFilter(filters,
                                                                 array_depth));

        }

        case ASTNode::AND: {
            unique_ptr<std::vector<unique_ptr<QueryRuntimeFilter> > > filters(
                new std::vector<unique_ptr<QueryRuntimeFilter> >);
            for(auto& child : node->children) {
                filters->push_back(WalkAST(ic, key, child.get(), array_depth));
            }
            return unique_ptr<QueryRuntimeFilter>(new AndFilter(filters,
                                                                array_depth));
        }

        case ASTNode::ARRAY: {
            key.PushObjectKey(node->value);
            key.PushArray();
            unique_ptr<QueryRuntimeFilter> ret(WalkAST(ic,
                                                         key,
                                                         node->children[0].get(),
                                                         array_depth + 1));
            key.Pop(StemmedKeyBuilder::Array);
            key.Pop(StemmedKeyBuilder::ObjectKey);
            return ret;
        }
        case ASTNode::FIELD:
        case ASTNode::LITERAL:
        case ASTNode::UNKNOWN:
        default:
            assert(false);
            return nullptr; //placate compiler
    }

        
}

std::unique_ptr<ASTNode> BuildTree(std::string& tokenstr);


Query::Query(std::string& str) : root(BuildTree(str)) {

}

std::unique_ptr<Results> Query::Execute(Index* index) {
    SnapshopIteratorCreator ic(index->GetDB());
    StemmedKeyBuilder keybuild;
    unique_ptr<QueryRuntimeFilter> filter(WalkAST(ic, keybuild, root.get(), 0));

    return unique_ptr<Results> (new Results(filter));
}

/* this is placehilder code until we have a parser. */

std::unique_ptr<ASTNode> BuildTree(std::string& tokenstr) {
    std::stringstream tokens(tokenstr);
    std::string token;
    std::stack<std::unique_ptr<ASTNode> > stack;
    std::unique_ptr<ASTNode> node;
    while (std::getline(tokens, token, ' ')) {
        node = std::unique_ptr<ASTNode>(new ASTNode);
        node->value = token;
        char c = token.front();
        switch (c) {
            case 'F':
                assert(token == "FIELD");
                node->type = ASTNode::FIELD;
                node->value = stack.top()->value;
                stack.pop();
                break;

            case 'E':
                assert(token == "EQUALS");
                node->type = ASTNode::EQUALS;
                node->children.push_back(std::move(stack.top()));
                stack.pop();
                node->children.push_back(std::move(stack.top()));
                stack.pop();
                break;

            case 'A':
                if (token == "AND") {
                    node->type = ASTNode::AND;
                    node->children.push_back(std::move(stack.top()));
                    stack.pop();
                    node->children.push_back(std::move(stack.top()));
                    stack.pop();

                } else if (token == "ARRAY") {
                    node->type = ASTNode::ARRAY;
                    node->value = stack.top()->value;
                    stack.pop();
                    node->children.push_back(std::move(stack.top()));
                    stack.pop();
                } else {
                    assert(false);
                    abort();
                }
                break;

            case '"':
                assert(token.back() == '"' && token.size() > 1);
                node->type = ASTNode::LITERAL;
                // NOTE: does NOT unescape the string, just strips off front and end quote
                node->value = {token.begin()+1, token.end()-1};
                break;
                
            case '\n':
            case '/':
                // comment line, skip
                std::getline(tokens, token);
                continue;
            default:
                assert(false);

        }
        stack.push(std::move(node));
    }
    assert(stack.size() == 1);
    return std::move(stack.top());
}












namespace_Noise_end
