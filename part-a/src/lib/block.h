/**
 * @file block.h
 * @brief This is the actual "Block" that will store the key and value pair.
 * @copyright Copyright (c) 2025
 * 
 */
#pragma once
#include <string>


/**
 * This is the actual "Block" that will store the key and value pair.
 * This block will be dynamically "allocated" and "deallocated" during "set" and "delete" operations in blinkdb.
 * 
 */
struct Block {

    bool used;         /* Tells if the block is used or not */
    std::string value; /* The value in the key-value pair */

    /**
     * @brief Default constructor to create a new Block object
     * 
     */
    Block() {
        used = false;
        value = "";
    }

    /* Move constructor */
    Block(Block&& other) noexcept
        : value(std::move(other.value)), used(other.used) {
        other.used = false;  /* Prevent dangling pointer */
        other.value.clear();
    }

    /* Move assignment operator */
    Block& operator=(Block&& other) noexcept {
        if (this != &other) {
            value = std::move(other.value);
            used = other.used;

            other.value.clear();
            other.used = false;
        }
        return *this;
    }

    Block(const Block& other) : used(other.used), value(other.value) {}

    Block& operator=(const Block &other) noexcept {
        if(this != &other) {
            used = other.used;
            value = other.value;
        }
        return *this;
    }
};