//
//  json_shred.cpp
//  noise
//
//  Created by Damien Katz on 12/11/15.
//  Copyright © 2015 Damien Katz. All rights reserved.
//


#include <yajl/yajl_parse.h>
#include <exception>

#include "records.pb.h"

#include "json_shred.hpp"

void ParseCtx::IncTopArrayOffset() {
    /* we encounter a new element. if we are a child element of an array
     increment the offset. If we aren't (we are the root value or a map
     value) we don't increment */
    if (!pathElementsStack.empty() && path.at(pathElementsStack.back()) == '$')
        pathArrayOffsets.back()++;
}


void ParseCtx::AddEntry(const char* text, size_t textLen) {
    const char* nextText = text;
    const char* endText = text + textLen;
    while (nextText < endText) {
        const char* stemmedBegin;
        const char* suffixBegin;
        size_t suffixLen;
        StemNext(nextText, endText - nextText,
                 &stemmedBegin,
                 &suffixBegin, &suffixLen);
        string docseqstr = std::to_string(docseq);
        string key;
        key.reserve(path.length() + tempbuff.length() + docseqstr.length() + 4);
        key += "D";// in the keyspace "D", now build the rest of key
        key += path;
        key += "!";
        key += tempbuff;
        key += "#";
        key += docseqstr;
        map[key][pathArrayOffsets].push_back({size_t(stemmedBegin - text),
                                              string(suffixBegin, suffixLen),
                                              long(suffixBegin - stemmedBegin)});
        nextText = suffixBegin + suffixLen;
    }
}

void ParseCtx::StemNext(const char* text, size_t len,
                        const char** stemmedBegin,
                        const char** suffixBegin, size_t* suffixLen) {
    tempbuff.resize(0);
    *suffixLen = 0;
    *suffixBegin = nullptr;
    const char* t = text;
    const char* end = text + len;
    while (t < end) {
        //skip non-alpha. this puts any leading whitespace into suffix
        char c = *t;
        if (isupper(c) || islower(c))
            break;
        t++;
    }
    if (t != text)
        // if we advanced past some non-alpha text, we preserve it in the
        // suffix (which is only usually a suffix, not always)
        *suffixBegin = text;
    //stemmedText must begin here.
    *stemmedBegin = t;
    while (t < end) {
        char c = *t;
        if (!isupper(c) && !islower(c))
            break;
        char l = tolower(c);
        // we changed the case and we if we aren't already tracking suffix
        // chars then do it now.
        if (l != c && !*suffixBegin)
            *suffixBegin = t;
        tempbuff.push_back(l);
        t++;
    }
    if (!*suffixBegin)
        *suffixBegin = t;
    while (t < end) {
        //skip non-alpha
        char c = *t;
        if (isupper(c) || islower(c))
            break;
        t++;
    }
    if (tempbuff.size()) {
        // stem the word
        size_t len = stemmer.Stem(&tempbuff.front(), tempbuff.size());
        tempbuff.resize(len);
    }
    *suffixLen = t - *suffixBegin;
}


static int callback_null(void * v)
{
    try {
        ParseCtx& ctx = *(ParseCtx*)v;
        ctx.IncTopArrayOffset();
        if (ctx.ignoreChildren) {
            return 1;
        }
        if (ctx.expectIdString) {
            ctx.tempbuff = "Expected string in _id field. Found null.";
            return 0;
        }

        return 1;
    } catch (std::exception& ) {
        std::exception_ptr exception_ptr;
        return 0;
    }
}

static int callback_boolean(void * v, int boolean)
{
    try {
        ParseCtx& ctx = *(ParseCtx*)v;
        ctx.IncTopArrayOffset();
        if (ctx.ignoreChildren) {
            return 1;
        }
        if (ctx.expectIdString) {
            ctx.tempbuff = "Expected string in _id field. Found boolean: ";
            ctx.tempbuff += boolean ? "true" : "false";
            return 0;
        }
        return 1;
    } catch (std::exception& ) {
        std::exception_ptr exception_ptr;
        return 0;
    }
}

