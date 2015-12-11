//
//  noise.h
//  noise
//
//  Created by Damien Katz on 12/10/15.
//  Copyright © 2015 Damien Katz. All rights reserved.
//

#ifndef noise_h
#define noise_h

// this is a stupid trick to work around Xcode indenting inside parens
#define namespace_Noise namespace Noise {
#define namespace_Noise_end };

namespace_Noise

enum OpenOptions {
    None,
    Create
};

class Updater
{
private:
    rocksdb::DB* rocks;

public:
    Updater();
    ~Updater();


    std::string Open(const std::string& name, OpenOptions opt=None);

    static void Delete(const std::string& name);

    void Add(const std::string& json);
    
    void Flush();
};


namespace_Noise_end

#endif /* noise_h */
