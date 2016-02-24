//
//  main.cpp
//  test
//
//  Created by Damien Katz on 1/13/16.
//  Copyright © 2016 Damien Katz. All rights reserved.
//

#include <fstream>
#include "noise.h"
#include "query.h"
#include "results.h"



int main(int argc, const char * argv[]) {
    std::ifstream querystream("testqueries.txt");

    std::string line;

    std::getline(querystream, line);

    Noise::Index index;
    std::string error = index.Open(argv[1]);
    if (error.length()) {
        fprintf(stderr, "Error opening index (%s): %s\n", argv[1], error.c_str());
        return 1;
    }

    while (std::getline(querystream, line)) {
        if (line.size() != 0 && line.find("//") != 0) {
            Noise::Query query(line);

            unique_ptr<Noise::Results> results(query.Execute(&index));

            uint64_t seq;
            std::string id;
            while ((seq = results->GetNext())) {
                if (index.FetchId(seq, &id))
                    std::cout << "id: " << id << " seq:"<< seq <<"\n";
                else
                    std::cout << "Failure to looku seq " << seq << "\n";
            }
        }
    }

    return 0;
}