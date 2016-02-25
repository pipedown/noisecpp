//
//  stems.h
//  noise
//
//  Created by Damien Katz on 2/23/16.
//  Copyright © 2016 Damien Katz. All rights reserved.
//

#ifndef stems_h
#define stems_h


namespace_Noise


// "Abc 123abc abc123 123 a"
/*

 stemmed: abc
 stemoff: 0
 suffix: "Abc "
 sufoff:  0

 stemmed: abc
 stemoff: 8
 suffix: "123abc "
 sufoff:  5

 stemmed: abc
 stemoff: 12
 suffix: "123 123 "
 sufoff:  15

 stemmed: a
 stemoff:
 suffix: ""
 sufoff:  15

 */

struct StemmedWord {
    size_t stemmed_offset;
    const char* stemmed = nullptr;
    size_t stemmed_len;

    size_t suffix_offset;
    const char* suffix = nullptr;
    size_t suffix_len;
};

class Stems {
    std::string tempbuf_;
    const char* input_;
    const char* input_end_;
    const char* current_;

public:

    Stems(const std::string& str)
        : input_(&*str.begin()), input_end_(&*str.end()), current_(input_) {}

    Stems(const char* input, size_t inputlen)
        : input_(input), input_end_(input_ + inputlen), current_(input) {}

    bool HasMore() {return current_ < input_end_;}

    StemmedWord Next();
};

namespace_Noise_end

#endif /* Stems_h */
