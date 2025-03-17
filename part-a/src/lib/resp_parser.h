#pragma once
#include <string>
#include <sstream>
#include <vector>
#include <stdint.h>
#include <stdlib.h>
#include <cstring>

struct parse_op {
    std::string cmd;
    std::string key;
    std::string value;
};

class resp_parser {
    private:
        std::vector<uint8_t> buffer;
        std::stringstream ss;
        void to_lower(std::string &s){
            for(auto &c : s){
                c = tolower(c);
            }
        }
        parse_op* parse(std::string input){
            std::string cmd, key;
            ss >> cmd;
            to_lower(cmd);
            if(cmd == "print"){
                op->cmd = std::move(cmd);
                return op;
            }
            ss >> key;
            if(cmd == "set"){
                std::string temp = "";
                std::string& valPtr = op->value;
                while(ss >> temp){
                    valPtr += temp + " ";
                }
                // std::cout << ip->op << " " << ip->key << " " << ip->value << std::endl;
            }
            return op;
        }
        void convert_to_byte_stream(parse_op* pop){
            std::vector<std::string&> params = std::vector<std::string&>(3);
            std::string bufferStr = "*2\r\n$3\r\n";
            params[0] = pop->cmd;
            params[1] = pop->key;
            params[2] = pop->value;
            
            if(params[0] == "set")
                bufferStr[1] = '3';
            
            bufferStr += params[0] + // the command itself
                         "\r\n$" +
                         std::to_string(params[1].length()) + // length of the key
                         params[1] +
                         "\r\n";
            if(bufferStr[1] != '2'){
                bufferStr += "$" +
                             std::to_string(params[2].length()) +
                             "\r\n" +
                             params[2] +
                             "\r\n";
            }
            // buffer.resize(bufferStr.length());
            std::memcpy(buffer.data(), bufferStr.data(), bufferStr.size());
        }
    public:
        // kept for debugging purposes. Need to move to private later
        parse_op *op;
        resp_parser(){
            op = new parse_op();
        }
        int parse_str(const std::string& input){
            ss = std::stringstream(input);
            parse_op* op = parse(input);
            if(op->cmd == "print")
                return 1;
            convert_to_byte_stream(op);
            return -1;
        }
        void convert_to_byte_stream(std::string& bufferStr, std::string& ip){
            
            bufferStr = "$" + std::to_string(ip.length()) + "\r\n" + ip + "\r\n";
            // buffer.resize(bufferStr.length());
            std::memcpy(buffer.data(), bufferStr.data(), bufferStr.size());
        }
        std::string get_len(std::string& data, int& i){
            std::string num = "";
            for(i = 1; i < data.length(); i++){
                if(data[i] == '\r' && data[i+1] == '\n'){
                    i+=2;
                    break;
                }
                num += data[i];
            }
            return num;
        }
        int parse_command(std::string& data, parse_op* pop){
            //*3\r\n$3\r\nGET\r\n$1\r\nk\r\n
            if(data.length() < 20 || data[0] != '*')
                return 0;
            int num_args = 0, i = 1;
            // get the number of arguments
            std::string num = get_len(data, i), cmd = "";
            num_args = std::stoi(num);
            
            if(num_args < 2 || num_args > 3)
                return 0;
            // cmd len starts
            if(data[i] != '$')
                return 0;
            num = get_len(data, i);
            if(num != "3")
                return 0;
            cmd = data.substr(i, 3);
            to_lower(cmd);
            if(cmd != "set" || cmd != "get" || cmd != "del")
                return 0;
            // check for delimiters
            if(data[i+3] != '\r' || data[i+4] != '\n')
                return 0;
            i += 5; // 3  +2
            // key len starts
            if(data[i] != '$')
                return 0;
            int key_len = std::stoi(get_len(data, i));
            if(key_len < 1)
                return 0;
            std::string key = data.substr(i, key_len);
            i += key_len + 2;
            // val len starts
            if(cmd == "set"){
                if(i == data.length())
                    return 0;
                if(data[i] != '$')
                    return 0;
                int val_len = std::stoi(get_len(data, i));
                if(val_len < 1)
                    return 0;
                std::string val = data.substr(i, val_len);
                pop->value = std::move(val);
            }
            pop->cmd = std::move(cmd);
            pop->key = std::move(key);
            return 1;
        }
};