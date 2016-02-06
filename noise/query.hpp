//
//  query.hpp
//  noise
//
//  Created by Damien Katz on 12/27/15.
//  Copyright © 2015 Damien Katz. All rights reserved.
//

#ifndef query_hpp
#define query_hpp

#include <stdio.h>
#include "noise.h"

namespace rocksdb {
    class DB;
}

namespace_Noise

class DocResult;

class QueryRuntimeFilter {
public:
    // returns the doc at startId, or next after
    virtual std::unique_ptr<DocResult> FirstResult(std::string& startId);

    // returns the doc next after previous
    virtual std::unique_ptr<DocResult> NextResult();
};



class Query {

    std::unique_ptr<ASTNode> BuildTree(std::istream& tokens);

public:
    Query(std::string& query);

    std::unique_ptr<ResultStream> Execute(rocksdb::DB* Snapshot);
};

namespace_Noise_end

#endif /* query_hpp */
