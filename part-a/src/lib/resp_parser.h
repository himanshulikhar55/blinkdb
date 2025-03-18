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
    std::string resp_str;
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

        std::string convert_to_resp(parse_op* pop){
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
            return bufferStr;
            // buffer.resize(bufferStr.length());
            std::memcpy(buffer.data(), bufferStr.data(), bufferStr.size());
        }

        void _parse_(std::string input, parse_op* ip){
            std::string cmd, key;
            ss >> cmd;
            to_lower(cmd);
            if(cmd == "print"){
                ip->cmd = std::move(cmd);
                return;
            }
            ss >> key;
            if(cmd == "set"){
                std::string temp = "";
                std::string& valPtr = ip->value;
                while(ss >> temp){
                    valPtr += temp + " ";
                }
                // std::cout << ip->op << " " << ip->key << " " << ip->value << std::endl;
            }
            op->resp_str = convert_to_resp(ip);
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
        bool invalid_cmd_len(std::string& data, int& i){
            if(data[i] != '$')
                return false;
            int len = std::stoi(get_len(data, i));
            if(len != 3)
                return false;
            return true;
        }
        std::string get_cmd(std::string &data, int& i){
            std::string cmd = data.substr(i, 3);
            to_lower(cmd);
            if(cmd != "set" || cmd != "get" || cmd != "del")
                return "";
            // check for delimiters
            if(data[i+3] != '\r' || data[i+4] != '\n')
                return "";
            i += 5; // 3  + 2
            return cmd;
        }
        int get_key_len(std::string& data, int& i){
            if(data[i] != '$')
                return 0;
            int key_len = std::stoi(get_len(data, i));
            if(key_len < 1)
                return 0;
            return key_len;
        }
        std::string verify_and_get_val(std::string& data, int& i){
            if(i == data.length())
                    return "";
                if(data[i] != '$')
                    return "";
                int val_len = std::stoi(get_len(data, i));
                if(val_len < 1)
                    return "";
                std::string val = data.substr(i, val_len);
                return val;
        }
    public:
        // kept for debugging purposes. Need to move to private later
        parse_op *op;
        resp_parser(){
            op = new parse_op();
        }
        void parse(std::string input, parse_op* pop){
            _parse_(input, pop);
        }
        std::string parse_str(const std::string& input){
            ss = std::stringstream(input);
            parse_op* op = parse(input);
            if(op->cmd == "print")
                return "print";
            std::string resp = convert_to_resp(op);
            if(resp == "")
                return "-ERR\r\n";
            return resp;
        }
        void convert_to_byte_stream(std::string& bufferStr, std::string& ip){
            
            bufferStr = "$" + std::to_string(ip.length()) + "\r\n" + ip + "\r\n";
            // buffer.resize(bufferStr.length());
            std::memcpy(buffer.data(), bufferStr.data(), bufferStr.size());
        }
        int parse_command(std::string& data, parse_op* pop){
            //*3\r\n$3\r\nGET\r\n$1\r\nk\r\n
            if(data.length() < 20 || data[0] != '*')
                return 0;
            int num_args = 0, i = 1, key_len = 0;
            
            // get the number of arguments
            std::string num = get_len(data, i), cmd = "", val = "";
            num_args = std::stoi(num);
            
            if(num_args < 2 || num_args > 3)
                return 0;
            
                // cmd len starts
            if(invalid_cmd_len(data, i))
                return 0;
            
            // cmd starts
            if((cmd = get_cmd(data, i)) == "")
                return 0;
            
            // key len starts
            if(key_len = get_key_len(data, i))
                return 0;
            
            //get key
            std::string key = data.substr(i, key_len);
            i += key_len + 2;
            
            // val len and val starts
            if(cmd == "set"){
                val = verify_and_get_val(data, i);
                if(val == "")
                    return 0;
                pop->value = std::move(val);
            }
            pop->cmd = std::move(cmd);
            pop->key = std::move(key);
            return 1;
        }
};