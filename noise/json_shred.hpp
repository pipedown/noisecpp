//
//  json_shred.hpp
//  noise
//
//  Created by Damien Katz on 12/11/15.
//  Copyright © 2015 Damien Katz. All rights reserved.
//

#ifndef json_shred_hpp
#define json_shred_hpp

#include <string>
#include <vector>
#include <map>
#include "porter.h"
#include "rocksdb/db.h"


using std::string;
using std::vector;
using std::map;

struct word_info
{
    //offset in the text field where the stemmed text starts
    size_t stemmedOffset;

    // the suffix of the stemmed text. When applied over stemmed, the original
    // text is returned.
    string suffixText;

    // the start of the suffixText, relative the stemmedOffset, if negative
    // the suffix starts before the stemmed word (in the case of the leading WS)
    long suffixOffset;

};

typedef vector<size_t> array_offsets;

typedef map<array_offsets, vector<word_info> > array_offsets_to_word_info;

typedef map<string, array_offsets_to_word_info > word_path_info_map;

struct ParseCtx {
public:
    string docid;
    bool expectIdString;
    unsigned long ignoreChildren;
    uint64_t docseq;
    string path;
    vector<size_t> pathArrayOffsets;
    vector<size_t> pathElementsStack;

    word_path_info_map map;
    Stemmer stemmer;

    std::exception_ptr exception_ptr;
    string tempbuff;

    ParseCtx() : expectIdString(false), ignoreChildren(0), docseq(0) {}

    void Reset() {
        expectIdString = false;
        ignoreChildren = 0;
        path.clear();
        pathArrayOffsets.clear();
        pathElementsStack.clear();
        docid.clear();
    }
    void IncTopArrayOffset();
    void AddEntry(const char* text, size_t len);
    void StemNext(const char* text, size_t len,
                  const char** stemmedBegin,
                  const char** suffixBegin, size_t* suffixLen);
};

class JsonShredder {
private:
    ParseCtx ctx;
public:
    bool Shred(uint64_t docseq, const std::string& json,
               std::string* idout, std::string* errout);

    void AddToBatch(rocksdb::WriteBatch* batch);
};

#endif /* json_shred_hpp */
