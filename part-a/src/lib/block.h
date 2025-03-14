#pragma once
#include <string>
#include <atomic>

/**
 * @brief This is the actual "Block" that will store the key and value pair.
 * This block will be dynamically allocated and deallocated during "set" and "delete" operations in blinkdb.
 * 
 */
struct Block {
    /**
     * @brief Tells if the block is used or not
     * 
     */
    std::atomic<bool> used;
    /**
     * @brief The key in the key-value pair
     * 
     */
    std::string key;
    /**
     * @brief The value in the key-value pair
     * 
     */
    std::string value;
};