#pragma once
#include "memorypool.h"
#include "diskstorage.h"
#include "bloomfilter.h"
#include "sparseindex.h"
#include <vector>
#include <string>
#include <thread>
#include <chrono>
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

            // Default constructor
            Entry() : occupied(false), key(""), value(nullptr) {}

            // Move constructor
            Entry(Entry&& other) noexcept 
                : key(std::move(other.key)), value(other.value) {
                occupied.store(other.occupied.load()); 
                other.value = nullptr; 
            }

            // Move assignment operator
            Entry& operator=(Entry&& other) noexcept {
                if (this != &other) {
                    occupied.store(other.occupied.load());
                    key = std::move(other.key);
                    value = other.value;
                    other.value = nullptr;
                }
                return *this;
            }

            // Disable copying
            Entry(const Entry&) = delete;
            Entry& operator=(const Entry&) = delete;
        };
        
        std::vector<Entry> table;
        size_t capacity;
        memorypool memoryPool;

        std::atomic<size_t> count;
        std::atomic<bool> resizing;
        double resizeThreshold;
        std::thread monitorThread;
        std::atomic<bool> stopThread;

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

        void monitorMemory() {
            while (!stopThread) {
                std::this_thread::sleep_for(std::chrono::seconds(5)); // Check every 5 seconds
                if (shouldResize()) {
                    resize();
                }
            }
        }
    
        bool shouldResize() {
            return ((double)count.load() / capacity) > resizeThreshold;
        }
    
        void resize() {
            if (resizing.exchange(true))
                return;
            
            size_t newCapacity = capacity * 2;
            std::vector<Entry> newTable(newCapacity);
            
            for (size_t i=0;i<capacity;i++) {
                Entry& entry = table[i];
                if (entry.occupied.load()) {
                    size_t index = hash1(entry.key) % newCapacity;
                    size_t step = hash2(entry.key);
                    for (size_t i = 0; i < newCapacity; ++i) {
                        size_t pos = (index + i * step) % newCapacity;
                        if (!newTable[pos].occupied.load()) {
                            newTable[pos] = std::move(entry);
                            newTable[pos].occupied.store(true);
                            break;
                        }
                    }
                }
            }
            table = std::move(newTable);
            capacity = newCapacity;
            resizing.store(false);
        }
    
    public:
        /**
         * @brief Construct a new hashtable object
         * 
         * @param size Size of the hashtable
         */
        hashtable(size_t size, double threshold = 0.70) 
            : capacity(size), table(size), memoryPool(size), count(0), resizeThreshold(threshold), resizing(false), stopThread(false) {
            monitorThread = std::thread(&hashtable::monitorMemory, this);            
        }
        
        ~hashtable() {
            stopThread.store(true);
            if (monitorThread.joinable()) monitorThread.join();
        }

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

            if ((double)count / capacity > 0.75) { // Resize condition
                resize();
            }
    
            size_t index = hash1(key);
            size_t step = hash2(key);

            for (size_t i = 0; i < capacity; ++i) {
                bool expected = false;
                size_t pos = (index + i * step) % capacity;

                if(table[pos].occupied.load() && table[pos].key == key){
                    table[pos].value->value = std::move(value);
                    return true;
                }

                if (table[pos].occupied.compare_exchange_strong(expected, true)) {
                    table[pos].key = key;
                    table[pos].value = memoryPool.allocate(std::move(value));
                    count++;
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
                bool expected = true;
                if (table[pos].key == key && table[pos].occupied.compare_exchange_strong(expected, true)) {
                    memoryPool.deallocate(table[pos].value);
                    table[pos].occupied.store(false);
                    count--;
                    return true;
                }
            }
            return false;
        }

        void evict(diskstorage& ds, sparseindex& sparseIndex, bloomfilter& bloomFilter) {
            for (size_t i = 0; i < capacity; ++i) {
                if (table[i].occupied.load()) {
                    std::string key = table[i].key;
                    std::string value = table[i].value->value;
                    
                    // Write to disk
                    ds.write(key, value, sparseIndex);
                    bloomFilter.insert(key);
                    // Mark the entry as empty
                    table[i].occupied.store(false);
                }
            }
            count = 0;  // Reset the count since all entries are evicted
        }

        void print(){
            for(size_t i = 0; i < capacity; i++){
                if(table[i].occupied.load()){
                    std::cout << table[i].key << " " << table[i].value->value << std::endl;
                }
            }
        }
    };