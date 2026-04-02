// cd /workspaces/OS-VM/Task_7_networkchat && g++ server.cpp -o server -pthread && g++ client.cpp -o client -pthread
#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
using namespace std;

// Listening thread (so that the server can recieve messages at any time)
void receive_thread_func(int client_socket) {
    char buffer[4096]; // store the incoming data (max. 4096)
    while (true) { // always listen until connetcion broken
        memset(buffer, 0, 4096); // reset the buffer memory with 0's (null) so old message doesnt appear in new message
        int bytes_received = recv(client_socket, buffer, 4096, 0); // record how many bytes were recieved from the client socket
        
        // 0 = client disconnected, -1 = network error
        if (bytes_received <= 0) {
            cout << "\r[System] Client disconnected. Press Enter to exit." << endl;
            exit(0);
        }

        cout << "\r" << string(50, ' ') << "\r"; 
        // \r moves the cursor to the start of the current line, 
        // prints 50 blank spaces (it "erases" the Server (You): prompt)
        // then moves the cursor back to the start again so the message starts at the left 
        cout << "Client: " << buffer << endl; // print the client message
        cout << "Server (You): " << flush; // put the prompt back 
    }
}

int main() {
    // 1. Creates a socket endpoint and returns a file descriptor. 
    // AF_INET sets the "language" to IPv4, and SOCK_STREAM sets the "method" to TCP.
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    int opt = 1;
    // 2. modify server at SOL_SOCKET (general) level
    // SO_REUSEADDR allows the socket to bind to a port that is still in a TIME_WAIT state to reserve the port if its shut down 
    // & handle any packets in transit. Otherwise port would take time to restart.
    // &opt is the address storing 1 (enable the feature) & sizeof is to specify how many byte sto read at this address
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 3. configure server address structure
    sockaddr_in address;
    // Address Family: Internet (IPv4)
    address.sin_family = AF_INET; 
     // define which IP address the server should "bind" to (all (wifi and ethernet))
    address.sin_addr.s_addr = INADDR_ANY;
    // convert port number from little endian byte order to network's big endian format for correct interpretation by router
    address.sin_port = htons(8080); 

    // 3. OS checks is the port is available & bind the program there to redirect any data to that port
    // 2nd argument is to convert the IPv4-specific sockaddr_in to a generic sockaddr pointer that works for any type (e.g. IPv6) 
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        cerr << "Bind failed. Use 'fuser -k 8080/tcp' to clear the port." << endl;
        return -1;
    }

    // 4. listen & accept
    // socket tells the OS its reasy to recieve (max. length of pending connections is 3)
    listen(server_fd, 3); 
    cout << "Waiting for a client..." << endl;
    
    // pauses the program until a client connects, nullptr = ignore the client's identity 
    // returns a new, dedicated socket specifically for communicating with that person.
    int client_socket = accept(server_fd, nullptr, nullptr);
    cout << "Connected! Real-time sync active.\n" << endl;

    // 5. multithreading
    // create background worker thread to run receive_thread_func using the dedicated client_socket
    // enables 'realtime' feature
    thread listener(receive_thread_func, client_socket);
    // // tells the OS to let the thread run independantly 
    listener.detach(); 

    // 6. handle user input
    string msg; // buffer to store the text being typed into the console
    while (true) { 
        cout << "Server (You): " << flush; // flush forces the output to display immediately without waiting for a newline
        if (!getline(cin, msg)) break; // read full line of text from the keyboard & if cin fails, break
        if (!msg.empty()) { // avoid empty packets 
            // send message as C-style character array across the network to the connected client
            send(client_socket, msg.c_str(), msg.length(), 0);
        }
    }

    // 7. shut down server
    // shut down ability to listen for new clients and release the file descriptor back to the OS to prevent resource leaks
    close(server_fd);
    return 0;
}