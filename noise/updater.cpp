//
//  updater.cpp
//  noise
//
//  Created by Damien Katz on 12/10/15.
//  Copyright © 2015 Damien Katz. All rights reserved.
//

#include <stdio.h>
#include <string>

#include <rocksdb/db.h>
#include "noise.h"
#include "records.pb.h"
#include "json_shred.hpp"

namespace_Noise

Index::Index() : rocks(nullptr) {
    wopt.sync = true;
}

Index::Index() {
    delete rocks;
}

std::string Index::Open(const std::string& name, OpenOptions opt) {
    rocksdb::Options options;
    rocksdb::Status st = rocksdb::DB::Open(options, name, &rocks);
    if (!st.ok()) {
        if (opt != OpenOptions::Create)
            return st.ToString();

        options.create_if_missing = true;

        st = rocksdb::DB::Open(options, name, &rocks);
        if (!st.ok())
            return st.ToString();

        //create new index. inital with empty header
        records::header header;
        header.set_version(1);
        header.set_high_seq(0);
        st = rocks->Put(wopt, "HDB", header.SerializeAsString());
        if (!st.ok())
            return st.ToString();
    }

    //validate header is there
    std::string value;
    st = rocks->Get(ropt, "HDB",  &value);

    if (!st.ok())
        return st.ToString();

    records::header header;
    header.ParseFromString(value);
    assert(header.version() == 1);
    highdocseq = header.high_seq();

    return "";
}

void Index::Delete(const std::string& name) {
    rocksdb::Options options;
    rocksdb::DestroyDB(name, options);
}


bool Index::Add(const std::string& json, std::string* err)
{
    std::string id;
    if (shred.Shred(highdocseq + 1, json, &id, err)) {
        idStrToIdSeq[std::string("I") + id] = "S" + std::to_string(++highdocseq);
        shred.AddToBatch(&batch);
        return true;
    } else {
        return false;
    }
}

rocksdb::Status Index::Flush()
{
    //look up all doc ids and 'delete' from the seq_to_ids keyspace
    std::vector<rocksdb::Slice> keys;
    for (auto idseq : idStrToIdSeq) {
        keys.push_back(idseq.first);
    }

    std::vector<std::string> seqsToDelete;

    rocks->MultiGet(ropt, keys, &seqsToDelete);

    std::string seqToDelete;
    for (auto seqToDelete : seqsToDelete) {
        if (seqToDelete.length()) {
            batch.Delete(seqToDelete);
        }
    }

    //add the ids_to_seq keyspace entries

    for (auto idseq : idStrToIdSeq) {
        batch.Put(idseq.first, idseq.second);
        batch.Put(idseq.second, idseq.first);
    }

    records::header header;
    header.set_version(1);
    header.set_high_seq(highdocseq);
    batch.Put("HDB", header.SerializeAsString());

    rocksdb::Status status = rocks->Write(wopt, &batch);
    batch.Clear();
    idStrToIdSeq.clear();
    
    return status;
}

namespace_Noise_end