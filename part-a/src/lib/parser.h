#pragma once
#include <string>
#include <sstream>

struct parse_ip {
    std::string op;
    std::string key;
    std::string value;
};

void to_lower(std::string &s){
    for(auto &c : s){
        c = tolower(c);
    }
}

parse_ip* parse(std::string input){
    parse_ip *ip = new parse_ip();
    std::stringstream ss(input);
    std::string temp;
    ss >> ip->op;
    to_lower(ip->op);
    if(ip->op == "print")
        return ip;
    ss >> ip->key;
    if(ip->op == "set"){
        std::string value = "";
        while(ss >> temp){
            value += temp + " ";
        }
        ip->value = value;
        // std::cout << ip->op << " " << ip->key << " " << ip->value << std::endl;
    }
    return ip;
}