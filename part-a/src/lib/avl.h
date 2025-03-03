#pragma once
#include <string>
#include <iostream>
#include <algorithm>

class avl_node{
    private:
        std::string value;
        int find_height(avl_node *head){
            if(head){
                int left_ht = 1 + find_height(head->left);
                int right_ht = 1 + find_height(head->right);
                return std::max(left_ht, right_ht);
            }
            return 0;
        }

        void print_tree(avl_node *head){
            if(head){
                std::cout << '(' << head->key  << ", " << head->value << ") ";
                print_tree(head->left);
                print_tree(head->right);
            } else {
                std::cout << "-1 ";
            }
        }
        
        avl_node *right_rotate(avl_node *head){
            avl_node *temp1 = head->left;
            avl_node *temp2 = temp1->right;
            temp1->right = head;
            head->left = temp2;
            return temp1;
        }
        
        avl_node *left_rotate(avl_node *head){
            avl_node *temp1 = head->right;
            avl_node *temp2 = temp1->left;
        
            head->right = temp2;
            temp1->left = head;
            return temp1;
        }

        avl_node * delete_key(avl_node *head, std::string key){
            if(!head)
                return head;
            else if(key < head->key){
                head->left = delete_key(head->left, key);
            } else if(head->key > key){
                head->right = delete_key(head->right, key);
            }  else {
                if(!head->left && !head->right)
                    return nullptr;
                if(!head->right)
                    head = head->left;
                else
                    *head = *(head->right);
            }
            int bal = find_height(head->left) - find_height(head->right);
            if(bal > 1){
                int left_ht = !head->left ? 0 : find_height(head->left->left);
                int right_ht = !head->right ? 0 : find_height(head->left->right);
                if(left_ht-right_ht >= 0)
                    return right_rotate(head);
                else{
                    head->left = left_rotate(head->left);
                    return right_rotate(head);
                }
            } else if(bal < -1){
                int left_ht = !head->left ? 0 : find_height(head->right->left);
                int right_ht = !head->right ? 0 : find_height(head->right->right);
                if(left_ht-right_ht <= 0)
                    return left_rotate(head);
                else{
                    head->right = right_rotate(head->right);
                    return left_rotate(head);
                }
            }
            return head;
        }
        
        avl_node * insert_node(avl_node *head, std::string key, std::string val){
            if(key < head->key)
                head->left =  insert_node(head->left, key, val);
            else if(key > head->key)
                head->right =  insert_node(head->right, key, val);
            else
                return head;
        
            int bal = find_height(head->left) - find_height(head->right);
            //means left has more than right. left left case
            if(bal > 1 && key < head->left->key)
                return right_rotate(head);
            else if(bal > 1 && key > head->left->key){
                head->left = left_rotate(head->left);
                return right_rotate(head);
            } if(bal < -1 && key > head->right->key)
                return left_rotate(head);
            else if(bal < -1 && key < head->right->key){
                head->right = right_rotate(head->right);
                return left_rotate(head);
            }
        
            return head;
        }
    public:
        avl_node *left;
        avl_node *right;
        std::string key;
        avl_node(std::string k, std::string v){
            key = k;
            value = std::move(v);
            left = nullptr;
            right = nullptr;
        }
        std::string get_value(){
            return this->value;
        }
        avl_node* insert(std::string key, std::string val){
            return insert_node(this, key, val);
        }
        avl_node* del(std::string key){
            return delete_key(this, key);
        }
        void print(){
            print_tree(this);
        }
};
