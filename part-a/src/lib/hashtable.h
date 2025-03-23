
/**
 * @file hashtable.h
 *Custom Hashtable implementation using open addressing with double hashing.
 *
 * This file contains the declaration and implementation of a custom hashtable that
 * employs open addressing with double hashing for collision resolution. Each entry in
 * the hashtable is represented by an Entry struct containing an occupancy flag,
 * a key string, and a pointer to a memory-managed Block. The hashtable integrates several
 * components including:
 *   - A custom memory pool for managing dynamic memory allocation.
 *   - Disk storage for offloading key-value pairs via the evict() function.
 *   - A sparse index for maintaining and updating disk entries.
 *   - A bloom filter to provide efficient approximate membership tests.
 *
 * Key features include:
 *   - Dynamic resizing of the table based on occupancy thresholds, monitored by a dedicated
 *     background thread.
 *   - Standard hashtable operations such as insert, get, del, and evict.
 *
 * The implementation is designed to efficiently handle memory and disk storage operations,
 * making use of modern C++ features such as move semantic and using thread to handle resizing.
 */
#pragma once
#include "memorypool.h"
#include "diskstorage.h"
#include "bloomfilter.h"
#include "sparseindex.h"
#include "debug.h"

#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <utility>
#include <fstream>
#include <sstream>

#define MAX_PROCESS_MEMORY 1000000 /* 1GB */
#define DEBUG_FILE "debug.log"

/**
 *This is the custom hashtable class. It uses open addressing with double hashing
 *        to resolve collisions. The table is a vector of Entry structs. Each Entry struct has a key,
 *        a value and a boolean flag to indicate if the entry is occupied.
 */

class hashtable {
    private:
        struct Entry {
            bool occupied;
            std::string key;
            size_t index;

            /* Default constructor */
            Entry() : occupied(false), key(""), index(-1) {}

            /* Move constructor */
            Entry(Entry&& other) noexcept
                : key(std::move(other.key)), index(std::move(other.index)), occupied(other.occupied) {
                other.index = -1;  /* Prevent dangling pointer */
                other.occupied = false;
                other.key.clear();
            }

            /* Move assignment operator */
            Entry& operator=(Entry&& other) noexcept {
                if (this != &other) {
                    key = std::move(other.key);
                    index = std::move(other.index);
                    occupied = other.occupied;

                    other.index = -1;
                    other.occupied = false;
                }
                return *this;
            }
        };
        
        std::vector<Entry> table; /* Structure to store the key-value pair and to tell if it is occupied or not */
        memorypool memory_pool;   /* Memory pool to manage dynamic memory allocation */

        double threshold;         /* Threshold fraction for resizing the hashtable */

        std::ofstream fout;       /* Debug file */

        size_t count;             /* Number of occupied entries in the hashtable */
        size_t capacity;          /* Current capacity of the hashtable */
        size_t interval;          /* Interval after which to check the size of the hastable */
        size_t resize_threshold;  /* Threshold after which to resize the hashtable */

        const static size_t INVALID_INDEX = -1;     /* Invalid index for the memory pool */
        bool resizing;            /* Flag to indicate if the hashtable is being resized */
        bool beingEvicted;        /* Flag to indicate if the hashtable is being evicted */
        bool stopThread;          /* Flag to stop the background thread */

        /* first hash function */
        size_t hash1(const std::string& key) {
            std::hash<std::string> hasher;
            return hasher(key) % capacity;
        }
        
        /* second hash function. Should never return 0 */
        size_t hash2(const std::string& key) {
            std::hash<std::string> hasher;
            return (hasher(key) % (capacity - 1)) + 1;
        }

        /* Tells if the hash table should be evicted to the disk */
        bool should_evict() {
            if(get_proc_mem_usage() > MAX_PROCESS_MEMORY) {
                std::cout << "Memory usage exceeded threshold. Should evict table to disk\n";
                return true;
            }
            return false;
        }
        
        /* Computes the new hash when rehashing the table */
        size_t rehash(const std::string& key, size_t new_capacity, std::vector<Entry>& new_table) {
            size_t index = hash1(key) % new_capacity;  /* Primary hash */
            size_t step = hash2(key) % new_capacity;   /* Step size for probing */
        
            /* Open addressing (linear probing with double hashing) */
            for (size_t i = 0; i < new_capacity; ++i) {
                size_t pos = (index + i * step) % new_capacity;  
                if (!new_table[pos].occupied) {
                    return pos;  /* Return first available slot */
                }
            }
            
            throw std::runtime_error("Rehashing failed: No free slot found");
        }
        
        /* Time to resize this chonker */
        void resize() {
            resizing = true;
            while(beingEvicted) {}
            printf("Resizing table\n");
            
            size_t newCapacity = capacity * 2;
            memory_pool.resize(newCapacity);
            
            std::vector<Entry> new_table(newCapacity);
            for (size_t i = 0; i < capacity; i++) {
                if (table[i].occupied) {
                    size_t new_index = rehash(table[i].key, newCapacity, new_table);
                    new_table[new_index].key = table[i].key;
                    std::string val = memory_pool.get_value(table[i].index);
                    new_table[new_index].index = memory_pool.allocate(val);
                    new_table[new_index].occupied = true;
                }
            }
            table.swap(new_table);
            capacity = newCapacity;
            resize_threshold = capacity * threshold;
            interval = resize_threshold / 10;
            interval = interval == 0 ? 1 : interval;
            fout << "New size of table: " << capacity << " = " << table.size() << std::endl;
            fout << "New resize threshold: " << resize_threshold << std::endl;
            fout << "New interval: " << interval << std::endl;
            fout << "Resizing done\n";
            fout.flush();
            resizing = false;
        }

