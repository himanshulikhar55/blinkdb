#pragma once
#include "memorypool.h"
#include <vector>
#include <string>
#include <atomic>

/**
 * @brief This is the custom hashtable class. It uses open addressing with double hashing
 *        to resolve collisions. The table is a vector of Entry structs. Each Entry struct has a key,
 *        a value and a boolean flag to indicate if the entry is occupied.
 *        The occupied variable is a std::atomic<bool> to ensure that the operations are atomic.
 */

class hashtable {
    private:
        struct Entry {
            std::atomic<bool> occupied;
            std::string key;
            Block* value;
        };
    
        std::vector<Entry> table;
        size_t capacity;
        memorypool memoryPool;

        // first hash function
        size_t hash1(const std::string& key) {
            std::hash<std::string> hasher;
            return hasher(key) % capacity;
        }
        
        // second hash function. Should never return 0
        size_t hash2(const std::string& key) {
            std::hash<std::string> hasher;
            return (hasher(key) % (capacity - 1)) + 1;
        }
    
    public:
        /**
         * @brief Construct a new hashtable object
         * 
         * @param size Size of the hashtable
         */
        hashtable(size_t size) : capacity(size), table(size), memoryPool(size) {}
        
        /**
         * @brief Construct a new hashtable object. This is the default constructor
         * 
         */
        hashtable() : capacity(0), table(0), memoryPool(0) {}

        /**
         * @brief Function to insert a key-value pair into the hashtable
         * 
         * @param key The key in the key-value pair
         * @param value The value in the key-value pair
         * @return true, when the key-value pair is inserted successfully
         * @return false, otherwise
         */
        bool insert(const std::string& key, std::string& value) {
            size_t index = hash1(key);
            size_t step = hash2(key);

            for (size_t i = 0; i < capacity; ++i) {
                bool expected = false;
                size_t pos = (index + i * step) % capacity;
                if (table[pos].occupied.compare_exchange_strong(expected, true)) {
                    table[pos].key = key;
                    table[pos].value = memoryPool.allocate(key, std::move(value));
                    return true;
                }
            }
            return false;
        }

        /**
         * @brief Function to get the value corresponding to a key
         * 
         * @param key The key whose value you want to retrieve
         * @return std::string Returns the value corresponding to the key. If the key does not exist, it returns an empty string
         */
        std::string get(const std::string& key) {
            size_t index = hash1(key);
            size_t step = hash2(key);
    
            for (size_t i = 0; i < capacity; ++i) {
                size_t pos = (index + i * step) % capacity;
                if (table[pos].occupied.load() && table[pos].key == key) {
                    return table[pos].value->value;
                }
            }
            return "";
        }
        
        /**
         * @brief Function to delete a key-value pair from the hashtable
         * 
         * @param key The key that you want to delete
         * @return true, if the key-value pair is deleted successfully
         * @return false, otherwise
         */
        bool del(const std::string& key) {
            size_t index = hash1(key);
            size_t step = hash2(key);
    
            for (size_t i = 0; i < capacity; ++i) {
                size_t pos = (index + i * step) % capacity;
                bool expected = false;
                if (table[pos].key == key && table[pos].occupied.compare_exchange_strong(expected, true)) {
                    memoryPool.deallocate(table[pos].value);
                    table[pos].occupied.store(false);
                    return true;
                }
            }
            return false;
        }
        void print(){
            for(size_t i = 0; i < capacity; i++){
                if(table[i].occupied.load()){
                    std::cout << table[i].key << " " << table[i].value->value << std::endl;
                }
            }
        }
    };