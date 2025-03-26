/**
 * @file blinkdb_server.h
 * Implementation of a Redis-like server using epoll for non-blocking I/O
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

#include "../../../part-a/src/lib/resp_parser.h"
#include "../../../part-a/src/lib/blinkdb.h"
#include "../../../part-a/src/lib/debug.h"
#include "lib/utils.h"

#define MAX_EVENTS 1024
#define PORT 9001

#ifdef DEBUG
    #define DB_SIZE 1000
#else
    #define DB_SIZE 100000
#endif
/**
 * @class blinkdb_server
 * Redis-compatible server implementation
 * 
 * This class implements a server that can handle multiple client connections concurrently
 * using **epoll** for **non-blocking I/O**. It interprets **RESP2** protocol commands and interfaces
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
     *Constructor for the blinkdb_server
     * @param max_conn Maximum number of parallel connections to handle. This will be passed as a param when initializing the server instance
     */
    blinkdb_server(int max_conn) : max_parallel_conn(max_conn), parser(), db(DB_SIZE) {
    }

    ~blinkdb_server() {
        std::remove("data.log");
        print_log("\nBlinkDB is now exiting the server. Bbye...");
        std::cout << "Server shutting down\n";

        client_data.clear();
    }

    /**
     *Main server loop
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
        DEBUG_PRINT2("Server started on port: ", PORT);
        std::vector<epoll_event> events = std::vector<epoll_event>(MAX_EVENTS);
        
        while (true) {
            /* Wait for an event */
            int event_count = epoll_wait(epoll_fd, events.data(), max_parallel_conn, -1);
            
            if (event_count == -1) {
                perror("Epoll wait failed");
                break;
            }
            /* Iterate over generated events */
            for (int i = 0; i < event_count; ++i) {
                if (events[i].data.fd == server_fd) {
                    sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    int client_fd = accept(server_fd, (sockaddr*)&client_addr,  &client_len);
                    if (client_fd == -1) {
                        perror("Accept failed");
                        continue;
                    }
                    
                    epoll_event event{EPOLLIN | EPOLLET, {.fd=client_fd}};
                    DEBUG_PRINT2("New client connected: ", client_fd);
                    
                    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1){
                        perror("Epoll_ctl failed");
                        close(client_fd);
                        continue;
                    }
                    
                    std::cout << "Client " << client_fd << " connected\n";
                } else {
                    int client_fd = events[i].data.fd;

                    DEBUG_PRINT2("Data received from client: ", client_fd);
                    DEBUG_PRINT2("Data: ", client_data[client_fd]);
                    parse_op* op = new parse_op();
                    /* Check if the data received is in proper format */
                    int status = handle_client(client_fd, epoll_fd, op);
                    /* Means the incoming data is in RESP2 format */
                    if(status){
                        client_data[client_fd].clear();
                        /* Handle set */
                        if(op->cmd == "set"){

                            if(db.set(op->key, op->value))
                                send(client_fd, "+OK\r\n", 5, 0);
                            else
                                send(client_fd, "-ERR\r\n", 6, 0);

                        } else if(op->cmd == "get"){

                            std::string val = db.get(op->key);
                            if(val != ""){
                                std::string bufferStr = "";
                                parser.convert_to_byte_stream(bufferStr, val);
                                send(client_fd, bufferStr.c_str(), bufferStr.length(), 0);
                            }
                            else {
                                DEBUG_PRINT("Key not available\n");
                                send(client_fd, "+OK\r\n", 5, 0);
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

                        } else if(op->cmd == "print"){ /* will most probably remove this. This is mainly for debugging purposes */

                            std::cout << "DB Content:\n";
                            db.print();
                            send(client_fd, "+OK\r\n", 5, 0);

                        } else if(op->cmd == "CONFIG"){
                            send(client_fd, "+OK\r\n", 5, 0);
                        }
                    } else {
                        /* Command is not in valid RESP2 Format but still sending OK */
                        send(client_fd, "+OK\r\n", 5, 0);
                    }
                    delete op;
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
        } else {
            client_data[client_fd].clear();
        }
        int i = 0;
        while(buffer[i] != '\0'){
            client_data[client_fd].push_back(buffer[i]);
            i++;
        }
        DEBUG_PRINT2("Data:\n", client_data[client_fd]);
        DEBUG_PRINT("------------------------------------");
        DEBUG_PRINT2("Received from: ", client_fd);
        int st = parser.parse_command(client_data[client_fd], op);
        DEBUG_PRINT2("Command: ", op->cmd);
        DEBUG_PRINT2("Key: ", op->key);
        DEBUG_PRINT2("Value: ", op->value);
        return st;
    }
};
