#include <vector>
#include <atomic>
#include <functional>
#include <string>

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
    bloomfilter(size_t num_bits, size_t num_hashes)
        : size(num_bits), hash_count(num_hashes), bit_array((num_bits + 63) / 64) {}

    void insert(const std::string& key) {
        auto [hash1, hash2] = hash(key);
        for (size_t i = 0; i < hash_count; ++i) {
            size_t index = (hash1 + i * hash2) % size;
            bit_array[index / 64].fetch_or(1ULL << (index % 64), std::memory_order_relaxed);
        }
    }

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
