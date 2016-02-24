//
//  stems.cpp
//  noise
//
//  Created by Damien Katz on 2/23/16.
//  Copyright © 2016 Damien Katz. All rights reserved.
//

#include "noise.h"
#include "stems.h"
#include "porter.h"

namespace_Noise

StemmedWord Stems::Next() {
    tempbuf_.resize(0);
    StemmedWord ret;
    std::string str;
    const char* start = current_;

    while (current_ < input_end_) {
        //skip non-alpha. this puts any leading whitespace into suffix
        char c = *current_;
        if (isupper(c) || islower(c))
            break;
        current_++;
    }
    if (current_ != start)
        // if we advanced past some non-alpha text, we preserve it in the
        // suffix (which is only usually a suffix, not always)
        ret.suffix = start;

    //stemmedText must begin here.
    ret.stemmed_offset = current_ - input_;

    while (current_ < input_end_) {
        char c = *current_;
        if (!isupper(c) && !islower(c))
            break;
        char l = tolower(c);
        // we changed the case and we if we aren't already tracking suffix
        // chars then do it now.
        if (l != c && !ret.suffix)
            ret.suffix = current_;
        tempbuf_.push_back(l);
        current_++;
    }
    if (!ret.suffix)
        ret.suffix = current_;
    ret.suffix_offset = ret.suffix - input_;
    while (current_ < input_end_) {
        //skip non-alpha
        char c = *current_;
        if (isupper(c) || islower(c))
            break;
        current_++;
    }
    if (tempbuf_.size()) {
        // stem the word
        size_t len = porter_stem_inplace(&tempbuf_.front(),
                                         (int)tempbuf_.size());
        tempbuf_.resize(len);
        if (ret.stemmed_offset + len < ret.suffix_offset) {
            ret.suffix_offset = ret.stemmed_offset + len;
            ret.suffix = input_ + ret.stemmed_offset;

        }

    }
    ret.suffix_len = current_ - ret.suffix;
    ret.stemmed = &tempbuf_.front();
    ret.stemmed_len = tempbuf_.length();
    return ret;
}

namespace_Noise_end