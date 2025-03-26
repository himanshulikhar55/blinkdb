#pragma once
#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <termios.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>


void printBanner() {
    std::string banner = R"( 

 _______   __        ______  __    __  __    __  _______   _______  
|       \ |  \      |      \|  \  |  \|  \  /  \|       \ |       \
| $$$$$$$\| $$       \$$$$$$| $$\ | $$| $$ /  $$| $$$$$$$\| $$$$$$$\
| $$__/ $$| $$        | $$  | $$$\| $$| $$/  $$ | $$  | $$| $$__/ $$
| $$    $$| $$        | $$  | $$$$\ $$| $$  $$  | $$  | $$| $$    $$
| $$$$$$$\| $$        | $$  | $$\$$ $$| $$$$$\  | $$  | $$| $$$$$$$\
| $$__/ $$| $$_____  _| $$_ | $$ \$$$$| $$ \$$\ | $$__/ $$| $$__/ $$
| $$    $$| $$     \|   $$ \| $$  \$$$| $$  \$$\| $$    $$| $$    $$
 \$$$$$$$  \$$$$$$$$ \$$$$$$ \$$   \$$ \$$   \$$ \$$$$$$$  \$$$$$$$ 
               
                        BLINKDB v0.1                                                                                       
)";
    std::cout << banner << std::endl;
}

void print_log(const char* message) {
    using namespace std::chrono;

    auto now = system_clock::now();

    std::time_t now_c = system_clock::to_time_t(now);
    std::tm* now_tm = std::localtime(&now_c);

    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    std::cout << std::put_time(now_tm, "%d %b %Y %H:%M:%S")
              << '.' << std::setfill('0') << std::setw(3) << ms.count()
              << " * " << message
              << std::endl;
}


void disable_echoctl() {
    struct termios term;
    if (tcgetattr(STDIN_FILENO, &term) == 0) {
        term.c_lflag &= ~ECHOCTL; // Disable ^C, ^Z etc. being shown
        tcsetattr(STDIN_FILENO, TCSANOW, &term);
    }
}

void restore_echoctl() {
    struct termios term;
    if (tcgetattr(STDIN_FILENO, &term) == 0) {
        term.c_lflag |= ECHOCTL; // Re-enable if you want to restore
        tcsetattr(STDIN_FILENO, TCSANOW, &term);
    }
}