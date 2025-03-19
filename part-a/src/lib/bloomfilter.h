
/**
 * @file bloomfilter.h
 * @brief Implements a space-efficient probabilistic Bloom filter.
 *
 * This file defines the bloomfilter class which provides a space-efficient
 * data structure to test whether a key is possibly present in a dataset.
 *
 * A Bloom filter is characterized by the following parameters:
 * - \f$n\f$: Expected number of elements to be inserted.
 * - \f$m\f$: Size of the bit array (in bits).
 * - \f$k\f$: Number of hash functions.
 *
 * The optimal parameters for a desired false positive probability \f$p\f$ are calculated as follows:
 *
 * 1. **Bit Array Size**:
 *    \f[
 *    m = - \frac{n \ln(p)}{(\ln(2))^2}
 *    \f]
 *
 * 2. **Number of Hash Functions**:
 *    \f[
 *    k = \frac{m}{n} \ln(2)
 *    \f]
 *
 * These formulas help minimize the false positive rate while using space efficiently.
 *
 * The implementation combines two hash functions to simulate k hash functions
 * using the formula: hash_i = hash1 + i * hash2, and uses atomic operations to
 * maintain the correctness of the bit array during concurrent insertions.
 *
 * This file is part of the design-lab project and is intended for quickly checking
 * key membership with a controlled false positive rate.
 */
#include <vector>
#include <atomic>
#include <functional>
#include <string>

/**
 * @brief Bloom filter class documentation.
 *
 * This Bloom filter is used to quickly test whether a key is possibly present in the dataset.
 * It provides a space-efficient probabilistic method with a configurable false positive rate.
 *
 */

class bloomfilter {
private:
    std::vector<std::atomic<uint64_t>> bit_array;
    size_t size;
    size_t hash_count;

    std::pair<size_t, size_t> hash(const std::string& key) const {
        std::hash<std::string> hasher;
        size_t hash1 = hasher(key);
        size_t hash2 = hash1 >> 3; // Second hash (cheap variation)
        return {hash1, hash2};
    }

public:
    /**
     * @brief Construct a new bloomfilter object
     * 
     * @param num_bits Size of the bit array (in bits)
     * @param num_hashes Number of hash functions
     */
    bloomfilter(size_t num_bits, size_t num_hashes)
        : size(num_bits), hash_count(num_hashes), bit_array((num_bits + 63) / 64) {}
    bloomfilter() : bloomfilter(0, 0) {}

    /**
     * @brief Function to insert a key into the bloomfilter. This function is only called when the key-value pair is being flushed to the disk
     * 
     * @param key The key to insert
     */
    void insert(const std::string& key) {
        auto [hash1, hash2] = hash(key);
        for (size_t i = 0; i < hash_count; ++i) {
            size_t index = (hash1 + i * hash2) % size;
            bit_array[index / 64].fetch_or(1ULL << (index % 64), std::memory_order_relaxed);
        }
    }

    /**
     * @brief Function to check if a key is possibly present in the bloomfilter
     * 
     * @param key The key to check
     * @return true If the key is possibly present
     * @return false Otherwise
     */
    bool possibly_contains(const std::string& key) const {
        auto [hash1, hash2] = hash(key);
        for (size_t i = 0; i < hash_count; ++i) {
            size_t index = (hash1 + i * hash2) % size;
            if (!(bit_array[index / 64].load(std::memory_order_relaxed) & (1ULL << (index % 64)))) {
                return false;
            }
        }
        return true;
    }
};
