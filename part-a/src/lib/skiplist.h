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
    skiplist(int maxLvl = 16, float prob = 0.5)
        : maxLevel(maxLvl), probability(prob) {
        head.store(new Node("", -1, maxLevel));
    }

    skiplist(){}

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
