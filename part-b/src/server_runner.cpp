/**
 * @file server_runner.cpp
 *Implementation of the BlinkDB server application
 *
 * This file implements the main entry point for the BlinkDB server application.
 * It initializes a TCP socket server that listens for incoming connections 
 * and handles them using an epoll-based event loop. When running the server the user
 * should give as the (the only one) command line arg the maximum number of parallel
 * connections the server will be required to handle.
 *
 * The server uses the blinkdb_server class to process client requests
 * according to the RESP (Redis Serialization Protocol) format.
 *
 * @note The server listens on the port defined by PORT macro (defined in blinkdb_server.h file).
 * @note The server can be terminated by sending a SIGINT signal (Ctrl+C). It will then gracefully exit while collecting all the memory.
 */

#include "lib/blinkdb_server.h"
#include "../../part-a/src/lib/blinkdb.h"
#include "../../part-a/src/lib/resp_parser.h"
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef DEBUG
    #define MAX_CONN 1000
#endif

/**
 *Handles the server exit process
 *
 * This function is called when the server is shutting down.
 * It prints a message and exits the process gracefully.
 */
void handle_exit();

/**
 *Signal handler for SIGINT (Ctrl+C)
 *
 * @param signum The signal number received
 *
 * This function is registered as the handler for SIGINT signals
 * and calls handle_exit() to terminate the server gracefully.
 */
void handleSigInt(int signum);

int main(int argc, char* argv[]) {
    #ifdef DEBUG
        int max_parallel_conn = MAX_CONN;
    #else
        int max_parallel_conn = 0;
    #endif
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <num_parallel_connections>" << std::endl;
        return -1;
    }
    try{
        max_parallel_conn = std::stoi(argv[1]);
    } catch(std::exception &e){
        std::cout << e.what() << std::endl;
        return -1;
    }
    if(max_parallel_conn < 10){
        std::cout << "Max Parallel Connections should be atleast 10\n";
        return -1;
    }
    std::cout << "Initalizing server with " << max_parallel_conn << " parallel connections" << std::endl;
    #ifdef DEBUG
        blinkdb_server server(MAX_CONN);
    #else
        blinkdb_server server(max_parallel_conn);
    #endif

    std::cout << "Setting up server" << std::endl;
    int server_fd = -1;
    int epoll_fd = -1;
    
    signal(SIGINT, handleSigInt);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (server_fd == -1) {
        perror("Socket creation failed");
        return false;
    }
    
    int opt = 1;
    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("Setsockopt failed");
        return false;
    }
    
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        return -1;
    }
    
    if (listen(server_fd, max_parallel_conn) == -1) {
        perror("Listen failed");
        return -1;
    }
    
    std::cout << "Adding to epoll" << std::endl;
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("Epoll creation failed");
        return -1;
    }
    
    epoll_event event{EPOLLIN, {.fd=server_fd}};
    
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
        perror("Epoll_ctl failed");
        return -1;
    }
    std::cout << "All initializations successful. Server started on port " << PORT << std::endl;

    server.run(epoll_fd, server_fd);

    return 0;
}

void handle_exit(){
    std::cout << "\nExiting the server\n";
    std::remove("data.log");
    return;
}

void handleSigInt(int signum){
    handle_exit();
}