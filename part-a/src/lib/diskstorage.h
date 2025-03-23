
/**
 * @file diskstorage.h
 *Provides the diskstorage class for disk-based storage of key-value pairs.
 *
 * This file defines the diskstorage class, which allows writing key-value pairs
 * to a log file in an append-only mode and reading them back by leveraging a sparse
 * indexing mechanism provided by the sparseindex class.
 *
 * The diskstorage class manages an output file stream that is used to append data
 * to the file "data.log". Every INTERVAL number of insertions, it updates the sparse
 * index with the offset position of the key within the file. The read() member function
 * uses the sparse index to retrieve and search for key-value entries efficiently.
 *
 * Usage Example:
 * @code {.cpp}
 * diskstorage ds;
 * sparseindex index;
 * ds.write("sampleKey", "sampleValue", index);
 * std::string value = ds.read("sampleKey", index);
 * @endcode
 * 
 * @see sparseindex.h
 */
#pragma once
#include <iostream>
#include <fstream>
#include <string>

#include "sparseindex.h"

#define INTERVAL 100
#define LOGFILE "data.log"

/**
 *This class is used to store key-value pairs on disk.
 * 
 */
class diskstorage {
private:
    std::ofstream* outFile;
    int counter = 0;
public:
    /**
     *Construct a new diskstorage object. It opens the file "data.log" in current directory in append mode.
     * It currently stores data as key-value pairs in the file (space-separated).
     * 
     */
    diskstorage() {
        outFile = new std::ofstream("data.log", std::ios::app);
    }

    ~diskstorage() {
        delete outFile;
    }

    /**
     *Function to write a key-value pair to disk. It also updates the sparse index with the key and offset in the file.
     * 
     * @param key The key in the key-value pair
     * @param value The value in the key-value pair
     * @param sparseIndex The sparse index that stores the key and offset in the file
     * @return **true**, if the key-value pair is written successfully
     * @return **false**, otherwise
     */
    bool write(const std::string& key, const std::string& value, sparseindex& sparseIndex) {
        std::ofstream* file = outFile;
        
        /* find the offset in the file */
        std::streampos offset = file->tellp();

        /* write to the file */
        *file << key << " " << value << "\n";
        (*file).flush();
        
        /* Update the sparse index if required */
        if((counter % INTERVAL) == 0)
            sparseIndex.insert(key, offset);
        ++counter;
        return true;
    }

    /**
     *Function to read a key-value pair from disk using the key. It uses the sparse index to find the offset in the file.
     * 
     * @param key The key in the key-value pair
     * @param sparseIndex The sparse index that stores the key and offset in the file
     * @return std::string The value in the key-value pair
     */
    std::string read(const std::string& key, sparseindex& sparseIndex) {
        std::ifstream inFile(LOGFILE);
        if (!inFile.is_open())
            return "";
        
        /* Get the offset from where to read and go there */
        std::streampos startPos = sparseIndex.get_offset(key);
        inFile.seekg(startPos);

        /*  */
        std::string fileKey, fileValue;
        int count = INTERVAL;
        while ((count--) && inFile >> fileKey >> fileValue) {
            if (fileKey == key) {
                return fileValue;
            }
        }
        return "";
    }
       
};