static int callback_number(void * v, const char * s, size_t l)
{
    try {
        ParseCtx& ctx = *(ParseCtx*)v;
        ctx.IncTopArrayOffset();
        if (ctx.ignoreChildren) {
            return 1;
        }
        if (ctx.expectIdString) {
            ctx.tempbuff = "Expected string in _id field. Found number: ";
            ctx.tempbuff += string(s, l);
            return 0;
        }
        return 1;
    } catch (std::exception& ) {
        std::exception_ptr exception_ptr;
        return 0;
    }
}



static int callback_string(void * v, const unsigned char * stringVal,
                    size_t stringLen)
{
    try {
        ParseCtx& ctx = *(ParseCtx*)v;
        if (ctx.ignoreChildren) {
            return 1;
        } else if (ctx.expectIdString) {
            ctx.docid.assign((const char*)stringVal, stringLen);
            ctx.expectIdString = false;
            return 1;
        }
        ctx.IncTopArrayOffset();
        ctx.AddEntry((const char*)stringVal, stringLen);
        return 1;
    } catch (std::exception& ) {
        std::exception_ptr exception_ptr;
        return 0;
    }
}

static int callback_map_key(void* v, const unsigned char * keyVal,
                     size_t keyLen) {
    try {
        ParseCtx& ctx = *(ParseCtx*)v;
        if (ctx.ignoreChildren == 1) {
            // this can only happen if the previous sibling key was special.
            // we must have already seen the special key and now we can stop
            // special handling
            ctx.ignoreChildren = 0;
        } else if (ctx.ignoreChildren > 1) {
            // we are inside a child object of a special key. ignore.
            return 1;
        }
        ctx.path.resize(ctx.pathElementsStack.back() + 1);

        if (keyVal[0] == '_' && ctx.path == ".") {
            // top level object and we have reserved field. do special handling
            if (keyVal[1] == 'i' && keyVal[2] == 'd' && keyLen == 3) {
                ctx.expectIdString = true;
            } else {
                ctx.ignoreChildren = 1;
            }
            return 1;
        }

        const unsigned char* keyEnd = keyVal + keyLen;
        while (keyVal != keyEnd) {
            char c = *keyVal++;
            switch (c) {
                case '\\':
                case '$':
                case '.':
                case '!':
                case '#':
                    // escape special chars
                    ctx.path.push_back('\\');
                default:
                    ctx.path.push_back(c);
            }

        }
        return 1;
    } catch (std::exception& ) {
        std::exception_ptr exception_ptr;
        return 0;
    }
}

static int callback_start_map(void* v)
{
    try {
        ParseCtx& ctx = *(ParseCtx*)v;
        ctx.IncTopArrayOffset();
        if (ctx.ignoreChildren) {
            ctx.ignoreChildren++;
            return 1;
        } else if (ctx.expectIdString) {
            ctx.tempbuff = "Expected string in _id field. Found object.";
            return 0;
        }
        ctx.pathElementsStack.push_back(ctx.path.length());
        ctx.path.push_back('.'); // indicate key
        return 1;
    } catch (std::exception& ) {
        std::exception_ptr exception_ptr;
        return 0;
    }
}

static int callback_end_map(void * v)
{
    try {
        ParseCtx& ctx = *(ParseCtx*)v;
        if (ctx.ignoreChildren) {
            ctx.ignoreChildren--;
            return 1;
        }
        ctx.path.resize(ctx.pathElementsStack.back());
        ctx.pathElementsStack.pop_back();

        return 1;
    } catch (std::exception& ) {
        std::exception_ptr exception_ptr;
        return 0;
    }
}

static int callback_start_array(void * v)
{
    try {
        ParseCtx& ctx = *(ParseCtx*)v;
        ctx.IncTopArrayOffset();
        ctx.pathElementsStack.push_back(ctx.path.length());
        ctx.path.push_back('$');
        ctx.pathArrayOffsets.push_back(0);
        if (ctx.ignoreChildren) {
            ctx.ignoreChildren++;
            return 1;
        } else if (ctx.expectIdString) {
            ctx.tempbuff = "Expected string in _id field. Found object.";
            return 0;
        }
        return 1;
    } catch (std::exception& ) {
        std::exception_ptr exception_ptr;
        return 0;
    }
}

