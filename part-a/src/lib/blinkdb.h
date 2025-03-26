/**
 * @file blinkdb.h
 * Defines the blinkdb class, a key-value store with in-memory and disk storage.
 *
 * @details
 * This file introduces the blinkdb class which implements an in-memory key-value store database. The design uses an 
 * in-memory hashtable (with open addressing and double hashing) to store key-value pairs. When the 
 * hashtable becomes 70% full, it checks if its size is greater than 1GB. If no, it resizes and rehashes the existing
 * eentries. Else, it evicts 50% of entries to disk storage. It leverages a sparse index for file 
 * lookups. A bloom filter is used to optimize read operations by quickly ruling out non-existent keys and saving a
 * disk-access.
 *
 * The class provides basic operations:
 * - SET: Insert a key-value pair into the store.
 * - GET: Retrieve the value corresponding to a given key, fetching from memory first, then disk if needed.
 * - DEL: Delete a key-value pair from the in-memory store.
 * - print: Print the contents of the in-memory store.
 *
 * The design ensures that when the user exits, all stored data, both in memory and on disk, are cleared (obviously).
 * 
 * @see hashtable.h
 * 
 * @copyright Copyright (c) 2025
 */

 /**
 * @mainpage BlinkDB
 * @note BlinkDB is a project developed for educational purposes and is not intended for production use.
 * @section overview Overview
 *
 * BlinkDB is an efficient **in-memory key-value store** that leverages both in-memory and on-disk storage to provide quick data access 
 * and high write speeds. The project employs an in-memory hashtable alongside disk storage for evicted entries (when the main memory
 * starts to fill up), ensuring scalability when the dataset grows large. Additionally, it integrates a bloom filter and a sparse
 * index to optimize read operations and disk lookups.
 *
 * @section architecture Architecture
 *
 * The architecture of BlinkDB consists of:
 * - In-Memory Hashtable: Utilizes open addressing with double hashing to store key-value pairs until the load factor reaches 70%.
 * - Resizing and Rehashing: When feasible (i.e., if memory usage is below 1GB), the hashtable is resized and rehashed to 
 *   accommodate more entries.
 * - Disk Storage Eviction: If resizing is not possible, 50% of the in-memory entries are evicted to disk storage to 
 *   free up memory.
 * - Disk Storage: Maintains evicted entries and is optimized using a sparse index for faster file lookups.
 * - Bloom Filter: Quickly rules out non-existent keys to reduce unnecessary disk accesses during read operations.
 *
 * @section operations Operations
 *
 * BlinkDB provides the following core functionalities:
 * - SET: Insert or update a key-value pair in the store.
 * - GET: Retrieve the value associated with a given key, first by checking in-memory storage, then, if needed, 
 *   falling back to disk storage.
 * - DEL: Delete a key-value pair from the in-memory store.
 * - print: Output the current contents of the in-memory store to the console.
 *
 * @section cleanup Cleanup
 *
 * Upon termination, BlinkDB **ensures** that both in-memory and disk data are cleared, maintaining data integrity and resource 
 * management. This was achieved after much deliberation and hours of testing using Valgrind and GDB.
 *
 * @section author Author
 *
 * Himanshu Likhar
 *
 * @section version Version
 *
 * 0.1
 *
 * @section date Date
 *
 * 2025-03-27
 */

 #pragma once
#include "hashtable.h"

/**
 * @class blinkdb
 * This is the blinkdb class. It is a key-value store that lets user to set, get and delete key-value pairs
 * It uses a hashtable to store the key-value pairs. When the 
 * hashtable becomes 70% full, it checks if its size is greater than 1GB. If no, it resizes and rehashes the existing
 * eentries. Else, it evicts 50% of entries to disk storage. It leverages a sparse index for file 
 * lookups. A bloom filter is used to optimize read operations by quickly ruling out non-existent keys and saving a
 * disk-access.
 */
class blinkdb {
    private:
        hashtable ht;             /* The hashtable to store key-value pairs */
        diskstorage ds;           /* The disk storage to store key-value pairs */
        bloomfilter bloomFilter;  /* The bloom filter to optimize read operations */
        sparseindex sparseIndex;  /* The sparse index to optimize disk lookups */
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
         *@brief This function should be called when we want to delete a key-value pair
         * 
         * @param key The key that you want to delete
         * @return **true** when the key-value pair is deleted successfully
         * @return **false** otherwise
         */
        bool del(std::string key){
            return ht.del(key);
        }

        /**
         * @brief Prints the contents of the database.
         * 
         */
        void print(){
            ht.print();
        }
};