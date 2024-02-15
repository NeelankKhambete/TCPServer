#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <map>
#include <string>
#include <cstring>
#include <pthread.h>

using namespace std;

int main(int argc, char ** argv) {
map<string, string> KV_DATASTORE;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

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
if (listening == -1) {
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
}

// Tell Winsock the socket is for listening
if (listen(listening, SOMAXCONN) == -1) {
cerr << "Error listening for connections" << endl;
return -1;
}

// Main server loop
while (true) {
// Accept incoming client connection
sockaddr_in client;
socklen_t clientSize = sizeof(client);
int clientSocket = accept(listening, (sockaddr*)&client, &clientSize);
if (clientSocket == -1) {
cerr << "Error accepting client connection" << endl;
continue;
}

// Create thread to handle client connection
pthread_t thread_id;
int* new_client = new int;
*new_client = clientSocket;
if (pthread_create(&thread_id, NULL, [](void* arg) {
int clientSocket = *((int*)arg);
char buf[4096];

// While loop: accept and process messages from the client
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

cout << "Received: " << string(buf, 0, bytesReceived) << endl;

// Process client message
buf[bytesReceived] = '\0'; // Null-terminate the received message
char *token = strtok(buf, " ");

// Extract the command
if (token != NULL) {
if (strcmp(token, "READ") == 0) {
// Read operation
// Extract key
token = strtok(NULL, "\n");
string key(token);

// Access the datastore (critical section)
pthread_mutex_lock(&mutex);
string response = KV_DATASTORE.count(key) ? KV_DATASTORE[key] : "NULL";
pthread_mutex_unlock(&mutex);

// Send response to client
send(clientSocket, response.c_str(), response.length(), 0);
} else if (strcmp(token, "WRITE") == 0) {
// Write operation
// Extract key and value
token = strtok(NULL, ":");
string key(token);
token = strtok(NULL, "\n");
string value(token);

// Access the datastore (critical section)
pthread_mutex_lock(&mutex);
KV_DATASTORE[key] = value;
pthread_mutex_unlock(&mutex);

// Send response to client
send(clientSocket, "OK", 2, 0);
} else if (strcmp(token, "COUNT") == 0) {
// Count operation
// Access the datastore (critical section)
pthread_mutex_lock(&mutex);
char count[10];
sprintf(count, "%lu", KV_DATASTORE.size());
pthread_mutex_unlock(&mutex);

// Send response to client
send(clientSocket, count, strlen(count), 0);
} else if (strcmp(token, "DELETE") == 0) {
// Delete operation
// Extract key
token = strtok(NULL, "\n");
string key(token);

// Access the datastore (critical section)
pthread_mutex_lock(&mutex);
int num_deleted = KV_DATASTORE.erase(key);
pthread_mutex_unlock(&mutex);

// Send response to client
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

// Close the client socket
close(clientSocket);
delete (int*)arg;
pthread_exit(NULL);
}, (void*)new_client) != 0) {
cerr << "Error creating thread" << endl;
delete new_client;
continue;
}
}

// Close the listening socket
close(listening);

return 0;
}
