//
//  main.cpp
//  noise
//
//  Created by Damien Katz on 12/10/15.
//  Copyright © 2015 Damien Katz. All rights reserved.
//

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <rocksdb/db.h>

#include "noise.h"

using Noise::Index;
using Noise::OpenOptions;
int main(int argc, char * const argv[]) {
    opterr = 0;
    OpenOptions openOptions = OpenOptions::None;
    int dodelete = 0;
    Index index;

    std::ifstream in;

    int c;
    while ((c = getopt(argc, argv, "cdf:")) != -1)
        switch (c) {
        case 'c':
            openOptions = OpenOptions::Create;
            break;

        case 'd':
            dodelete = 1;
            break;

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

    for (int i = optind + 1; i < argc; i++)
        fprintf(stderr, "Ignoring argument %s\n", argv[i]);

    if (optind >= argc) {
        fprintf(stderr, "Missing index directory argument\n");
        return 1;
    }
    if (dodelete)
        Index::Delete(argv[optind]);

    std::string error = index.Open(argv[optind], openOptions);

    if (error.length()) {
        fprintf(stderr, "Error opening index (%s): %s\n", argv[optind], error.c_str());
        return 1;
    }

    // stream shredded docs into in index

    while (!std::cin.eof()) {
        std::string json;
        std::getline(std::cin, json);
        if (!index.Add(json, &error)) {
            fprintf(stderr, "Error processing document: %s\n", error.c_str());
        }
    }

    rocksdb::Status status = index.Flush();

    assert(status.ok());

    return 0;
}
