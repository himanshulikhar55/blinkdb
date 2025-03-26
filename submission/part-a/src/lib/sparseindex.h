
/**
 * @file sparseindex.h
 *Sparse index implementation using a skip list.
 *
 * This file defines the sparseindex class, which encapsulates a skip list that stores key-offset pairs.
 * The class leverages the efficiency of skip lists to allow fast insertion and nearest-neighbor offset retrieval,
 * which is particularly useful for indexing. For N keys, the maximum level L for the skip list is recommended as:
 * \f[
 * L = \log_{1/p}(N)
 * \f]
 * With p chosen as 0.5, the formula simplifies to:
 * 
  * \f[
 * L = \log_2(N)
 * \f]
 *
 * The design aims to provide a balance between space efficiency and quick lookup times, utilizing the probabilistic
 * balancing nature of skip lists.
 */
#pragma once
#include "skiplist.h"

/**
 *This class is a sparse-index that uses a skip list to store key-offset pairs.
 * 
 */

class sparseindex {
    private:
        skiplist index;
    
    public:
        /**
         *Construct a new sparseindex object
         * 
         */
        sparseindex() : index(20, 0.5) {}
        
        /**
         *Function to insert a key-offset pair into the skiplist
         * 
         * @param key The key in the key-offset pair
         * @param offset The offset in the key-offset pair
         */
        void insert(const std::string& key, std::streampos offset) {
            index.insert(key, offset);
        }
        
        /**
         *Function to get the nearest offset for a given key
         * 
         * @param key The key for which the offset is to be found
         * @return std::streampos The offset for the given key
         */
        std::streampos get_offset(const std::string& key) {
            return index.get_nearest_offset(key);
        }
    };
    