/**
 * @file blinkdb.h
 *Defines the blinkdb class, a key-value store with in-memory and disk storage.
 *
 * @details
 * This file introduces the blinkdb class which implements a key-value store. The design uses an 
 * in-memory hashtable (with open addressing and double hashing) to store key-value pairs. When the 
 * hashtable becomes full, it evicts entries to disk storage and leverages a sparse index for file 
 * lookups. A bloom filter is used to optimize read operations by quickly ruling out non-existent keys.
 *
 * The class provides basic operations:
 * - set: Insert a key-value pair into the store.
 * - get: Retrieve the value corresponding to a given key, fetching from memory first, then disk if needed.
 * - del: Delete a key-value pair from the in-memory store.
 *
 * The design ensures that when the user exits, all stored data, both in memory and on disk, are cleared.
 *
 * @note This file is part of the design-lab project and is located in the src/lib directory.
 */
#pragma once
#include "hashtable.h"

/**
 *This is the blinkdb class. It is a key-value store that lets user to set, get and delete key-value pairs
 * It uses a hashtable to store the key-value pairs. The hashtable uses open addressing with double hashing to resolve collisions.
 * If the in-memory hashtable is full, the key-value pairs are stored in a file on disk.
 * When the user exits, all the data is erased from the disk and from the memory (obviously).
 * 
 */
class blinkdb {
    private:
        hashtable ht;
        diskstorage ds;
        bloomfilter bloomFilter;
        sparseindex sparseIndex;
    public:
        /**
         *Construct a new blinkdb object
         * 
         */
        blinkdb(size_t size) : ht(size) {
            bloomFilter = bloomfilter(size, 8);
        }

        /**
         *Function to set a key-value pair
         * 
         * @param key The key in the key-value pair
         * @param value The value in the key-value pair
         * @return **true** when the key-value pair is set successfully
         * @return **false** otherwise
         */
        bool set(std::string key, std::string value){
            if (ht.insert(key, value))
                return true;
            ht.evict(ds, sparseIndex, bloomFilter);
            return ht.insert(key, value);
        }

        /**
         *Function to get value correspoding to a key
         * 
         * @param key The key whose value you want to retrieve
         * @return std::string 
         */
        std::string get(std::string key){
            std::string val = ht.get(key);
            if(val != "")
                return val;
            if(!bloomFilter.possibly_contains(key))
                return "";
            /**
             * Need to go to the disk If still not found,
             * then the key does not exist and we return an empty string
             * */
            return ds.read(key, sparseIndex);
        }

        /**
         *Function to delete a key-value pair
         * 
         * @param key The key that you want to delete
         * @return **true** when the key-value pair is deleted successfully
         * @return **false** otherwise
         */
        bool del(std::string key){
            return ht.del(key);
        }

        void print(){
            ht.print();
        }
};