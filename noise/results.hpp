//
//  results.hpp
//  noise
//
//  Created by Damien Katz on 2/5/16.
//  Copyright © 2016 Damien Katz. All rights reserved.
//

#ifndef results_hpp
#define results_hpp

#include <stdio.h>
#include "noise.h"


namespace_Noise

class DocResult;

class Results {
    std::unique_ptr<QueryRuntimeFilter> filter;
    bool first_has_been_called;
    Results(std::unique_ptr<QueryRuntimeFilter>& filter) {

    }

    std::string get_next() {

        if (!first_has_been_called) {
            first_has_been_called = true;
            std::unique_ptr<DocResult> dr (filter.FirstResult(""));
            if (dr) {
                
            }
        }
            filter.NextResult()
            return ( ?
        }
            filter
            virtual unique_ptr<DocResult> FirstResult(std::string& startId);

            // returns the doc next after previous
            virtual unique_ptr<DocResult> NextResult();
        };
    }
};

namespace_Noise_end

#endif /* results_hpp */
