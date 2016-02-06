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



int main(int argc, const char * argv[]) {
    std::ifstream qeurytextstrem("testqueries.txt");
    std::string qeurytex((std::istreambuf_iterator<char>(qeurytextstrem)),
                    std::istreambuf_iterator<char>());

    printf("%s\n", qeurytex.c_str());


    Noise::Query query(qeurytex);

    unique_ptr<Noise::Results> results(query.execute(rss));

    for(auto doc : results) {

    }

    return 0;
}