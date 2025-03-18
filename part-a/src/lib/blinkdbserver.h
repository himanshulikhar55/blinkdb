#pragma once
#include <iostream>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <cstring>
#include <unordered_map>

#include "resp_parser.h"
#include "blinkdb.h"

#define MAX_EVENTS 100
#define PORT 9001

/*
    Next Steps

    Integrate the RESP-2 Parser with the Server
        Modify the server to use the resp_parser class you implemented. -> done
        When a client sends a command, the server should parse it using resp_parser. -> done
        Convert parsed commands into BlinkDB operations (SET, GET, DEL). -> done

    Modify the Server to Interact with BlinkDB
        Instead of simply echoing responses, the server should execute database commands.
        Example:
            If SET key value is received, call db.set(key, value). -> done
            If GET key is received, call db.get(key) and send the response. -> done

    Handle Concurrent Clients Properly -> not doing this
        Right now, the server can accept multiple clients but only processes them using epoll.
        Consider adding worker threads for handling requests if needed.

    Implement Proper RESP-2 Responses -> not done
        Right now, the server just sends +OK\r\n for every request. -> done
        Implement correct responses ($length\r\nvalue\r\n for GET, etc.). -> done
*/

class blinkdbserver {
public:
    blinkdbserver(int max_conn) : server_fd(-1), epoll_fd(-1), max_parallel_conn(max_conn), parser(), db(10000) {
    }
    ~blinkdbserver() {
        if (server_fd != -1) close(server_fd);
        if (epoll_fd != -1) close(epoll_fd);
    }

    bool start() {
        if (!setup_server())
            return false;
        run();
        return true;
    }

private:
    int server_fd;
    int epoll_fd;
    int max_parallel_conn;
    std::unordered_map<int, std::string> client_data;
    resp_parser parser;
    blinkdb db;

    bool setup_server() {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd == -1) {
            perror("Socket creation failed");
            return false;
        }
        
        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(PORT);
        
        if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
            perror("Bind failed");
            return false;
        }
        
        if (listen(server_fd, max_parallel_conn) == -1) {
            perror("Listen failed");
            return false;
        }
        
        epoll_fd = epoll_create1(0);
        if (epoll_fd == -1) {
            perror("Epoll creation failed");
            return false;
        }
        
        epoll_event event{};
        event.events = EPOLLIN;
        event.data.fd = server_fd;
        
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
            perror("Epoll_ctl failed");
            return false;
        }
        std::cout << "Server started on port " << PORT << std::endl;
        return true;
    }
    void handle_client(int client_fd) {
        char buffer[1024];
        // std::cout << "Before READ\n";
        ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer));
        // std::cout << "After READ\n";
        if (bytes_read <= 0) {
            close(client_fd);
        } else {
            buffer[bytes_read] = '\0';
            if(client_data.find(client_fd) == client_data.end()){
                client_data[client_fd] = "";
            }
            int i = 0;
            while(buffer[i] != '\0'){
                client_data[client_fd].push_back(buffer[i]);
                i++;
            }
            // std::cout << "Received from: " << client_fd << std::endl;
        }
    }
    bool check_data_complete(const std::string& data){
        if(data.size() < 5)
            return false;
        if(data.substr(data.size() - 2) == "\r\n")
            return true;
        return false;
    }
    void run() {
        epoll_event events[MAX_EVENTS];
        while (true) {
            int event_count = epoll_wait(epoll_fd, events, max_parallel_conn, -1);
            for (int i = 0; i < event_count; ++i) {
                // std::cout << "HERE\n";
                if (events[i].data.fd == server_fd) {
                    sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
                    if (client_fd == -1) {
                        perror("Accept failed");
                        continue;
                    }
                    
                    epoll_event event{};
                    event.events = EPOLLIN;
                    event.data.fd = client_fd;
                    std::cout << "New client connected: " << client_fd << std::endl;
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event);
                } else {
                    // std::cout << "IN ELSE\n";
                    handle_client(events[i].data.fd);
                    // std::cout << "Data received from client: " << events[i].data.fd << std::endl;
                    // std::cout << "Data: " << client_data[events[i].data.fd] << std::endl;
                    parse_op* op = new parse_op();
                    int status = parser.parse_command(client_data[events[i].data.fd], op);
                    // std::cout << "Status: " << status << '\n';
                    if(status){
                        // std::cout << "Data received from: " << events[i].data.fd << std::endl;
                        // std::cout << "Data: " << client_data[events[i].data.fd] << std::endl;
                        client_data.erase(events[i].data.fd);
                        if(op->cmd == "set"){
                            if(db.set(op->key, op->value))
                                send(events[i].data.fd, "+OK\r\n", 5, 0);
                            else
                                send(events[i].data.fd, "-ERR\r\n", 6, 0);
                        } else if(op->cmd == "get"){
                            std::string val = db.get(op->key);
                            if(val != ""){
                                std::string bufferStr = "";
                                parser.convert_to_byte_stream(bufferStr, val);
                                // std::cout << "Value: " << val << std::endl;
                                // std::cout << "Sending to client: " << bufferStr << std::endl;
                                send(events[i].data.fd, bufferStr.c_str(), bufferStr.length(), 0);
                            }
                            else {
                                std::cout << "Something wrong while sending\n";
                                send(events[i].data.fd, "-ERR\r\n", 6, 0);
                            }
                        } else if(op->cmd == "del"){
                            if(db.del(op->key))
                                send(events[i].data.fd, "+OK\r\n", 5, 0);
                            else
                                send(events[i].data.fd, "-ERR\r\n", 6, 0);
                        } else if(op->cmd == "quit"){\
                            int client_id = events[i].data.fd;
                            send(events[i].data.fd, "+OK\r\n", 5, 0);
                            close(events[i].data.fd);
                            client_data.erase(client_id);
                            std::cout << "Client " << client_id << " disconnected\n";
                        } else if(op->cmd == "print"){ // will most probably remove this. This is mainly for debugging purposes
                            std::cout << "DB Content:\n";
                            db.print();
                            send(events[i].data.fd, "+OK\r\n", 5, 0);
                        }
                    } else {
                        // but data not complete
                        send(events[i].data.fd, "+OK\r\n", 5, 0);
                    }
                }
            }
        }
    }
};