        /* How much of the area has it already occupied? */
        size_t get_proc_mem_usage() {
            std::ifstream file("/proc/self/status");
            std::string line;
            size_t memory_usage = 0;
        
            while (std::getline(file, line)) {
                /* Look for "VmRSS" (Resident Set Size) */
                if (line.rfind("VmRSS:", 0) == 0) {
                    std::stringstream ss(line);
                    std::string label;
                    ss >> label >> memory_usage;
                    break;
                }
            }
            
            return memory_usage;
        }
    
    public:
        /**
         *Construct a new hashtable object
         * 
         * @param size Size of the hashtable
         * @param threshold Threshold for resizing the hashtable. Currently this cannot be changed
         */
        hashtable(size_t size, const double threshold = 0.70) 
            : capacity(size), count(0), threshold(threshold), resizing(false), beingEvicted(false) {
            table = std::vector<Entry>(size);
            memory_pool = memorypool(size);
            interval = (threshold*capacity)/10;
            interval = interval == 0 ? 1 : interval;
            resize_threshold = size * threshold;
            fout = std::ofstream(DEBUG_FILE, std::ios::out);
        }
        
        ~hashtable() {
            fout.close();
            table.clear();
        }

        /**
         *Construct a new hashtable object. This is the default constructor
         * 
         */
        hashtable() : capacity(0), table(0), memory_pool(0) {}

        /**
         *Function to insert a key-value pair into the hashtable
         * 
         * @param key The key in the key-value pair
         * @param value The value in the key-value pair
         * @return **true**, when the key-value pair is inserted successfully
         * @return **false**, otherwise
         */
        bool insert(const std::string& key, std::string& value) {
            
            while(resizing || beingEvicted){} /* Wait for resizing to finish */

            size_t index = hash1(key);
            size_t step = hash2(key);
            
            for (size_t i = 0; i < capacity; ++i) {
                
                size_t pos = (index + i * step) % capacity;

                if (table[pos].occupied && (table[pos].key == key)) {
                    DEBUG_PRINT("Duplicate key found\n");
                    if(table[pos].index == INVALID_INDEX){
                        DEBUG_PRINT2("Error: Dangling pointer detected at index ", pos);
                        table[pos].index = memory_pool.allocate(value);
                    } else {
                        DEBUG_PRINT2("Requesting to set at index: ",  table[pos].index);
                        memory_pool.set_value(table[pos].index, value);
                    }
                    return true;
                }

                if (table[pos].occupied == false) {
                    DEBUG_PRINT("Empty slot found");
                    table[pos].key = key;
                    table[pos].index = memory_pool.allocate(value);
                    table[pos].occupied = true;
                    count++;
                    if(count % interval == 0){
                        if(should_evict()){
                            std::cout << "Eviction Triggered\n";
                            return false;
                        }
                        if(count >= resize_threshold){
                            std::thread t(&hashtable::resize, this);
                            t.detach(); /* It will run until it finishes the work */
                        }
                    }
                    return true;
                }
            }
            DEBUG_PRINT2("Size is: ", table.size());
            DEBUG_PRINT2("Currently occupied: ", count);
            DEBUG_PRINT2("Could not insert: ", key);
            return false;
        }

        /**
         *Function to get the value corresponding to a key
         * 
         * @param key The key whose value you want to retrieve
         * @return std::string Returns the value corresponding to the key. If the key does not exist, it returns an empty string
         */
        std::string get(const std::string& key) {
            while(resizing || beingEvicted){} /* Wait for resizing to finish */
            size_t index = hash1(key);
            size_t step = hash2(key);
    
            for (size_t i = 0; i < capacity; ++i) {
                size_t pos = (index + i * step) % capacity;
                if (table[pos].occupied && table[pos].key == key) {
                    return memory_pool.get_value(table[pos].index);
                }
            }
            return "";
        }
        
        /**
         *Function to delete a key-value pair from the hashtable
         * 
         * @param key The key that you want to delete
         * @return **true**, if the key-value pair is deleted successfully
         * @return **false**, otherwise
         */
        bool del(const std::string& key) {
            while(resizing || beingEvicted){} /* Wait for resizing to finish */
            size_t index = hash1(key);
            size_t step = hash2(key);
    
            for (size_t i = 0; i < capacity; ++i) {
                size_t pos = (index + i * step) % capacity;
                if (table[pos].key == key && table[pos].occupied == true) {
                    memory_pool.deallocate(table[pos].index);
                    table[pos].occupied = false;
                    count--;
                    return true;
                }
            }
            return false;
        }

        /**
         *Evicts all occupied entries from the hash table and offloads them to disk.
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
            std::cout << "Evicting started\n";
            beingEvicted = true;
            int evicted_count = 0, eviction_threshold = count/2;
            for (size_t i = 0; i < capacity; ++i) {
                if (table[i].occupied && table[i].key != "") {
                    std::string key = table[i].key;
                    std::string value = memory_pool.get_value(table[i].index);
        
                    /* Write to disk */ 
                    ds.write(key, value, sparseIndex);
                    bloomFilter.insert(key);
        
                    /* Mark as unoccupied */
                    table[i].occupied = false;
        
                    if (table[i].index != INVALID_INDEX) {
                        memory_pool.deallocate(table[i].index);
                        table[i].index = INVALID_INDEX;
                    }
        
                    table[i].key.clear();
                    evicted_count++;
                    if(evicted_count >= eviction_threshold){
                        break;
                    }
                }
            }
            count -= evicted_count;
            beingEvicted = false;
            std::cout << "~50\% entries entries evicted to disk\n";
        }        

        void print(){
            for(size_t i = 0; i < capacity; i++){
                if(table[i].occupied){
                    std::string value = memory_pool.get_value(table[i].index);
                    std::cout << table[i].key << " " << value << std::endl;
                }
            }
        }
    };