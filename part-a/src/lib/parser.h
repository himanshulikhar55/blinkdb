#pragma once
#include <string>
#include <sstream>

struct parse_ip {
    std::string op;
    std::string key;
    std::string value;
};

parse_ip* parse(std::string input){
    parse_ip *ip = new parse_ip();
    std::stringstream ss(input);
    std::string temp;
    ss >> temp;
    ip->op = std::move(temp);
    if(ip->op == "PRINT")
        return ip;
    ss >> temp;
    ip->key = std::move(temp);
    if(ip->op == "SET"){
        ss >> temp;
        ip->value = std::move(temp);
    }
    return ip;
}