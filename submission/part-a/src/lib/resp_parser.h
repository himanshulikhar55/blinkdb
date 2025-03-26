/**
 * @file resp_parser.h
 *Provides functionalities for parsing and converting commands using the Redis Serialization Protocol (RESP).
 *
 * This file defines the data structure and class used to process commands in RESP format.
 * It includes:
 *   - The parse_op structure, which encapsulates a parsed command's details including
 *     the command type, key, value, and the generated RESP formatted string.
 *   - The resp_parser class, responsible for parsing input strings and converting them
 *     into RESP-encoded messages. It supports commands such as GET, SET, DEL, and PRINT,
 *     handling conversion between standard string representation and RESP formatted data.
 *
 * The parser offers robust mechanisms to validate, format, and retrieve command components,
 * ensuring the correct conversion to and from RESP representations. Additionally, helper
 * methods are provided for converting strings to byte streams and handling specific command
 * conditions like quit, print, and configuration commands.
 *
 * Detailed functionality is documented within the respective methods.
 */
#pragma once
#include "debug.h"

#include <string>
#include <sstream>
#include <vector>
#include <stdint.h>
#include <stdlib.h>
#include <iostream>
#include <cstring>

/**
 *Represents a parsed command operation.
 *
 * This structure holds the details of a command parsed from an input string.
 * It encapsulates the command, key, and value parts of the operation, as well
 * as a generated response string in RESP format (used in communication protocols).
 *
 * Members:
 * - cmd: The command name (e.g., "SET", "GET", "DEL", etc.) indicating the operation.
 * - key: The target key on which the command operates.
 * - value: The associated value for operations that require it (e.g., "SET"); empty otherwise.
 * - resp_str: The response string generated following the parsing of the command,
 *             formatted as a RESP message.
 */
struct parse_op {
    std::string cmd;
    std::string key;
    std::string value;
    std::string resp_str;
};

/**
 *A parser for RESP (Redis Serialization Protocol) commands.
 *
 * The resp_parser class provides methods to parse input strings formatted in RESP,
 * extract commands and their arguments, and convert standard strings into RESP-formatted
 * byte streams. It supports parsing of commands such as GET, SET, DEL, and PRINT.
 *
 */
class resp_parser {
    private:
        const std::string quit = "*1\r\n$4\r\nQUIT\r\n";
        std::stringstream ss;
        parse_op *op;
        
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
            
            bufferStr += params[0] + /* the command itself */
                         "\r\n$" +
                         std::to_string(params[1].length()) + /* length of the key */
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

        void _parse_(std::string input, parse_op* ip, int mode){
            if(input == "print"){
                if(mode == 2)
                    ip->resp_str = "print";
                else
                    ip->cmd = "PRINT";
                return;
            }
            std::string cmd, key;
            ss = std::stringstream(input);
            ss >> cmd;
            to_higher(cmd);
            ip->cmd = cmd;
            to_lower(cmd);
            ss >> key;
            ip->key = key;
            if(cmd == "set"){
                std::string temp = "";
                std::string val = "";
                while(ss >> temp){
                    val += temp + " ";
                }
                /* Value might be empty */
                if(val.length() > 0)
                    val.pop_back();
                ip->value = val;
            }
            if(mode == 2)
                convert_to_resp(ip);
        }

        std::string get_len(std::string& data, int *i){
            std::string num = "";
            size_t j = *i;
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
            int j = *i, len = 0;
            if(data[j] != '$')
                return true;
            *i += 1;
             /* stoi spoils the party which is why it has to be kept under supervision */
             try{
                len = std::stoi(get_len(data, i));
            } catch(const std::exception& e){
                std::cerr << e.what() << '\n';
                return false;
            }
            if(len != 3)
                return true;
            return false;
        }
        std::string get_cmd(std::string &data, int *i){
            int j = *i;
            std::string cmd = "";
            try
            {
                cmd = data.substr(j, 3);
            } catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
                return "";
            }
            
