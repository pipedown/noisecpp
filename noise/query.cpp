//
//  query.cpp
//  noise
//
//  Created by Damien Katz on 12/27/15.
//  Copyright © 2015 Damien Katz. All rights reserved.
//

#include <stack>
#include <set>
#include <list>
#include <memory>
#include <sstream>
#include "records.pb.h"
#include <rocksdb/db.h>
#include "query.hpp"

namespace_Noise

using std::unique_ptr;

class ASTLiteral;

static const std::string emptystr("");


class ASTNode {
protected:
    std::string name;
public:
    virtual ~ASTNode() {}
    virtual void SetName(std::string& _name) {name = _name;}
    virtual void Initialize(std::stack<std::unique_ptr<ASTNode> >&) {
        //by default do nothing
    }
    virtual bool IsField() {return false;}
    const std::string& Literal() const {
        return name;
    }
};

class ASTField : public ASTNode {
public:

    virtual void Initialize(std::stack<std::unique_ptr<ASTNode> >& stack) {
        name = stack.top()->Literal();
        stack.pop();
    }
    virtual bool IsField() {return true;}
    virtual const std::string& Literal() const {
        return name;
    }
};


class ASTEquals : public ASTNode {
    std::list<std::unique_ptr<ASTNode> > children;
public:
    virtual void Initialize(std::stack<std::unique_ptr<ASTNode> >& stack) {
        children.push_back(std::move(stack.top()));
        stack.pop();
        children.push_back(std::move(stack.top()));
        stack.pop();
    }
};

class ASTAnd : public ASTNode {
    std::list<std::unique_ptr<ASTNode> > children;
public:
    virtual void Initialize(std::stack<std::unique_ptr<ASTNode> >& stack) {
        children.push_back(std::move(stack.top()));
        stack.pop();
        children.push_back(std::move(stack.top()));
        stack.pop();
    }
};

class ASTArray : public ASTNode {
    std::unique_ptr<ASTNode> child;
public:
    virtual void Initialize(std::stack<std::unique_ptr<ASTNode> >& stack) {
        name = stack.top()->Literal();
        stack.pop();
        child = std::move(stack.top());
        stack.pop();
    }
};


class ASTLiteral : public ASTNode {
public:
    const std::string value;
    // NOTE: does NOT unescape the string, just strips off front and end quote
    ASTLiteral(std::string& Quoted) : value(Quoted.begin()+1, Quoted.end()-1) {
    }
    virtual bool IsLiteral() {
        return true;
    }
};


std::unique_ptr<ASTNode> BuildTree(std::istream& tokens) {
    std::string token;
    std::stack<std::unique_ptr<ASTNode> > stack;
    std::unique_ptr<ASTNode> node;
    while (std::getline(tokens, token, ' ')) {
        switch (token.front()) {
            case 'F':
                assert(token == "FIELD");
                node = std::unique_ptr<ASTNode>(new ASTField);
                break;

            case 'E':
                assert(token == "EQUALS");
                node = std::unique_ptr<ASTNode>(new ASTEquals);
                break;

            case 'A':
                if (token == "AND")
                    node = std::unique_ptr<ASTNode>(new ASTAnd);
                else if (token == "ARRAY")
                    node = std::unique_ptr<ASTNode>(new ASTArray);
                else
                    assert(false);
                break;

            case '"':
                assert(token.back() == '"' && token.size() > 1);
                node = std::unique_ptr<ASTNode>(new ASTLiteral(token));
                break;

            case '/':
                // comment line, skip
                std::getline(tokens, token);
                continue;
            default:
                assert(false);

        }
        node->SetName(token);
        node->Initialize(stack);
        stack.push(std::move(node));
    }
    assert(stack.size() == 1);
    return std::move(stack.top());
}



struct DocResult {
    std::string id;
    std::list<std::vector<uint64_t> > array_paths;

    void TruncateArrayPaths(size_t array_path_depth) {
        for(auto ap : array_paths)
            ap.resize(array_path_depth);
    }
    bool IntersectArrayPaths(const DocResult& other) {
        for (auto ap = array_paths.begin(); ap != array_paths.end(); ap++) {
            bool found = false;
            for (auto ap_other : array_paths) {
                if (*ap == ap_other) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                auto del = ap++;
                array_paths.remove(*del);
            }
        }
        return array_paths.size() > 0;
    }
};





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

    size_t prefix_len;
    uint64_t stemmed_offset;
    std::string stemmed;
    int64_t suffix_offset;
    std::string suffix_text;
    std::string path;

    std::string key_prefix;
    rocksdb::Iterator& iter;
