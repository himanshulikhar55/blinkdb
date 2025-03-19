/**
 * @file server_runner.cpp
 * @brief Implementation of the BlinkDB server application
 *
 * This file implements the main entry point for the BlinkDB server application.
 * It initializes a TCP socket server that listens for incoming connections 
 * and handles them using an epoll-based event loop. The server is designed
 * to handle up to MAX_CONN simultaneous connections.
 *
 * The server uses the blinkdb_server class to process client requests
 * according to the RESP (Redis Serialization Protocol) format.
 *
 * @note The server listens on the port defined by PORT macro (defined elsewhere).
 * @note The server can be terminated by sending a SIGINT signal (Ctrl+C).
 */

/**
 * @brief Main entry point for the BlinkDB server
 *
 * This function:
 * 1. Creates a blinkdb_server instance
 * 2. Sets up signal handling for graceful termination
 * 3. Creates and configures a TCP socket to listen for connections
 * 4. Sets up an epoll instance for efficient I/O multiplexing
 * 5. Starts the server's main event loop by calling blinkdb_server::run()
 *
 * @return 0 on successful execution, negative value on error
 */
#include "lib/blinkdb_server.h"
#include "lib/blinkdb.h"
#include "lib/resp_parser.h"
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_CONN 1000
/**
 * @brief Handles the server exit process
 *
 * This function is called when the server is shutting down.
 * It prints a message and exits the process gracefully.
 */
void handle_exit();

/**
 * @brief Signal handler for SIGINT (Ctrl+C)
 *
 * @param signum The signal number received
 *
 * This function is registered as the handler for SIGINT signals
 * and calls handle_exit() to terminate the server gracefully.
 */
void handleSigInt(int signum);

int main() {
    blinkdb_server server(MAX_CONN);
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
    
    if (listen(server_fd, MAX_CONN) == -1) {
        perror("Listen failed");
        return -1;
    }
    
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
    std::cout << "Server started on port " << PORT << std::endl;

    server.run(epoll_fd, server_fd);

    return 0;
}

void handle_exit(){
    std::cout << "\nExiting the server\n";
    exit(0);
    std::cin.clear();
    std:: cout << "Failed to exit the server\n";
}

void handleSigInt(int signum){
    handle_exit();
}