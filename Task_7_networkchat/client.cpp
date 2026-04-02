#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
using namespace std;

// Listening thread (so that the server can recieve messages at any time)
void receive_thread_func(int server_socket) {
    char buffer[4096]; 
    while (true) {
        memset(buffer, 0, 4096);
        int bytes_received = recv(server_socket, buffer, 4096, 0);
        
        if (bytes_received <= 0) {
            cout << "\r[System] Server shut down. Press Enter to exit." << endl;
            exit(0);
        }

        cout << "\r" << string(50, ' ') << "\r";
        cout << "Server: " << buffer << endl;
        cout << "Client (You): " << flush;
    }
}

int main() {

    // 1. create a socket & configure server address structure
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);

    // 2. Presentation to Network function 
    // converts IP address string "127.0.0.1" into binary that network stack can process. 
    // stores the binary value in the sin_addr field of the server address structure to define the connection destination
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    // 3. try to connect with the servers port & IP
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "Connection failed. Is the server running?" << endl;
        return -1;
    }

    cout << "Connected! Real-time sync active.\n" << endl;

    // 4. multithreading for realtime messaging
    thread listener(receive_thread_func, sock);
    listener.detach();

    // 5. handle input
    string msg;
    while (true) {
        cout << "Client (You): " << flush;
        if (!getline(cin, msg)) break;
        if (!msg.empty()) {
            send(sock, msg.c_str(), msg.length(), 0);
        }
    }

    // 6. shut down
    close(sock);
    return 0;
}