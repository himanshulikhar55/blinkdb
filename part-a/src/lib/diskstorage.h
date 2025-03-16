#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <atomic>
#include "sparseindex.h"

class diskstorage {
private:
    std::atomic<std::ofstream*> outFile;

public:
    diskstorage() {
        outFile.store(new std::ofstream("data.log", std::ios::app));
    }

    ~diskstorage() {
        delete outFile.load();
    }

    bool write(const std::string& key, const std::string& value, sparseindex& sparseIndex) {
        std::ofstream* file = outFile.load();
        std::streampos offset = file->tellp();
        *file << key << " " << value << "\n";
        (*file).flush();
        sparseIndex.insert(key, offset);
        return true;
    }

    std::string read(const std::string& key, sparseindex& sparseIndex) {
        std::ifstream inFile("data.log");
        if (!inFile.is_open())
            return "";

        std::streampos startPos = sparseIndex.get_offset(key);
        inFile.seekg(startPos);

        std::string fileKey, fileValue;
        while (inFile >> fileKey >> fileValue) {
            if (fileKey == key) {
                return fileValue;
            }
        }
        return "";
    }
       
};
