//
//  stemmed_key.h
//  noise
//
//  Created by Damien Katz on 2/17/16.
//  Copyright © 2016 Damien Katz. All rights reserved.
//

#ifndef stemmed_key_h
#define stemmed_key_h

class StemmedKeyBuilder {
public:
    // BuildState is really simple state tracker to prevent misuse of api
    enum SegmentType {
        None,
        ObjectKey,
        Array,
        Word,
        DocSeq,
    };
    StemmedKeyBuilder() {
        segments.reserve(10);
        fullkey.reserve(100); //magic reserve numbers that are completely arbitrary
        fullkey = "W"; // first char is keyspace identifier. W means Word keyspace
    }

    size_t SegmentsCount() const {
        return segments.size();
    }

    const std::string& Key() const {return fullkey;}

    void PushObjectKey(std::string& key) {
        PushObjectKey(key.c_str(), key.length());
    }

    void PushObjectKey(const char* objectkey, size_t len) {
        assert(segments.size() == 0 || segments.back().type == ObjectKey ||
               segments.back().type == Array);
        segments.push_back({ObjectKey, fullkey.size()});
        fullkey.push_back('.');
        const char* end = objectkey + len;
        for (const char* c = objectkey; c < end; c++) {
            switch (*c) {
                // escape chars that conflict with delimiters
                case '\\':
                case '$': //this is the array path delimiter
                case '.': //this is an object key delimiter
                case '!': //is the stemmed word delimiter
                case '#': //this is the doc seq delimiter
                    fullkey.push_back('\\');
                    // fall though
                default:
                    fullkey.push_back(*c);
            }

        }
        
    }

    void PushArray() {
        assert(segments.size() == 0 || segments.back().type == ObjectKey ||
               segments.back().type == Array);
        segments.push_back({Array, fullkey.size()});
        fullkey.push_back('$');
    }

    void PushWord(const char* stemmedword, size_t len) {
        assert(segments.back().type == ObjectKey ||
               segments.back().type == Array);
        segments.push_back({Word, fullkey.size()});
        fullkey.push_back('!');
        fullkey.append(stemmedword, len);
        fullkey.push_back('#');
    }

    void PushDocSeq(uint64_t seq) {
        assert(segments.back().type == Word);
        segments.push_back({DocSeq, fullkey.size()});
        fullkey += std::to_string(seq);
    }

    void Pop(SegmentType expected_type) {
        assert(segments.back().type == expected_type);
        segments.pop_back();
    }

    SegmentType LastPushedSegmentType() {
        return segments.size() ? segments.back().type : None;
    }
private:

    struct Segment {
        SegmentType type;
        size_t offset;
    };
    std::vector<Segment> segments;

    std::string fullkey;
};


#endif /* stemmed_key_h */
