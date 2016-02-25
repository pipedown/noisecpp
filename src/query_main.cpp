//
//  main.cpp
//  test
//
//  Created by Damien Katz on 1/13/16.
//  Copyright © 2016 Damien Katz. All rights reserved.
//

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fstream>
#include "noise.h"
#include "query.h"
#include "results.h"


int main(int argc, char * const argv[]) {
    opterr = 0;

    std::ifstream in;

    int c;
    while ((c = getopt(argc, argv, "f:")) != -1) {
        switch (c) {
            case 'f':
            {
                in.open(optarg);
                if (!in.is_open()) {
                    fprintf(stderr, "Unable to open '-%s'.\n", optarg);
                    return 1;
                }
                std::cin.rdbuf(in.rdbuf()); //redirect std::cin
                break;
            }

            case '?':
                if (optopt == 'f')
                    fprintf(stderr, "Option '-f' requires a filename.\n");
                else if (isprint (optopt))
                    fprintf(stderr, "Unknown option '-%c'.\n", optopt);
                else
                    fprintf(stderr,
                            "Unknown option character '\\x%x'.\n",
                            optopt);
                return 1;
            default:
                abort();
        }
    }

    for (int i = optind + 1; i < argc; i++)
        fprintf(stderr, "Ignoring argument %s\n", argv[i]);
    
    if (optind >= argc) {
        fprintf(stderr, "Missing index directory argument\n");
        return 1;
    }

    std::string line;

    std::getline(in, line);

    Noise::Index index;
    std::string error = index.Open(argv[optind]);
    if (error.length()) {
        fprintf(stderr, "Error opening index (%s): %s\n", argv[1], error.c_str());
        return 1;
    }

    while (std::getline(in, line)) {
        if (line.size() != 0 && line.find("//") != 0) {
            Noise::Query query(line);

            unique_ptr<Noise::Results> results(query.Execute(&index));

            uint64_t seq;
            std::string id;
            while ((seq = results->GetNext())) {
                if (index.FetchId(seq, &id))
                    std::cout << "id: " << id << " seq:" << seq <<"\n";
                else
                    std::cout << "Failure to lookup seq " << seq << "\n";
            }
        }
    }

    return 0;
}