#ifndef RESP_CLIENT_H
#define RESP_CLIENT_H

#include <string>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

class resp_client {
public:
    resp_client(const std::string &server_ip, int port);
    ~resp_client();

    resp_client(const std::string &server_ip, int port)
        : server_ip(server_ip), port(port), sock_fd(-1) {}

    ~resp_client() {
        if (sock_fd != -1) close(sock_fd);
    }

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
#endif
