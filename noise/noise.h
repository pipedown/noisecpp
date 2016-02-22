//
//  noise.h
//  noise
//
//  Created by Damien Katz on 12/10/15.
//  Copyright © 2015 Damien Katz. All rights reserved.
//

#ifndef noise_h
#define noise_h


// this is a stupid trick to work around Xcode editor indenting inside parens
#define namespace_Noise namespace Noise {
#define namespace_Noise_end };

#include <cstdint>
#include <rocksdb/db.h>

namespace_Noise

enum OpenOptions {
    None,
    Create
};

class Index
{
private:
    rocksdb::DB* rocks;
    rocksdb::WriteOptions wopt;
    rocksdb::ReadOptions  ropt;

    rocksdb::WriteBatch batch;

    uint64_t highdocseq;
    std::map<std::string,std::string> idStrToIdSeq;

public:
    Index();
    ~Index();

    rocksdb::DB* GetDB() {return rocks;}

    std::string Open(const std::string& name, OpenOptions opt=None);

    static void Delete(const std::string& name);

    bool Add(const std::string& json, std::string* err);

    bool FetchId(uint64_t seq, std::string* id);
    
    rocksdb::Status Flush();
};


namespace_Noise_end

#endif /* noise_h */
