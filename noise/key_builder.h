//
//  key_builder.h
//  noise
//
//  Created by Damien Katz on 2/17/16.
//  Copyright © 2016 Damien Katz. All rights reserved.
//

#ifndef key_builder_h
#define key_builder_h

class KeyBuilder {
public:
    // BuildState is really simple state tracker to prevent misuse of api
    enum SegmentType {
        None,
        ObjectKey,
        Array,
        Word,
        DocSeq,
    };
private:
    struct Segment {
        SegmentType type;
        size_t offset;
    };
    std::vector<Segment> segments_;

    std::string fullkey_;
public:
    KeyBuilder() {
        segments_.reserve(10);
        fullkey_.reserve(100); //magic reserve numbers that are completely arbitrary
        fullkey_ = "W"; // first char is keyspace identifier. W means Word keyspace
    }

    size_t SegmentsCount() const {
        return segments_.size();
    }

    const std::string& key() const {return fullkey_;}

    void PushObjectKey(std::string& key) {
        PushObjectKey(key.c_str(), key.length());
    }

    void PushObjectKey(const char* objectkey, size_t len) {
        assert(segments_.size() == 0 || segments_.back().type == ObjectKey ||
               segments_.back().type == Array);
        segments_.push_back({ObjectKey, fullkey_.size()});
        fullkey_.push_back('.');
        const char* end = objectkey + len;
        for (const char* c = objectkey; c < end; c++) {
            switch (*c) {
                // escape chars that conflict with delimiters
                case '\\':
                case '$': //this is the array path delimiter
                case '.': //this is an object key delimiter
                case '!': //is the stemmed word delimiter
                case '#': //this is the doc seq delimiter
                    fullkey_.push_back('\\');
                    // fall though
                default:
                    fullkey_.push_back(*c);
            }

        }
        
    }

    void PushArray() {
        assert(segments_.size() == 0 || segments_.back().type == ObjectKey ||
               segments_.back().type == Array);
        segments_.push_back({Array, fullkey_.size()});
        fullkey_.push_back('$');
    }

    void PushWord(const char* stemmedword, size_t len) {
        assert(segments_.back().type == ObjectKey ||
               segments_.back().type == Array);
        segments_.push_back({Word, fullkey_.size()});
        fullkey_.push_back('!');
        fullkey_.append(stemmedword, len);
        fullkey_.push_back('#');
    }

    void PushDocSeq(uint64_t seq) {
        assert(segments_.back().type == Word);
        segments_.push_back({DocSeq, fullkey_.size()});
        fullkey_ += std::to_string(seq);
    }

    void Pop(SegmentType expected_type) {
        assert(segments_.back().type == expected_type);
        fullkey_.resize(segments_.back().offset);
        segments_.pop_back();
    }

    SegmentType LastPushedSegmentType() {
        return segments_.size() ? segments_.back().type : None;
    }

};


#endif /* key_builder_h */
