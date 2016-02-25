//
//  query.h
//  noise
//
//  Created by Damien Katz on 12/27/15.
//  Copyright © 2015 Damien Katz. All rights reserved.
//

#ifndef query_h
#define query_h


namespace_Noise

struct DocResult;
class Results;

struct ASTNode {
    enum Type
    {
        UNKNOWN,
        FIELD,
        EQUALS,
        AND,
        ARRAY,
        LITERAL,
    };
    Type type;
    std::string value;
    std::vector<unique_ptr<ASTNode> > children;

};

class QueryRuntimeFilter {
public:
    // returns the doc at startId, or next after
    virtual unique_ptr<DocResult> FirstResult(uint64_t startId) = 0;

    // returns the doc next after previous
    virtual unique_ptr<DocResult> NextResult() = 0;
};


class Query {
    unique_ptr<ASTNode> root;
public:
    Query(std::string& query);

    unique_ptr<Results> Execute(Index* index);
};

namespace_Noise_end

#endif /* query_h */
