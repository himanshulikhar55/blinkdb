#pragma once
#include "skiplist.h"
class sparseindex {
    private:
        skiplist index;
    
    public:
        sparseindex() : index(16, 0.5) {}
        
        void insert(const std::string& key, std::streampos offset) {
            index.insert(key, offset);
        }
    
        std::streampos get_offset(const std::string& key) {
            return index.get_nearest_offset(key);
        }
    };
    