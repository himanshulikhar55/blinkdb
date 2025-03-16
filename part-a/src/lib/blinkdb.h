#pragma once
#include "hashtable.h"

/**
 * @brief This is the blinkdb class. It is a key-value store that lets user to set, get and delete key-value pairs
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
         * @brief Construct a new blinkdb object
         * 
         */
        blinkdb(size_t size) : ht(size) {
            bloomFilter = bloomfilter(size, 6);
        }

        /**
         * @brief Function to set a key-value pair
         * 
         * @param key The key in the key-value pair
         * @param value The value in the key-value pair
         * @return true when the key-value pair is set successfully
         * @return false otherwise
         */
        bool set(std::string key, std::string value){
            if (ht.insert(key, value))
                return true;
            ht.evict(ds, sparseIndex, bloomFilter);
            return ht.insert(key, value);
        }

        /**
         * @brief Function to get value correspoding to a key
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
            // Need to go to the disk
            // If still not found, then the key does not exist and we return an empty string
            return ds.read(key, sparseIndex);
        }

        /**
         * @brief Function to delete a key-value pair
         * 
         * @param key The key that you want to delete
         * @return true when the key-value pair is deleted successfully
         * @return false otherwise
         */
        bool del(std::string key){
            return ht.del(key);
        }

        void print(){
            ht.print();
        }
};