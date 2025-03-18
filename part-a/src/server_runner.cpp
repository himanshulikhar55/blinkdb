#include "lib/blinkdbserver.h"

int main() {
    blinkdbserver server(100);
    if (!server.start()) {
        std::cerr << "Server failed to start" << std::endl;
        return 1;
    }
    return 0;
}