static int callback_end_array(void * v)
{
    try {
        ParseCtx& ctx = *(ParseCtx*)v;
        ctx.pathArrayOffsets.pop_back();
        ctx.path.resize(ctx.pathElementsStack.back());
        ctx.pathElementsStack.pop_back();
        if (ctx.ignoreChildren) {
            ctx.ignoreChildren--;
            return 1;
        }
        return 1;
    } catch (std::exception& ) {
        std::exception_ptr exception_ptr;
        return 0;
    }
}

static yajl_callbacks callbacks = {
    callback_null,
    callback_boolean,
    NULL,
    NULL,
    callback_number,
    callback_string,
    callback_start_map,
    callback_map_key,
    callback_end_map,
    callback_start_array,
    callback_end_array
};

/** This is helper struct to clean up the yajl parser and associated items
 * without convoluted error handling or leaks. **/
struct YajlHandle {
    yajl_handle hand;
    unsigned char* errstring;

    YajlHandle(void* ctx) : errstring(nullptr){
        hand = yajl_alloc(&callbacks, NULL, ctx);
        if (!hand)
            throw std::runtime_error("failure to allocate json parser");
    }

    operator yajl_handle () {
        return hand;
    }

    /* this method can only be called once */
    void GetError(const std::string& json, std::string* errout) {
        assert(errstring == nullptr);
        errstring = yajl_get_error(hand, 1,
                       (unsigned char*)json.c_str(),
                       json.length());
         *errout = std::string((char*)errstring);

    }

    ~YajlHandle() {
        if (errstring)
            yajl_free_error(hand, errstring);
        if (hand)
            yajl_free(hand);

    }
};

/* if this method throw an exception then the object is invalid */
bool JsonShredder::Shred(uint64_t docseq,
                         const std::string& json,
                            std::string* idout,
                            std::string* errout) {
    YajlHandle hand(&ctx);
    yajl_status stat;
    bool success = true;
    ctx.docseq = docseq;
    stat = yajl_parse(hand, (unsigned char*)json.c_str(), json.length());

    if (stat == yajl_status_ok)
        stat = yajl_complete_parse(hand);

    if (stat == yajl_status_client_canceled) {
        if (ctx.exception_ptr) {
            // some sorta exception occurred. rethrow.
            std::rethrow_exception(ctx.exception_ptr);
        } else {
            // error message must be in tempbuff
            *errout = ctx.tempbuff;
            success = false;
        }
    } else if (stat != yajl_status_ok) {
        hand.GetError(json, errout);
        success = false;
    }
    if (ctx.docid.length() == 0) {
        *errout = "missing _id field";
        success = false;
    }
    *idout = ctx.docid;

    if (!success)
        ctx.Reset();

    return success;
}

void JsonShredder::AddToBatch(rocksdb::WriteBatch* batch) {
    records::payload pbpayload;
    for (auto wordPathInfos : ctx.map) {
        records::arrayoffsets_to_wordinfo& pbarrayoffsets_to_wordinfo =
                                    *pbpayload.add_arrayoffsets_to_wordinfos();
        for (auto arrayOffsetsInfos : wordPathInfos.second) {
            records::wordinfo& pbwordinfo =
                                    *pbarrayoffsets_to_wordinfo.add_wordinfos();
            for (auto arrayOffset : arrayOffsetsInfos.first) {
                pbarrayoffsets_to_wordinfo.add_arrayoffsets(arrayOffset);
            }
            for (auto wordinfo : arrayOffsetsInfos.second) {
                pbwordinfo.set_stemmedoffset(wordinfo.stemmedOffset);
                pbwordinfo.set_suffixtext(wordinfo.suffixText);
                pbwordinfo.set_suffixoffset(wordinfo.suffixOffset);
            }
        }
        batch->Put(wordPathInfos.first,
                  pbarrayoffsets_to_wordinfo.SerializeAsString());
    }
    ctx.Reset();
}

