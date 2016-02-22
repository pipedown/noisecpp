//
//  results.cpp
//  noise
//
//  Created by Damien Katz on 2/5/16.
//  Copyright © 2016 Damien Katz. All rights reserved.
//

#include <vector>
#include <list>
#include <string>

#include "query.hpp"
#include "results.hpp"


namespace_Noise


void DocResult::TruncateArrayPaths(size_t array_path_depth) {
    for(auto ap : array_paths)
        ap.resize(array_path_depth);
}
bool DocResult::IntersectArrayPaths(const DocResult& other) {
    for (auto ap = array_paths.begin(); ap != array_paths.end(); ap++) {
        bool found = false;
        for (auto ap_other : other.array_paths) {
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

 namespace_Noise_end
