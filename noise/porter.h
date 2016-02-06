//
//  porter.h
//  JBuick
//
//  Created by Damien Katz on 12/6/15.
//  Copyright © 2015 Damien Katz. All rights reserved.
//

#ifndef porter_h
#define porter_h

#ifdef __cplusplus
extern "C" {
#endif

struct stemmer;
typedef struct stemmer* porter;

porter create_stemmer(void);
void free_stemmer(porter z);
int stem(porter z, char * b, int k);

#ifdef __cplusplus
}

class Stemmer {
    porter stemmer;
public:
    Stemmer() : stemmer(nullptr) {
        stemmer = create_stemmer();
        if (stemmer == nullptr) {
            throw std::runtime_error("Couldn't allocate stemmer");
        }
    }

    ~Stemmer() {
        free_stemmer(stemmer);
    }

    size_t Stem(char* text, size_t k) {
        return (size_t)stem(stemmer, text, (int)k);
    }
};

#endif

#endif /* porter_h */
