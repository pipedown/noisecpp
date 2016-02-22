//
//  results.hpp
//  noise
//
//  Created by Damien Katz on 2/5/16.
//  Copyright © 2016 Damien Katz. All rights reserved.
//

#ifndef results_hpp
#define results_hpp

#include "noise.h"

namespace_Noise

struct DocResult {
    uint64_t seq;
    std::list< std::vector<uint64_t> > array_paths;

    void TruncateArrayPaths(size_t array_path_depth);
    bool IntersectArrayPaths(const DocResult& other);
};

class Results {
    std::unique_ptr<QueryRuntimeFilter> filter;
    bool first_has_been_called = false;

public:
    Results(std::unique_ptr<QueryRuntimeFilter>& _filter) :
        filter(std::move(_filter)) {}

    uint64_t GetNext() {
        std::unique_ptr<DocResult> dr;
        if (!first_has_been_called) {
            first_has_been_called = true;

            dr = filter->FirstResult(0);
        } else {
            dr = filter->NextResult();
        }

        return dr ? dr->seq : 0;
    }
};

namespace_Noise_end

#endif /* results_hpp */
