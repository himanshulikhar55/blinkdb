/**
 * @file blinkdb_client.h
 * Defines the blinkdb_client class for connecting to a BlinkDB server and sending requests.
 *
 * This file provides a simple client interface that provides public methods to either directly execute commands
 * on the passed on BLINKDB instance based on the inputs or uses POSIX sockets to establish a TCP
 * connection to a BlinkDB server, send requests, and receive responses.
 * The former is used for local mode and the latter for client-server mode.
 * 
 * @copyright Copyright (c) 2025
 * @see resp_parser.h
 * @see blinkdb.h
 */
#pragma once
#include <string>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "blinkdb.h"
#include "resp_parser.h"

/**
 * @class blinkdb_client
 * Client for establishing a connection to a BlinkDB server.
 *
 * The blinkdb_client class encapsulates the functionality to create a socket, connect
 * to a server through a specified IP and port, send a request, and receive a response.
 * It ensures the proper release of socket resources when the instance is destroyed.
 *
 * Example usage:
 * @code
 *   blinkdb_client client("127.0.0.1", 8080);
 *   if (client.connect_to_server()) {
 *       std::string response = client.send_request("GET /data");
 *       std::cout << "Server response: " << response << std::endl;
 *   }
 * @endcode
 * 
 * @note The user should enter the commands in the following format only. The case of the command does not matter
 * but the format must be followed:
 * @code
 *      SET <KEY> <VALUE> (Eg: SET A 10)
 *      GET <KEY>         (Eg: GET A)
 *      DEL <KEY>         (Eg: DEL A)
 *      exit              (Eg: exit)
 *      print             (Eg: print)
 * @endcode
 * 
 * The 
 */
class blinkdb_client {
public:
    /**
     * onstructs a new blinkdb_client object.
     *
     * Initializes the client with the provided server IP address and port number. The socket
     * file descriptor is set to an invalid value (-1) until a connection is established.
     *
     * @param server_ip The IP address of the BlinkDB server.
     * @param port The port number on which the BlinkDB server is listening.
     */

    blinkdb_client(const std::string &server_ip, int port)
        : server_ip(server_ip), port(port), sock_fd(-1) {}

    blinkdb_client(): server_ip(""), port(-1), sock_fd(-1){}
    /**
     * Destroys the blinkdb_client object.
     *
     * Ensures that the open socket connection is closed upon destruction to free up system resources.
     */
    ~blinkdb_client() {
        if (sock_fd != -1) close(sock_fd);
    }
    
    /**
     * Connects to the BlinkDB server.
     *
     * This function creates a TCP socket, configures the server address using the specified IP and port,
     * and attempts to establish a connection to the BlinkDB server. If socket creation, address conversion,
     * or connection fails, the function prints an error message using perror and returns false.
     *
     * @return **true** if the connection is successfully established; false otherwise.
     * 
     * @note In case of failure, an error message is printed using perror.
     */
    bool connect_to_server() {
        sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_fd == -1) {
            perror("Socket creation failed");
            return false;
        }

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0) {
            perror("Invalid address");
            return false;
        }

        if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
            perror("Connection failed");
            return false;
        }

        return true;
    }
    /**
     * Sends a request to the BlinkDB server and receives the response.
     *
     * Transmits the provided request string over the established socket. It then waits for a response
     * from the server, reads it into a buffer, and returns it as a std::string.
     *
     * @param request A string containing the request message to be sent.
     *
     * @return A std::string containing the server's response, or the string "ERROR" if sending or
     *         receiving data fails.
     *
     * @note Error messages are printed using perror on failure.
     */
    std::string send_request(const std::string &request) {
        if (send(sock_fd, request.c_str(), request.size(), 0) == -1) {
            perror("Send failed");
            return "ERROR";
        }

        char buffer[4096] = {0};
        ssize_t bytes_received = recv(sock_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received == -1) {
            perror("Receive failed");
            return "ERROR";
        }

        return std::string(buffer, bytes_received);
    }

    void execute(blinkdb &db, parse_op *ip){
        if(ip->cmd == "SET"){
            if((ip->value).length() == 0){
                std::cout << "value cannot be empty" << std::endl;
                return;
            }
            if(!db.set(ip->key, ip->value)){
                std::cout << "Could not set " << ip->key << " to " << ip->value << ". Try again" << std::endl;
            }

        } else if(ip->cmd == "GET"){

            std::string val = db.get(ip->key);
            if(val != ""){
                std::cout << val << std::endl;
            }
            else {
                std::cout << "NULL\n";
            }
        
        } else if(ip->cmd == "DEL"){
        
            if(!db.del(ip->key)){
                std::cout << "Does not exist." << std::endl;
            }

        } else if(ip->cmd == "PRINT"){ /* will most probably remove this. This is mainly for debugging purposes */

            std::cout << "DB Content:\n";
            db.print();
        }
    }
private:
    std::string server_ip; /* IP address of the BlinkDB server */
    int port;              /* Port number of the BlinkDB server */
    int sock_fd;           /* Socket file descriptor */
};