/**
 * @file memorypool.h
 *Implements a memory pool for managing Block objects.
 *
 * This file defines the memorypool class, which is responsible for allocating and deallocating memory blocks,
 * each represented by a Block object that can store key-value pairs. The memory pool maintains a fixed-size vector
 * of Blocks and uses a queue to track available (unused) blocks.
 *
 * The allocation function assigns a key-value pair to the first available block and marks it as used,
 * ensuring efficient memory usage. The deallocation function returns a block to the pool for future reuse.
 *
 * This design is intended for scenarios where managing a collection of key-value pairs in memory
 * is crucial for performance and resource utilization.
 *
 * @note The memorypool relies on the Block class declared in "block.h", and it uses C++ Standard Library
 * facilities like std::vector and std::queue.
 */
#pragma once
#include "block.h"
#include "debug.h"
#include <iostream>
#include <vector>

#include <memory>
#include <queue>

/**
 *This class is a memory pool that is used to allocate and deallocate memory blocks that will store key-value pairs
 * 
 */

class memorypool {
private:
    std::vector<Block> pool;
    size_t capacity;
    std::queue<int> free_blocks;
    const static size_t INVALID_INDEX = -1;
    void _resize_(size_t new_size) {
        std::vector<Block> new_pool(new_size);
        std::move(pool.begin(), pool.end(), new_pool.begin());
        pool.swap(new_pool); /* Swap to avoid old memory getting freed unexpectedly */
        
        for (size_t i = capacity; i < new_size; i++) {
            free_blocks.push(i);
        }
        capacity = new_size;
    }

public:
    /**
     *Construct a new memorypool object with a given size
     * 
     * @param size The number of "Blocks" that the memory pool can store in-memory
     */
    memorypool(size_t size) : capacity(size) {
        free_blocks = std::queue<int>();
        for (size_t i = 0; i < size; i++) {
            free_blocks.push(i);
        }
        pool = std::vector<Block>(size);
    }

    ~memorypool() {
        pool.clear();
        while (!free_blocks.empty()) {
            free_blocks.pop();
        }
    }

    /**
     *Default constructor for a new memorypool object
     * 
     */
    memorypool(): capacity(0), pool(0) {}
    
    /**
     *When "set" is called, a new Block is allocated from the memory pool and the key-value pair is stored in the Block
     * 
     * @param value The value in the key-value pair
     * @return Block* A pointer to the Block that stores the key-value pair
     */
    size_t allocate(std::string& value) {
        while (!free_blocks.empty()) {
            int index = free_blocks.front();
            free_blocks.pop();
            Block& block = pool[index];
            if (!block.used) {
                DEBUG_PRINT2("Block used: ",  index);
                block.used = true;
                block.value = value;
                DEBUG_PRINT2("Value set: ",  block.value);
                return index;
            }
        }
        return -1;
    }
    
    /**
     *When "del" is called, the Block that stores the key-value pair is deallocated from the memory pool
     * 
     * @param block Pointer the Block that stores the key-value pair
     * @return **true** When the Block is deallocated successfully
     * @return **false** Otherwise
     */
    bool deallocate(size_t index) {
        if(index != INVALID_INDEX){
            free_blocks.push(index);
            pool[index].used = false;
            pool[index].value.clear();
            return true;
        }
        return false;
    }

    /**
     *Resizes the memory pool to a new size
     * 
     * @param new_size The new size of memory pool
     */
    void resize(size_t new_size){
        _resize_(new_size);
    }

    /**
     *Sets the value of a Block at a given index
     * 
     * @param index The index at which you want to store the value
     * @param value The value that you want to store at that index
     */
    void set_value(size_t index, std::string& value){
        DEBUG_PRINT2("Value before: ", pool[index].value.c_str());
        Block& block = pool[index];
        block.value = value;
        DEBUG_PRINT2("Value set: ", pool[index].value.c_str());
    }

    /**
     *Get the value of a Block at a given index
     * 
     * @param index The index at which you want to get the value
     * @return std::string The value of at the index
     */
    std::string get_value(size_t index){
        return pool[index].value;
    }
};