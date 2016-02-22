//
//  main.cpp
//  test
//
//  Created by Damien Katz on 1/13/16.
//  Copyright © 2016 Damien Katz. All rights reserved.
//

#include <iostream>
#include <string>
#include <stack>
#include <list>
#include <memory>
#include <sstream>
#include <fstream>
#include "noise.h"
#include "query.hpp"
#include "results.hpp"



int main(int argc, const char * argv[]) {
    // Really, this much code to read file into a string?
    std::ifstream querystream("testqueries.txt");

    std::string line;

    std::getline(querystream, line);

    Noise::Index index;
    std::string error = index.Open(argv[0]);
    if (error.length()) {
        fprintf(stderr, "Error opening index (%s): %s\n", argv[0], error.c_str());
        return 1;
    }

    while (true) {
        if (line.size() == 0 || line.find("//") != 0)
            continue;
            Noise::Query query(line);

            std::unique_ptr<Noise::Results> results(query.Execute(&index));

            uint64_t seq = results->GetNext();
            while(seq) {
                printf("id: %ul\n", seq);
            }
        std::getline(querystream, line);
    }

    return 0;
}