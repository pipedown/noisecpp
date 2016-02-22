//
//  porter.h
//  JBuick
//
//  Created by Damien Katz on 12/6/15.
//  Copyright © 2015 Damien Katz. All rights reserved.
//

#ifndef porter_h
#define porter_h

int porter_stem_inplace(char * b, int k);

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
    const char* stemmed;
    size_t stemmed_len;

    size_t suffix_offset;
    const char* suffix;
    size_t suffix_len;
};

class Stems {
    std::string tempbuf;
    const char* input;
    const char* input_end;
    const char* current;

public:

    Stems(const std::string& str) :
        input(&*str.begin()), input_end(&*str.end()), current(input) {}

    Stems(const char* input_, size_t inputlen) :
        input(input_), input_end(input_ + inputlen), current(input) {}

    bool HasMore() {
        return current < input_end;
    }

    StemmedWord Next() {
        tempbuf.resize(0);
        StemmedWord ret;
        std::string str;
        const char* start = current;

        while (current < input_end) {
            //skip non-alpha. this puts any leading whitespace into suffix
            char c = *current;
            if (isupper(c) || islower(c))
                break;
            current++;
        }
        if (current != start)
            // if we advanced past some non-alpha text, we preserve it in the
            // suffix (which is only usually a suffix, not always)
            ret.suffix = start;

        //stemmedText must begin here.
        ret.stemmed_offset = current - input;

        while (current < input_end) {
            char c = *current;
            if (!isupper(c) && !islower(c))
                break;
            char l = tolower(c);
            // we changed the case and we if we aren't already tracking suffix
            // chars then do it now.
            if (l != c && !ret.suffix)
                ret.suffix = current;
            tempbuf.push_back(l);
            current++;
        }
        if (!ret.suffix)
            ret.suffix = current;
        while (current < input_end) {
            //skip non-alpha
            char c = *current;
            if (isupper(c) || islower(c))
                break;
            current++;
        }
        if (tempbuf.size()) {
            // stem the word
            size_t len = porter_stem_inplace(&tempbuf.front(), (int)tempbuf.size());
            tempbuf.resize(len);
        }
        ret.suffix_len = current - ret.suffix;
        ret.suffix_offset = ret.suffix - input;
        ret.stemmed = &tempbuf.front();
        ret.stemmed_len = tempbuf.length();
        return ret;
    }
};


#endif /* porter_h */
