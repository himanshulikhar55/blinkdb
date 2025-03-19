
/**
 * @file skiplist.h
 * @brief Implementation of a concurrent skiplist data structure.
 *
 * This file provides the implementation of a skiplist data structure using C++.
 * The skiplist is designed to support efficient insertion and search operations,
 * leveraging probabilistic balancing and atomic operations for thread safety.
 *
 * The data structure consists of:
 * - A nested Node struct, representing each node in the skiplist, which holds a key,
 *   a stream offset, and a vector of atomic pointers to subsequent nodes at different levels.
 * - Member functions for inserting nodes and retrieving the nearest offset for a given key.
 *
 * The design uses a probabilistic method for promoting nodes to higher levels based on a
 * user-defined probability, ensuring balanced performance across operations.
 *
 * Typical usage example:
 * @code
 * skiplist sl;
 * sl.insert("example_key", 42);
 * std::streampos pos = sl.get_nearest_offset("another_key");
 * @endcode
 *
 * @note This implementation showcases concurrency control using std::atomic and assumes
 * that the environment supports C++11 or later.
 *
 * @author
 * @date
 */
#include <iostream>
#include <atomic>
#include <vector>
#include <random>
#include <limits>
#include <string>

class skiplist {
private:
    struct Node {
        std::string key;
        size_t offset;
        std::vector<std::atomic<Node*>> next;

        Node(std::string k, size_t off, int level)
            : key(k), offset(off), next(level) {  // Initialize vector with `level` elements
            for (size_t i = 0; i < level; ++i) {
                next[i].store(nullptr, std::memory_order_relaxed);  // Properly initialize atomics
            }
        }
    };

    std::atomic<Node*> head;
    int maxLevel;
    float probability;

    int randomLevel() {
        int level = 1;
        while (((float)rand() / RAND_MAX) < probability && level < maxLevel) {
            level++;
        }
        return level;
    }

public:
    /**
     * @brief Constructs a new skiplist object.
     *
     * Initializes the skiplist with a specified maximum level and probability.
     * A header node is created with an empty key and a placeholder value (-1)
     * using the provided maximum level.
     *
     * @param maxLvl The maximum level allowed in the skiplist (default is 16). This cannot be changed currently.
     * @param prob The probability used for level promotion of nodes (default is 0.5). This cannot be changed currently.
     */
    skiplist(int maxLvl = 16, float prob = 0.5)
        : maxLevel(maxLvl), probability(prob) {
        head.store(new Node("", -1, maxLevel));
    }

    skiplist(){}

    /**
     * @brief Inserts a new node with the given key and stream offset into the skip list.
     *
     * This function navigates the skip list from the highest level down to level 0 to find
     * the appropriate insertion point for the new node. It maintains arrays of predecessor
     * and successor nodes for each level during the traversal. A new random level is determined
     * for the node, and the node is inserted into each level using atomic operations to ensure
     * thread safety.
     *
     * @param key The key associated with the new node; used for maintaining order within the list.
     * @param offset The stream offset associated with the key, typically representing a position 
     *               or location in an external data source.
     */
    void insert(const std::string& key, std::streampos offset) {
        Node* preds[maxLevel], * succs[maxLevel];
        Node* pred = head.load();

        for (int level = maxLevel - 1; level >= 0; --level) {
            Node* curr = pred->next[level].load();
            while (curr && curr->key < key) {
                pred = curr;
                curr = curr->next[level].load();
            }
            preds[level] = pred;
            succs[level] = curr;
        }

        int newLevel = randomLevel();
        Node* newNode = new Node(key, offset, newLevel);

        for (int level = 0; level < newLevel; ++level) {
            do {
                newNode->next[level].store(succs[level]);
            } while (!preds[level]->next[level].compare_exchange_weak(succs[level], newNode));
        }
    }

    /**
     * @brief Retrieves the stream offset of the node immediately preceding the given key.
     *
     * This function traverses the skip list from the top-most level down to the lowest level,
     * moving forward at each level until it finds the last node whose key is less than the specified key.
     * The function then returns the offset of this node within the stream.
     *
     * @param key The key to search for in the skip list.
     *
     * @return std::streampos The offset of the node with the largest key that is less than the input key.
     *
     * @note If no node with a key less than the provided key exists, the offset of the head node is returned.
     */
    std::streampos get_nearest_offset(const std::string& key) {
        Node* pred = head.load();

        for (int level = maxLevel - 1; level >= 0; --level) {
            Node* curr = pred->next[level].load();
            while (curr && curr->key < key) {
                pred = curr;
                curr = curr->next[level].load();
            }
        }
        return pred->offset;
    }
};
