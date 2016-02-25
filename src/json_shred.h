//
//  json_shred.h
//  noise
//
//  Created by Damien Katz on 12/11/15.
//  Copyright © 2015 Damien Katz. All rights reserved.
//

#ifndef json_shred_h
#define json_shred_h

namespace_Noise

struct word_info
{
    //offset in the text field where the stemmed text starts
    size_t stemmedOffset;

    // the suffix of the stemmed text. When applied over stemmed, the original
    // text is returned.
    std::string suffixText;

    // the start of the suffixText, relative the stemmedOffset, if negative
    // the suffix starts before the stemmed word (in the case of the leading WS)
    long suffixOffset;

};

typedef std::vector<size_t> array_offsets;

typedef std::map<array_offsets, std::vector<word_info> >
            array_offsets_to_word_info;

typedef std::map<std::string, array_offsets_to_word_info > word_path_info_map;

struct ParseCtx {
public:
    std::string docid;
    bool expectIdString = false;
    unsigned long ignoreChildren = 0;
    uint64_t docseq = 0;
    std::vector<size_t> pathArrayOffsets;
    KeyBuilder keybuilder;

    word_path_info_map map;

    std::exception_ptr exception_ptr = nullptr;
    std::string tempbuff;

    void IncTopArrayOffset();
    void AddEntries(const char* text, size_t len);
};

class JsonShredder {
private:
    ParseCtx ctx_;
public:
    static void AddPathToString(std::string& path_path, std::string& dest);

    bool Shred(uint64_t docseq, const std::string& json,
               std::string* idout, std::string* errout);

    void AddToBatch(rocksdb::WriteBatch* batch);
};


namespace_Noise_end

#endif /* json_shred_hpp */
