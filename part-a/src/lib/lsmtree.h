#include "memtable.h"

/**
 * @brief This is the 
 * 
 */

class lsmtree{
    private:
        memtable mTable;
    public:
        lsmtree(){
            mTable = memtable();
        }
        bool insert(std::string key, std::string value){
            if(mTable._insert(key, value))
                return true;
            return false;
        }
        std::string get(std::string key){
            return mTable._get(key);
        }
        bool _del(std::string key){
            return mTable._del(key);
        }
        void print(){
            mTable.print();
        }
};