//
//  main.cpp
//  test
//
//  Created by Damien Katz on 1/13/16.
//  Copyright © 2016 Damien Katz. All rights reserved.
//

#include <iostream>
#include <stack>
#include <list>
#include <memory>
#include <sstream>
#include <fstream>
#include "query.hpp"
#include "results.hpp"



int main(int argc, const char * argv[]) {
    // Really, this much code to read file into a string?
    std::ifstream querystream("testqueries.txt");
    std::string querystr((std::istreambuf_iterator<char>(querystream)),
                    std::istreambuf_iterator<char>());

    printf("%s\n", querystr.c_str());


    Noise::Query query(querystr);

    Index index("someindex");
    unique_ptr<Noise::Results> results(index.Execute(query));

    string id = results->GetNext();
    while(id.size()) {
        printf("id: %s\n", querystr.c_str());
    }

    return 0;
}