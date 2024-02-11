//#include <stdio.h>
//#include <stdlib.h>
#include <iostream>
#include <cstring>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include<map>

using namespace std;

int main(int argc, char ** argv) {
    map<string, string> KV_DATASTORE;
    int portno; /* port to listen on */
  
    /* 
     * check command line arguments 
     */
    if (argc != 2) {
        cerr << "usage: " << argv[0] << " <port>" << endl;
        exit(1);
    }

    // Server port number taken as command line argument
    portno = atoi(argv[1]);

    // Create a socket
    int listening = socket(AF_INET, SOCK_STREAM, 0);
    if (listening == -1)
    {
        cerr << "Can't create a socket! Quitting" << endl;
        return -1;
    }
 
    // Bind the ip address and port to a socket
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(portno);
    inet_pton(AF_INET, "0.0.0.0", &hint.sin_addr);
 
    if (bind(listening, (sockaddr*)&hint, sizeof(hint)) == -1) {
        cerr << "Error binding socket to port" << endl;
        return -1;
    };
 
    // Tell Winsock the socket is for listening
    if (listen(listening, SOMAXCONN) == -1) {
        cerr << "Error listening for connections" << endl;
        return -1;
    };
 
    // Wait for a connection
    sockaddr_in client;
    socklen_t clientSize = sizeof(client);
 
    int clientSocket = accept(listening, (sockaddr*)&client, &clientSize);
 
    char host[NI_MAXHOST];      // Client's remote name
    char service[NI_MAXSERV];   // Service (i.e. port) the client is connect on
 
    memset(host, 0, NI_MAXHOST); // same as memset(host, 0, NI_MAXHOST);
    memset(service, 0, NI_MAXSERV);
 
    if (getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0) {
        cout << host << " connected on port " << service << endl;
    } else {
        inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
        cout << host << " connected on port " << ntohs(client.sin_port) << endl;
    }
 
    // Close listening socket
    close(listening);
 
    // While loop: accept and echo message back to client
    char buf[4096];
 
    while (true) {
        memset(buf, 0, 4096);
 
        // Wait for client to send data
        int bytesReceived = recv(clientSocket, buf, 4096, 0);
        if (bytesReceived == -1) {
            cerr << "Error in recv(). Quitting" << endl;
            break;
        }
 
        if (bytesReceived == 0) {
            cout << "Client disconnected " << endl;
            break;
        }
 
        cout << string(buf, 0, bytesReceived) << endl;
 
        // Process client message
        buf[bytesReceived] = '\0'; // Null-terminate the received message
        char *token = strtok(buf, " ");

        // Extract the command
        if (token != NULL) {
            if (strcmp(token, "READ") == 0) {
                token = strtok(NULL, "\n");
                string key(token);
                string response = KV_DATASTORE.count(key) ? KV_DATASTORE[key] : "NULL";
                send(clientSocket, response.c_str(), response.length(), 0);
            } else if (strcmp(token, "WRITE") == 0) {
                token = strtok(NULL, ":");
                string key(token);
                token = strtok(NULL, "\n");
                string value(token);
                KV_DATASTORE[key] = value;
                send(clientSocket, "OK", 2, 0);
            } else if (strcmp(token, "COUNT") == 0) {
                char count[10];
                sprintf(count, "%lu", KV_DATASTORE.size());
                send(clientSocket, count, strlen(count), 0);
            } else if (strcmp(token, "DELETE") == 0) {
                token = strtok(NULL, "\n");
                string key(token);
                int num_deleted = KV_DATASTORE.erase(key);
                if (num_deleted)
                    send(clientSocket, "FIN", 3, 0);
                else
                    send(clientSocket, "NULL", 4, 0);
            } else if (strcmp(token, "END") == 0) {
                // End the connection
                break;
            }
        }
    }
 
    // Close the socket
    close(clientSocket);
 
    return 0;
}
