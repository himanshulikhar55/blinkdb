#pragma once
#include <string>
#include <sstream>
#include <vector>
#include <stdint.h>
#include <stdlib.h>
#include <iostream>
#include <cstring>

struct parse_op {
    std::string cmd;
    std::string key;
    std::string value;
    std::string resp_str;
};

class resp_parser {
    private:
        const std::string quit = "*1\r\n$4\r\nQUIT\r\n";
        std::stringstream ss;
        void to_lower(std::string &s){
            for(auto &c : s){
                c = tolower(c);
            }
        }
        void to_higher(std::string &s){
            for(auto &c : s){
                c = toupper(c);
            }
        }
        void convert_to_resp(parse_op* pop){
            std::vector<std::string> params = std::vector<std::string>(3);
            std::string bufferStr = "*2\r\n$3\r\n";
            params[0] = std::move(pop->cmd);
            params[1] = std::move(pop->key);
            params[2] = std::move(pop->value);
            
            if(params[0] == "SET")
                bufferStr[1] = '3';
            
            bufferStr += params[0] + // the command itself
                         "\r\n$" +
                         std::to_string(params[1].length()) + // length of the key
                         "\r\n" +
                         params[1] +
                         "\r\n";
            if(bufferStr[1] != '2'){
                bufferStr += "$" +
                             std::to_string(params[2].length()) +
                             "\r\n" +
                             params[2] +
                             "\r\n";
            }
            pop->resp_str = std::move(bufferStr);
        }

        void _parse_(std::string input, parse_op* ip){
            if(input == "print"){
                ip->resp_str = "print";
                return;
            }
            std::string cmd, key;
            ss = std::stringstream(input);
            ss >> cmd;
            to_higher(cmd);
            ip->cmd = cmd;
            to_lower(cmd);
            if(cmd == "print"){
                ip->cmd = std::move(cmd);
                return;
            }
            ss >> key;
            ip->key = key;
            if(cmd == "set"){
                std::string temp = "";
                std::string val = "";
                while(ss >> temp){
                    val += temp + " ";
                }
                val.pop_back();
                ip->value = val;
            }
            convert_to_resp(ip);
        }

        std::string get_len(std::string& data, int *i){
            std::string num = "";
            int j = *i;
            for(; j < data.length(); j++){
                if(data[j] == '\r' && data[j+1] == '\n'){
                    j+=2;
                    break;
                }
                num += data[j];
            }
            *i = j;
            return num;
        }
        bool invalid_cmd_len(std::string& data, int *i){
            int j = *i;
            if(data[j] != '$')
                return true;
            *i += 1;
            int len = std::stoi(get_len(data, i));
            if(len != 3)
                return true;
            return false;
        }
        std::string get_cmd(std::string &data, int *i){
            int j = *i;
            std::string cmd = data.substr(j, 3);
            to_lower(cmd);
            if(cmd != "set" && cmd != "get" && cmd != "del")
                return "";
            // check for delimiters
            if(data[j+3] != '\r' || data[j+4] != '\n')
                return "";
            j += 5; // 3  + 2
            *i = j;
            return cmd;
        }
        int get_key_len(std::string& data, int *i){
            if(data[*i] != '$')
                return 0;
            *i += 1;
            int key_len = std::stoi(get_len(data, i));
            if(key_len < 1)
                return 0;
            return key_len;
        }
        std::string verify_and_get_val(std::string& data, int *i){
            int j = *i;
            if(j == data.length())
                return "";
            if(data[j] != '$')
                return "";
            *i += 1;
            int val_len = std::stoi(get_len(data, i));
            if(val_len < 1)
                return "";
            std::string val = data.substr(*i, val_len);
            *i += val_len + 2;
            return val;
        }
    public:
        // kept for debugging purposes. Need to move to private later
        parse_op *op;
        resp_parser(){
            op = new parse_op();
        }
        void get_val(std::string& response){
            int pos = response.find("\r\n");
            if(pos == std::string::npos)
                return;
            int len = std::stoi(response.substr(1, pos - 1));
            response = response.substr(pos+2, len);
        }
        void parse(std::string input, parse_op* pop){
            _parse_(input, pop);
        }
        // std::string parse_str(const std::string& input){
        //     ss = std::stringstream(input);
        //     parse_op* op;
        //     parse(input, op);
        //     if(op->cmd == "print")
        //         return "print";
        //     std::string resp = convert_to_resp(op);
        //     if(resp == "")
        //         return "-ERR\r\n";
        //     return resp;
        // }
        void convert_to_byte_stream(std::string& bufferStr, std::string& ip){
            bufferStr = "$" + std::to_string(ip.length()) + "\r\n" + ip + "\r\n";
        }
        int extract_print(std::string& data, parse_op* pop){
            int i = 1;
            std::string num = get_len(data, &i);
            int len = std::stoi(num);
            if(len != 5)
                return 0;
            std::string cmd = data.substr(i, len);
            to_lower(cmd);
            if(cmd != "print")
                return 0;
            pop->cmd = std::move(cmd);
            return 1;
        }
        int parse_command(std::string& data, parse_op* pop){
            //*3\r\n$3\r\nGET\r\n$1\r\nk\r\n
            //"2\r\n$3\r\nGET\r\n$1a\r\n
            //$5\r\nprint\r\n
            if(data == "print"){
                pop->cmd = "print";
                return 1;
            }
            if(data.length() < 11 || data[0] != '*')
                return 0;
            if(data == quit){
                pop->cmd = "quit";
                return 1;
            }
            if(data[0] == '$')
                return extract_print(data, pop);
            int num_args = 0, i = 1, key_len = 0;
            
            // get the number of arguments
            std::string num = get_len(data, &i), cmd = "", val = "";
            num_args = std::stoi(num);
            // std::cout << num_args << std::endl;
            if(num_args < 2 || num_args > 3)
                return 0;
            
            // cmd len starts
            if(invalid_cmd_len(data, &i))
                return 0;
            
            // cmd starts
            if((cmd = get_cmd(data, &i)) == "")
                return 0;
            
            // key len starts
            if((key_len = get_key_len(data, &i)) == 0)
                return 0;
            
            //get key
            std::string key = data.substr(i, key_len);
            i += key_len + 2;
            
            // val len and val starts
            if(cmd == "set"){
                val = verify_and_get_val(data, &i);
                if(val == "")
                    return 0;
                pop->value = std::move(val);
            }
            pop->cmd = std::move(cmd);
            pop->key = std::move(key);
            std::cout << pop->cmd << " " << pop->key << " " << pop->value << std::endl;
            return 1;
        }
};