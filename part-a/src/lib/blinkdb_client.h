/**
 * @file blinkdb_client.h
 * @brief Defines the blinkdb_client class for connecting to a BlinkDB server and sending requests.
 *
 * This file provides a simple client interface that uses POSIX sockets to establish a TCP
 * connection to a BlinkDB server, send requests, and receive responses.
 */

/**
 * @brief Establishes a connection to the BlinkDB server.
 *
 * Creates a TCP socket, sets up the server address structure using the provided IP and port,
 * and attempts to establish a connection to the server.
 *
 * @return true if the connection was successfully established; false otherwise.
 *
 * @note In case of failure, an error message is printed using perror.
 */
#pragma once
#include <string>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

/**
 * @class blinkdb_client
 * @brief Client for establishing a connection to a BlinkDB server.
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
 */
class blinkdb_client {
public:
    /**
     * @brief Constructs a new blinkdb_client object.
     *
     * Initializes the client with the provided server IP address and port number. The socket
     * file descriptor is set to an invalid value (-1) until a connection is established.
     *
     * @param server_ip The IP address of the BlinkDB server.
     * @param port The port number on which the BlinkDB server is listening.
     */

    blinkdb_client(const std::string &server_ip, int port)
        : server_ip(server_ip), port(port), sock_fd(-1) {}

    /**
     * @brief Destroys the blinkdb_client object.
     *
     * Ensures that the open socket connection is closed upon destruction to free up system resources.
     */
    ~blinkdb_client() {
        if (sock_fd != -1) close(sock_fd);
    }
    
    /**
     * @brief Connects to the BlinkDB server.
     *
     * This function creates a TCP socket, configures the server address using the specified IP and port,
     * and attempts to establish a connection to the BlinkDB server. If socket creation, address conversion,
     * or connection fails, the function prints an error message using perror and returns false.
     *
     * @return true if the connection is successfully established; false otherwise.
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
     * @brief Sends a request to the BlinkDB server and receives the response.
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
private:
    std::string server_ip;
    int port;
    int sock_fd;
};