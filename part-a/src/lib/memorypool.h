#pragma once
#include "block.h"
#include <iostream>
#include <vector>
#include <atomic>
#include <memory>
#include <queue>

/**
 * @brief This class is a memory pool that is used to allocate and deallocate memory blocks that will store key-value pairs
 * 
 */

class memorypool {
private:
    std::vector<Block> pool;
    size_t capacity;
    std::queue<int> free_blocks;
    
public:
    /**
     * @brief Construct a new memorypool object with a given size
     * 
     * @param size The number of "Blocks" that the memory pool can store in-memory
     */
    memorypool(size_t size) : capacity(size), pool(size) {
        free_blocks = std::queue<int>();
        for (int i = 0; i < size; i++) {
            free_blocks.push(i);
        }
    }

    /**
     * @brief Default constructor for a new memorypool object
     * 
     */
    memorypool(): capacity(0), pool(0) {}
    
    /**
     * @brief When "set" is called, a new Block is allocated from the memory pool and the key-value pair is stored in the Block
     * 
     * @param value The value in the key-value pair
     * @return Block* A pointer to the Block that stores the key-value pair
     */
    Block* allocate(std::string&& value) {
        while (!free_blocks.empty()) {
            int index = free_blocks.front();
            free_blocks.pop();
            Block& block = pool[index];
            if (!block.used.load()) {
                block.used.store(true);
                block.value = std::move(value);
                return &block;
            }
        }
        return nullptr;
    }
    
    /**
     * @brief When "del" is called, the Block that stores the key-value pair is deallocated from the memory pool
     * 
     * @param block Pointer the Block that stores the key-value pair
     * @return true When the Block is deallocated successfully
     * @return false Otherwise
     */
    bool deallocate(Block* block) {
        if(block){
            size_t index = block - &pool[0];
            free_blocks.push(index);
            block->used = false;
            return true;
        }
        return false;
    }
};