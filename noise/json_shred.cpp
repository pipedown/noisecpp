//
//  json_shred.cpp
//  noise
//
//  Created by Damien Katz on 12/11/15.
//  Copyright © 2015 Damien Katz. All rights reserved.
//


#include <yajl/yajl_parse.h>
#include <exception>

#include "rocksdb/db.h"

#include "noise.h"
#include "stems.h"
#include "records.pb.h"
#include "key_builder.h"

#include "json_shred.h"


namespace_Noise


void ParseCtx::IncTopArrayOffset() {
    /* we encounter a new element. if we are a child element of an array
     increment the offset. If we aren't (we are the root value or a map
     value) we don't increment */
    if (keybuilder.LastPushedSegmentType() == KeyBuilder::Array)
        pathArrayOffsets.back()++;
}


void ParseCtx::AddEntries(const char* text, size_t textLen) {
    Stems stems(text, textLen);

    while (stems.HasMore()) {
        StemmedWord word = stems.Next();
        keybuilder.PushWord(word.stemmed, word.stemmed_len);
        keybuilder.PushDocSeq(docseq);
        map[keybuilder.key()][pathArrayOffsets]
            .push_back({word.stemmed_offset,
                        std::string(word.suffix, word.suffix_len),
                        long(word.suffix_offset - word.stemmed_offset)});
        keybuilder.Pop(KeyBuilder::DocSeq);
        keybuilder.Pop(KeyBuilder::Word);
    }
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
            ctx.tempbuff += std::string(s, l);
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
        ctx.AddEntries((const char*)stringVal, stringLen);
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

        if (keyVal[0] == '_' && ctx.keybuilder.SegmentsCount() == 1) {
            // top level object and we have reserved field. do special handling
            if (keyLen == 3 && keyVal[1] == 'i' && keyVal[2] == 'd') {
                ctx.expectIdString = true;
            } else {
                ctx.ignoreChildren = 1;
            }
            return 1;
        }

        // pop the dummy value
        ctx.keybuilder.Pop(KeyBuilder::ObjectKey);

        // push the real value
        ctx.keybuilder.PushObjectKey((char*)keyVal, keyLen);
       
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
        ctx.keybuilder.PushObjectKey("", 0); // push a dummy value
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
        ctx.keybuilder.Pop(KeyBuilder::ObjectKey);

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
        ctx.keybuilder.PushArray();
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
        ctx.keybuilder.Pop(KeyBuilder::Array);
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
    YajlHandle hand(&ctx_);
    yajl_status stat;
    bool success = true;
    ctx_.docseq = docseq;
    stat = yajl_parse(hand, (unsigned char*)json.c_str(), json.length());

    if (stat == yajl_status_ok)
        stat = yajl_complete_parse(hand);

    if (stat == yajl_status_client_canceled) {
        if (ctx_.exception_ptr) {
            // some sorta exception occurred. rethrow.
            std::rethrow_exception(ctx_.exception_ptr);
        } else {
            // error message must be in tempbuff
            *errout = ctx_.tempbuff;
            success = false;
        }
    } else if (stat != yajl_status_ok) {
        hand.GetError(json, errout);
        success = false;
    }
    if (ctx_.docid.length() == 0) {
        *errout = "missing _id field";
        success = false;
    }
    *idout = ctx_.docid;

    return success;
}

void JsonShredder::AddToBatch(rocksdb::WriteBatch* batch) {
    records::payload pbpayload;
    for (auto wordPathInfos : ctx_.map) {
        auto* pbarrayoffsets_to_wordinfo =
                                    pbpayload.add_arrayoffsets_to_wordinfos();
        for (auto arrayOffsetsInfos : wordPathInfos.second) {
            auto pbwordinfo =
                                    pbarrayoffsets_to_wordinfo->add_wordinfos();
            for (auto arrayOffset : arrayOffsetsInfos.first) {
                pbarrayoffsets_to_wordinfo->add_arrayoffsets(arrayOffset);
            }
            for (auto wordinfo : arrayOffsetsInfos.second) {
                pbwordinfo->set_stemmedoffset(wordinfo.stemmedOffset);
                pbwordinfo->set_suffixtext(wordinfo.suffixText);
                pbwordinfo->set_suffixoffset(wordinfo.suffixOffset);
            }
        }
        std::string payload = pbpayload.SerializeAsString();
        batch->Put(wordPathInfos.first,
                  payload);
    }
}


namespace_Noise_end

