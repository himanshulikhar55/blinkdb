/**
 * @file client_runner.cpp
 *Client runner for connecting to BlinkDB and interacting with it.
 *
 * This file provides the main functionality for a client that connects to a BlinkDB server.
 * It establishes a connection to the server on a predefined port, handles user input, and
 * communicates using the RESP2 protocol. The client supports sending various commands and
 * cleanly exits on receiving an exit command or SIGINT signal.
 * 
 * The client can run in only one of two modes: The first mode is "Local mode"
 * which basically instantiates a local instance of BlinkDB and runs GET, SET and DEL command.
 * The second mode is the "Client-Server mode". It is the mode in which the user connects to a BLINKDB server
 * and all the requests are processed in the Server-side. The client takes the input from the user, parses it into
 * RESP2 format and uses POSIX sockets to establish a TCP connection to a BlinkDB server.
 * 
 * @note The client can only run in the modes described above. The user should run the client in the following way:
 * @code
 *     ./client_runner <mode>
 * @endcode
 * where <mode> can be either 1 or 2. 1 is for local mode and 2 is for client-server mode.
 *
 * Signal Handlers:
 *   - SIGINT: Invokes the function to safely exit the client by sending a QUIT command.
 *   - SIGTSTP: Outputs a prompt message to remind the user of the available exit command.
 *
 * Key Functions:
 *   - sigtstpHandler: Displays a reminder message when the SIGTSTP signal (Ctrl+Z) is received.
 *   - handle_exit: Sends the quit command to the BlinkDB server and terminates the client.
 *   - handleSigInt: Triggered by SIGINT (Ctrl+C) and calls handle_exit to perform a cleanup before exit.
 *   - main: Sets up the client, registers signal handlers, and enters an interactive loop to process user input.
 *  * This client allows users to send commands to the BlinkDB server using the RESP-2 protocol.
 * The supported commands are:
 *
 * - **SET key value**: Store a key-value pair in the database.
 * - **GET key**: Retrieve the value associated with a key.
 * - **DEL key**: Delete a key from the database.
 * - **exit**: Exit the client.
 * - **print**: Print the contents of the database.
 * ## Usage Examples:
 * 
 * @code
 * ./client_runner <mode>
 * @endcode
 * 
 * Once inside the interactive client, you can type commands as follows:
 * 
 * ```
 * > SET A 10
 * +OK
 * 
 * > GET A
 * 10
 * > DEL A
 * +OK
 * 
 * ```
 * 
 * If running in "Client-Server Mode" the client connects to the BlinkDB server running on the preconfigured port (**9001**).
 * 
 * ## Command Format:
 * The client serializes commands into the **RESP2 protocol format** before sending them:
 * 
 * - **SET A 10** → `*3\r\n$3\r\nSET\r\n$1\r\nA\r\n$2\r\n10\r\n`
 * - **GET A** → `*2\r\n$3\r\nGET\r\n$1\r\nA\r\n`
 * - **DEL A** → `*2\r\n$3\r\nDEL\r\n$1\r\nA\r\n`
 *
 * The server responds with a result (OK, a value, or an error message (-ERR)).
 *
 * @note The client communicates with the server using a custom RESP2 protocol format.
 */
#include <iostream>
#include <string>
#include <csignal>
#include <unistd.h>
#include <termios.h>
#include <limits>

#include "lib/blinkdb_client.h"
#include "lib/resp_parser.h"
#include "lib/blinkdb.h"
#include "lib/utils.h"

#define PORT 9001

std::string response = "\nPlease enter 'exit' to exit the client\n";
const std::string quit = "*1\r\n$4\r\nQUIT\r\n";
blinkdb_client client;
int mode = 0;

void sigtstpHandler(int signum) {
    std::cout << response;
}

void handle_exit(){
    restore_echoctl();
    if(mode == 2){
        std::string response = client.send_request(quit);
        if(response == "+OK\r\n"){
            std::cout << "Exiting the client\n";
            std::remove("data.log");
            exit(0);
        }
    }
    std::cin.clear();
    std:: cout << "Failed to send server that the client is exiting. Memory leak possible. Exiting  btly...\n";
    exit(-1);
}

void handleSigInt(int signum){
    restore_echoctl();
    handle_exit();
}

std::string to_lower(std::string s){
    std::string temp = "";
    for(char c : s){
        temp += tolower(c);
    }
    return temp;
}

int main(int argc, char* argv[]) {
    if(argc != 2){
        std::cout << "Usage: " << argv[0] << " <mode> (can be one of 1 or 2)" << std::endl;
        return -1;
    }
    try{
        mode = std::stoi(argv[1]);
    } catch(std::exception &e){
        std::cout << e.what() << std::endl;
    }
    if(mode < 1 || mode > 2){
        std::cout << "Invalid mode. It can either be:\n1-> Interact with blinkdb locally.\n2-> Interact with it over the server\n";
        return -1;
    }
    disable_echoctl();
    resp_parser parser;
    parse_op ip;
    signal(SIGINT, handleSigInt);

    signal(SIGTSTP, sigtstpHandler);  /* Handle Ctrl+Z */
    if(mode == 2){
        client = blinkdb_client("127.0.0.1", PORT);
        if (!client.connect_to_server()) {
            std::cerr << "Failed to connect to server.\n";
            restore_echoctl();
            return 1;
        }
        while (true) {
            std::cout << "User> ";
            std::string input;
            getline(std::cin, input);
            if(input.empty()){
                std::cout << "Please enter a valid command\n";
                continue;
            }
            if(input == "exit"){
                handle_exit();
            }
            parser.parse(input, &ip, mode);  /* Fill ip->resp_str */
            if (ip.resp_str.empty())
                continue;

            std::string response = client.send_request(ip.resp_str);
            if(to_lower(input.substr(0,3)) == "get")
                parser.get_val(response);
            std::cout << response << std::endl;
        }
    } else {
        blinkdb db(10000);
        while (true) {
            std::cout << "User> ";
            std::string input;
            getline(std::cin, input);
            if(input.empty()){
                std::cout << "Please enter a valid command\n";
                continue;
            }
            if(input == "exit"){
                std::cout << "Exiting...\n" << std::endl;
                return 1;
            }
            parser.parse(input, &ip, mode);  /* Fill ip with appropriate values */
            if (ip.cmd.empty()){
                std::cout << "Invalid command. Please have a look at documentation for supported command types and their usage.\n";
                continue;
            }
            client.execute(db, &ip);
        }
    }

    return 0;
}