            to_lower(cmd);
            if(cmd != "set" && cmd != "get" && cmd != "del")
                return "";
            /* check for delimiters */
            if(data[j+3] != '\r' || data[j+4] != '\n')
                return "";
            j += 5; /* 3  + 2 */
            *i = j;
            return cmd;
        }
        int get_key_len(std::string& data, int *i){
            int key_len = 0;
            if(data[*i] != '$')
                return 0;
            *i += 1;
            /* stoi spoils the party which is why it has to be kept under supervision */
            try{
                key_len = std::stoi(get_len(data, i));
            } catch(const std::exception& e){
                std::cerr << e.what() << '\n';
                return 0;
            }
            if(key_len < 1)
                return 0;
            return key_len;
        }
        std::string verify_and_get_val(std::string& data, int *i){
            size_t j = *i, val_len = 0;
            if(j == data.length())
                return "";
            if(data[j] != '$')
                return "";
            *i += 1;
             /* stoi spoils the party which is why it has to be kept under supervision */
             try{
                val_len = std::stoi(get_len(data, i));
             } catch(const std::exception& e){
                 std::cerr << e.what() << '\n';
                 return "";
             }
            if(val_len < 1)
                return "";
            std::string val = data.substr(*i, val_len);
            *i += val_len + 2;
            return val;
        }
    
    public:
        resp_parser(){
            op = new parse_op();
        }
        ~resp_parser(){
            delete op;
        }
        void get_val(std::string& response){
            size_t pos = response.find("\r\n"), len = 0;
            if(pos == std::string::npos)
                return;
            try{
                len = std::stoi(response.substr(1, pos - 1));
            } catch(std::exception &e){
                std::cerr << e.what() << '\n';
                return;
            }
            response = response.substr(pos+2, len);
        }
        void parse(std::string input, parse_op* pop, int mode){
            _parse_(input, pop, mode);
        }
        /**
         *Converts an IP string to a byte stream.
         *
         * This function constructs a byte stream from the given IP string by first creating a prefix with
         * the '$' character followed by the length of the IP string, then a carriage return and line feed (CRLF),
         * the IP string itself, and finally another CRLF.
         *
         * @param bufferStr Reference to a string where the resulting byte stream will be stored.
         * @param ip The IP address string to be converted.
         */
        void convert_to_byte_stream(std::string& bufferStr, std::string& ip){
            bufferStr = "$" + std::to_string(ip.length()) + "\r\n" + ip + "\r\n";
        }
        /**
         *Parses a RESP-encoded command string and extracts its components.
         *
         * This function processes an input RESP string (data) and attempts to extract the command,
         * key, and optionally the value from it. The parsed components are stored in the provided
         * parse_op structure (pop).
         *
         * The function handles several special cases:
         *   - If the input is "print", it sets the command in pop to "print" and returns success.
         *   - If the input contains "CONFIG", it sets the command to "CONFIG" and returns success.
         *   - If the input matches the predefined quit command, the command is set to "quit".
         *
         * For a general RESP command, the function:
         *   - Verifies that the input starts with '*' and is of sufficient length.
         *   - Extracts the number of arguments.
         *   - Validates and retrieves the command string using helper functions, ensuring it is one
         *     of the supported commands ("set", "get", "del").
         *   - Extracts the key from the input.
         *   - If the command is "set", it also extracts the associated value.
         *
         * @param data A reference to the RESP-encoded command string to be parsed.
         * @param pop  A pointer to a parse_op structure where the parsed command, key, and value
         *             (if applicable) will be stored.
         * 
         * @return int Returns 1 if the command is successfully parsed; otherwise, returns 0 to
         *         indicate a parsing failure (due to incorrect formatting or unsupported number of arguments).
         */
        int parse_command(std::string& data, parse_op* pop){
            //*3\r\n$3\r\nGET\r\n$1\r\nk\r\n
            //*2\r\n$3\r\nGET\r\n$1\r\na\r\n
            //$5\r\nprint\r\n
            if(data == "print"){
                pop->cmd = "print";
                return 1;
            }
            if(data.find("CONFIG") != std::string::npos){
                pop->cmd = "CONFIG";
                return 1;
            }
            if(data.length() < 11 || data[0] != '*')
                return 0;   
            if(data == quit){
                pop->cmd = "quit";
                return 1;
            }
            int num_args = 0, i = 1, key_len = 0;
            
            /* get the number of arguments */
            std::string num = get_len(data, &i), cmd = "", val = "";
            num_args = std::stoi(num);
            if(num_args < 2 || num_args > 3)
                return 0;
            
            /* cmd len starts */
            if(invalid_cmd_len(data, &i))
                return 0;
            
            /* cmd starts */
            if((cmd = get_cmd(data, &i)) == "")
                return 0;
            
            /* key len starts */
            if((key_len = get_key_len(data, &i)) == 0)
                return 0;
            
            /* get key */
            std::string key = data.substr(i, key_len);
            i += key_len + 2;
            
            /* val len and val starts */
            if(cmd == "set"){
                val = verify_and_get_val(data, &i);
                if(val == "")
                    return 0;
                pop->value = std::move(val);
            }
            pop->cmd = std::move(cmd);
            pop->key = std::move(key);
            DEBUG_PRINT2("Command: ", pop->cmd); 
            DEBUG_PRINT2("Key: ", pop->key);
            DEBUG_PRINT2("Value: ", pop->value);
            return 1;
        }
};