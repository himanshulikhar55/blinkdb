
/**
 * @file hashtable.h
 * @brief Custom Hashtable implementation using open addressing with double hashing.
 *
 * This file contains the declaration and implementation of a custom hashtable that
 * employs open addressing with double hashing for collision resolution. Each entry in
 * the hashtable is represented by an Entry struct containing an atomic occupancy flag,
 * a key string, and a pointer to a memory-managed Block. The hashtable integrates several
 * components including:
 *   - A custom memory pool for managing dynamic memory allocation.
 *   - Disk storage for offloading key-value pairs via the evict() function.
 *   - A sparse index for maintaining and updating disk entries.
 *   - A bloom filter to provide efficient approximate membership tests.
 *
 * Key features include:
 *   - Atomic operations ensuring thread-safety in the manipulation of table entries.
 *   - Dynamic resizing of the table based on occupancy thresholds, monitored by a dedicated
 *     background thread.
 *   - Standard hashtable operations such as insert, get, del, and evict.
 *
 * The implementation is designed to efficiently handle memory and disk storage operations,
 * making use of modern C++ features such as move semantics, std::atomic, and multithreading.
 */
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

            /* Default constructor */
            Entry() : occupied(false), key(""), value(nullptr) {}

            /* Move constructor */
            Entry(Entry&& other) noexcept 
                : key(std::move(other.key)), value(other.value) {
                occupied.store(other.occupied.load()); 
                other.value = nullptr; 
            }

            /* Move assignment operator */
            Entry& operator=(Entry&& other) noexcept {
                if (this != &other) {
                    occupied.store(other.occupied.load());
                    key = std::move(other.key);
                    value = other.value;
                    other.value = nullptr;
                }
                return *this;
            }

            /* Disable copying */
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
         * @param threshold Threshold for resizing the hashtable. Currently this cannot be changed
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

        /**
         * @brief Evicts all occupied entries from the hash table and offloads them to disk.
         *
         * This function iterates over each slot in the table and, for every occupied entry:
         * - Retrieves the key and its associated value.
         * - Writes the key-value pair to disk via the provided disk storage, updating the sparse index.
         * - Inserts the key into the bloom filter.
         * - Marks the entry as unoccupied.
         *
         * After processing all entries, the count of occupied entries is reset to zero.
         *
         * @param ds Reference to a diskstorage object used to perform disk write operations.
         * @param sparseIndex Reference to a sparseindex object managing disk entry indexing.
         * @param bloomFilter Reference to a bloomfilter object used for tracking key presence.
         */
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