//
//  main.cpp
//  noise
//
//  Created by Damien Katz on 12/10/15.
//  Copyright © 2015 Damien Katz. All rights reserved.
//

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

#include "noise.h"

using Noise::Updater;
using Noise::OpenOptions;

int main(int argc, char * const argv[]) {
    int c;
    int index;
    opterr = 0;
    OpenOptions openOptions = OpenOptions::None;
    int dodelete = 0;
    Updater updater;
    while ((c = getopt(argc, argv, "cd")) != -1)
        switch (c)
    {
        case 'c':
            openOptions = OpenOptions::Create;
            break;

        case 'd':
            dodelete = 1;
            break;

        case '?':
            if (isprint (optopt))
                fprintf(stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf(stderr,
                        "Unknown option character `\\x%x'.\n",
                        optopt);
            break;
        default:
            abort();
    }

    for (index = optind + 1; index < argc; index++)
        fprintf(stderr, "Ignoring argument %s\n", argv[index]);

    if (optind >= argc) {
        fprintf(stderr, "Missing index directory argument\n");
        return 1;
    }
    if (dodelete)
        Updater::Delete(argv[optind]);

    std::string error = updater.Open(argv[optind]);

    if (error.length()) {
        std::cout << "Error opening index (" << argv[optind] << "):" << error;
    }

    // stream shredded docs into in index

    printf("Hello, World!\n");
    return 0;
}
