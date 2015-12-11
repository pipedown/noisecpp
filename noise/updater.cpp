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


namespace_Noise



Updater::Updater() : rocks(nullptr) {

}

Updater::~Updater() {
    delete rocks;
}

std::string Updater::Open(const std::string& name, OpenOptions opt) {
    rocksdb::Options options;
    options.create_if_missing = opt == OpenOptions::Create;
    rocksdb::Status status = rocksdb::DB::Open(options, name, &rocks);
    if (status.ok())
        return std::string();
    else
        return status.ToString();
}

void Updater::Delete(const std::string& name) {
    rocksdb::Options options;
    rocksdb::DestroyDB(name, options);
}



void Updater::AddDoc(const std::string& json)
{

}
    
void Updater::Flush()
{

}

namespace_Noise_end