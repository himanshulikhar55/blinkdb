#include "avl.h"

#define NULLSTR ""

#define MAX_SIZE 4096

class memtable {
    private:
        avl_node *head;
        int count = 0;
    public:
        memtable(){
            head = nullptr;
        }
        bool _insert(std::string key, std::string value){
            if(count == MAX_SIZE){
                // need to save into a vector and start a new tree
                // Will write that logic later
            }
            if(!head){
                head = new avl_node(key, value);
                return true;
            }
            head = head->insert(key, value);
            return true;
        }
        std::string _get(std::string key){
            avl_node *temp = head;
            while(temp){
                if(temp->key == key)
                    return temp->get_value();
                else if(temp->key < key)
                    temp = temp->right;
                else
                    temp = temp->left;
            }
            return NULLSTR;
        }
        bool _del(std::string key){
            head = head->del(key);
            return true;
        }

        void print(){
            head->print();
        }
};