public:
    ExactWordMatchFilter(rocksdb::Iterator& _iter,
                         std::string _path,
                         uint64_t _stemmed_offset,
                         std::string _stemmed,
                         int64_t _suffix_offset,
                         std::string _suffix_text)
    : iter(_iter), path(_path), stemmed_offset(_stemmed_offset),
        stemmed(_stemmed), suffix_offset(_suffix_offset),
        suffix_text(_suffix_text)
    {
        key_prefix.reserve(path.length() + stemmed.length() + 10);
        key_prefix += "D" + path + string("!") + stemmed + "#";
    }

    virtual unique_ptr<DocResult> FirstResult(std::string& startId) {
        // build full key
        key_prefix += startId;

        // seek in index to GTE entry
        iter.Seek(key_prefix);

        //revert buffer to prefix
        key_prefix.resize(prefix_len);


        return NextResult();
    }

    virtual unique_ptr<DocResult> NextResult() {

        unique_ptr<DocResult> dr(new DocResult);

        while(iter.Valid()) {
            // all out of stuff to return. This (maybe?) can never be invalid
            // except for io error.

            rocksdb::Slice id = iter.key(); // start off as full path

            if (!id.starts_with(key_prefix))
                // prfix no longer matches. all out of stuff to return.
                return nullptr;

            // remove prefix from id as it should be
            id.remove_prefix(prefix_len);

            records::payload payload;
            payload.ParseFromArray(iter.value().data(),
                                   (int)iter.value().size());

            for (auto& aos_wis : payload.arrayoffsets_to_wordinfos()) {
                for (auto& wi : aos_wis.wordinfos()) {
                    if (stemmed_offset == wi.stemmedoffset() &&
                        suffix_offset == wi.suffixoffset() &&
                        suffix_text == wi.suffixtext()) {
                        // we have a candidate document to return
                        auto arrayoffsets = aos_wis.arrayoffsets();
                        dr->array_paths.emplace_back(arrayoffsets.begin(), arrayoffsets.end());
                    }
                }
            }

            if (dr->array_paths.size() != 0) {
                // record the doc id before we return
                dr->id.replace(0, id.size(), id.data());
                return dr;
            }
            iter.Next();
        }
        
        return nullptr;

    }

};

class SentenceMatchFilter : public QueryRuntimeFilter {
    SentenceMatchFilter(unique_ptr<std::list<unique_ptr<QueryRuntimeFilter> > >);
};


class AndFilter : public QueryRuntimeFilter {
    unique_ptr<std::list<QueryRuntimeFilter> > filters;
    std::list<QueryRuntimeFilter>::iterator filter;
    int match_array_depth;
public:
    AndFilter(unique_ptr<std::list<QueryRuntimeFilter> >& _filters,
              int _MatchArrayDepth) : filter(_filters->begin()) {
        filters = std::move(_filters);
    }
    virtual std::unique_ptr<DocResult> FirstResult(std::string& advance) {
        auto base_result = filter->FirstResult(advance);
        return std::move(Result(base_result));
    }
    virtual unique_ptr<DocResult> NextResult() {
        auto base_result = filter->NextResult();
        return std::move(Result(base_result));
    }

    unique_ptr<DocResult> Result(unique_ptr<DocResult>& base_result) {
        size_t matchesNeededCount = filters->size() - 1;
        base_result->TruncateArrayPaths(match_array_depth);
        if (base_result == nullptr)
            return nullptr;
        while (true) {
            if (filter == filters->end())
                filter = filters->begin();
            else
                filter++;
            auto next_result = filter->FirstResult(base_result->id);
            if (next_result == nullptr)
                return nullptr;
            next_result->TruncateArrayPaths(match_array_depth);
            if (base_result->id == next_result->id) {
                // got a potential match. intersect paths
                if (base_result->IntersectArrayPaths(*next_result)) {
                    // intersection exists
                    if (--matchesNeededCount == 0)
                        // got all the matches needed, return result
                        return std::move(base_result);
                } else {
                    //no way this doc is a match. get next candidate doc
                    base_result = filter->NextResult();
                    matchesNeededCount = filters->size() - 1;
                }
            } else {
                // no way this doc is a match. we already have next cadidate doc
                base_result = std::move(next_result);
                matchesNeededCount = filters->size() - 1;
            }
        }
    }
};


namespace_Noise_end
