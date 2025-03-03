#include "lsmtree.h"

class blinkdb {
    private:
        lsmtree lsm;
    public:
        /**
         * @brief Construct a new blinkdb object
         * 
         */
        blinkdb(){
            lsm = lsmtree();
        }
        /**
         * @brief Helps set a key-value pair
         * 
         * @param key The key in the key-value pair
         * @param value The value in the key-value pair
         * @return true when the key-value pair is set successfully
         * @return false otherwise
         */
        bool set(std::string key, std::string value){
            if(lsm.insert(key, value))
                return true;
            return false;
        }
        /**
         * @brief Helps get value correspoding to a key
         * 
         * @param key The key whose value you want to retrieve
         * @return std::string 
         */
        std::string get(std::string key){
            return lsm.get(key);
        }
        /**
         * @brief Helps delete a key-value pair
         * 
         * @param key The key that you want to delete
         * @return true when the key-value pair is deleted successfully
         * @return false otherwise
         */
        bool del(std::string key){
            return lsm._del(key);
        }

        void print(){
            lsm.print();
        }
};