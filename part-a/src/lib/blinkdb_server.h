/**
 * @file blinkdb_server.h
 * @brief Implementation of a Redis-like server using epoll for non-blocking I/O
 * 
 * This file defines the blinkdb_server class which implements a server that:
 * - Uses epoll for efficient handling of multiple client connections
 * - Implements basic Redis-compatible commands (SET, GET, DEL, QUIT)
 * - Parses RESP (Redis Serialization Protocol) commands
 * - Provides connection management and data storage via blinkdb
 * 
 * @note Default server port is 9001
 */
#pragma once
#include <csignal>
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
#define DB_SIZE 100000

/**
 * @class blinkdb_server
 * @brief Redis-compatible server implementation
 * 
 * This class implements a server that can handle multiple client connections concurrently
 * using epoll for non-blocking I/O. It interprets RESP protocol commands and interfaces
 * with a blinkdb database instance for data storage and retrieval.
 * 
 * Supported commands:
 * - SET: Store a key-value pair
 * - GET: Retrieve a value by key
 * - DEL: Remove a key-value pair
 * - QUIT: Disconnect client
 * - CONFIG: Server configuration (basic implementation)
 * - PRINT: Debug command to display database contents
 */

class blinkdb_server {
public:
    /**
     * @brief Constructor for the blinkdb_server
     * @param max_conn Maximum number of parallel connections to handle
     */
    blinkdb_server(int max_conn) : max_parallel_conn(max_conn), parser(), db(DB_SIZE) {
    }

    /**
     * @brief Main server loop
     * 
     * Continuously listens for client connections and data using epoll.
     * Handles new client connections, processes client requests, and manages
     * disconnections. Delegates command parsing to the resp_parser and
     * data operations to the blinkdb instance.
     * 
     * @param epoll_fd File descriptor for the epoll instance
     * @param server_fd File descriptor for the server socket
     */
    void run(int epoll_fd, int server_fd) {
        epoll_event events[MAX_EVENTS];
        
        while (true) {
            int event_count = epoll_wait(epoll_fd, events, max_parallel_conn, -1);
            if (event_count == -1) {
                perror("Epoll wait failed");
                break;
            }
            for (int i = 0; i < event_count; ++i) {
                // std::cout << "HERE\n";
                if (events[i].data.fd == server_fd) {
                    sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    int client_fd = accept(server_fd, (sockaddr*)&client_addr,  &client_len);
                    if (client_fd == -1) {
                        perror("Accept failed");
                        continue;
                    }
                    
                    // Change event registration to:
                    epoll_event event{EPOLLIN | EPOLLET, {.fd=client_fd}};
                    // std::cout << "New client connected: " << client_fd
                    //           << ", errno: " << errno << std::endl;
                    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1){
                        perror("Epoll_ctl failed");
                        close(client_fd);
                        continue;
                    }
                    std::cout << "Client " << client_fd << " connected\n";
                } else {
                    // std::cout << "IN ELSE\n";
                    int client_fd = events[i].data.fd;

                    // std::cout << "Data received from client: " << client_fd << " errorno: " << errno << std::endl;
                    // std::cout << "Data: " << client_data[client_fd] << std::endl;
                    parse_op* op = new parse_op();
                    int status = handle_client(client_fd, epoll_fd, op);
                    // std::cout << "Status: " << status << '\n';
                    if(status){
                        // std::cout << "Data received from: " << client_fd << std::endl;
                        // std::cout << "Data: " << client_data[client_fd] << std::endl;
                        client_data[client_fd].clear();
                        if(op->cmd == "set"){
                            if(db.set(op->key, op->value))
                                send(client_fd, "+OK\r\n", 5, 0);
                            else
                                send(client_fd, "-ERR\r\n", 6, 0);
                        } else if(op->cmd == "get"){
                            std::string val = db.get(op->key);
                            // db.print();
                            if(val != ""){
                                std::string bufferStr = "";
                                parser.convert_to_byte_stream(bufferStr, val);
                                // std::cout << "Value: " << val << std::endl;
                                // std::cout << "Sending to client: " << bufferStr << std::endl;
                                send(client_fd, bufferStr.c_str(), bufferStr.length(), 0);
                            }
                            else {
                                std::cout << "Key not available\n";
                                send(client_fd, "-1\r\n", 3, 0);
                            }
                        } else if(op->cmd == "del"){
                            if(db.del(op->key))
                                send(client_fd, "+OK\r\n", 5, 0);
                            else
                                send(client_fd, "-ERR\r\n", 6, 0);
                        } else if(op->cmd == "quit"){
                            int client_id = client_fd;
                            send(client_fd, "+OK\r\n", 5, 0);
                            close(client_fd);
                            client_data.erase(client_id);
                            std::cout << "Client " << client_id << " disconnected\n";
                        } else if(op->cmd == "print"){ // will most probably remove this. This is mainly for debugging purposes
                            std::cout << "DB Content:\n";
                            db.print();
                            send(client_fd, "+OK\r\n", 5, 0);
                        } else if(op->cmd == "CONFIG"){
                            send(client_fd, "+OK\r\n", 5, 0);
                        }
                    } else {
                        // but data not complete
                        send(client_fd, "+OK\r\n", 5, 0);
                    }
                }
            }
        }
    }

private:
    int max_parallel_conn;
    std::unordered_map<int, std::string> client_data;
    resp_parser parser;
    blinkdb db;
    int handle_client(int client_fd, int epoll_fd, parse_op* op) {
        char buffer[1024];
        // std::cout << "Before READ\n";
        ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer));
        if(bytes_read <= 0) {
            std::cout << "Client disconnected: " << client_fd << std::endl;
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
            close(client_fd);
            return 0;
        }
        if(client_data.find(client_fd) == client_data.end()){
            client_data[client_fd] = "";
        } else {
            client_data[client_fd].clear();
        }
        client_data[client_fd].append(buffer, bytes_read);
        buffer[bytes_read] = '\0';
        if(client_data.find(client_fd) == client_data.end()){
            client_data[client_fd] = "";
        }
        int i = 0;
        while(buffer[i] != '\0'){
            client_data[client_fd].push_back(buffer[i]);
            i++;
        }
        // std::cout << "Data:\n" << client_data[client_fd] 
        //           << "\n--------------------------------------\n";
        // std::cout << "Received from: " << client_fd << std::endl;
        int st = parser.parse_command(client_data[client_fd], op);
        // std::cout << "Command: " << op->cmd << std::endl;
        // std::cout << "Key: " << op->key << std::endl;
        // std::cout << "Value: " << op->value << std::endl;
        return st;
    }
};
