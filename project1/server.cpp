// Simple server for TSAM-409 Programming Assignment 1
//
// Compile: g++ -Wall -std=c++11 server.cpp
// Command line: ./server 5000 
//
// Author(s):
// Jacky Mallett (jacky@ru.is)
// Einar Örn Gissurarson (einarog05@ru.is)
// Stephan Schiffel (stephans@ru.is)
// Last modified: 02.09.2021 by Kristofer Gauti Þórhallsson
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <algorithm>
#include <set>
#include <vector>
#include <signal.h>
#include <iostream>
#include <sstream>
#include <thread>

#ifndef SOCK_NONBLOCK
#include <fcntl.h>
#endif

#define BACKLOG  5          // Allowed length of queue of waiting connections

// predefined replies from the server
char SUCCESS_MESSAGE[] = "Command executed successfully\n"; 
char ERROR_MESSAGE[] = "Error executing command\n"; 
char MALFORMED_MESSAGE[] = "Unknown command\n"; 

//Global variable for graceful shutdown
int listenSock;   // Socket for connections to server

// Somewhat gracefully terminate program when user presses ctrl+c
void signal_handler( int signal_n)
{
    if (signal_n == SIGINT)
    {
        printf("\nShutting down...\n");
        //Close the listening socket
        shutdown(listenSock, SHUT_RDWR);
        close(listenSock);
        exit(signal_n);
    }
}

std::set<int> clients; // set of currently open client sockets (file descriptors)

// Open socket for specified port.
//
// Returns -1 if unable to create the socket for any reason.

int open_socket(int portno)
{
	struct sockaddr_in sk_addr;   // address settings for bind()
	int sock;                     // socket opened for this port
	int set = 1;                  // for setsockopt

	// Create socket for connection. Note: OSX doesn´t support SOCK_NONBLOCK
	// so we have to use a fcntl (file control) command there instead. 

	#ifndef SOCK_NONBLOCK
		if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		{
			perror("Failed to open socket");
			return(-1);
		}

		int flags = fcntl(sock, F_GETFL, 0);

		if(fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0)
		{
			perror("Failed to set O_NONBLOCK");
		}
	#else
		if((sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK , IPPROTO_TCP)) < 0)
		{
			perror("Failed to open socket");
			return(-1);
		}
	#endif

	// Turn on SO_REUSEADDR to allow socket to be quickly reused after 
	// program exit.

	if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set)) < 0)
	{
		perror("Failed to set SO_REUSEADDR:");
	}

	// Initialise memory
	memset(&sk_addr, 0, sizeof(sk_addr));

	// Set type of connection

	sk_addr.sin_family      = AF_INET;
	sk_addr.sin_addr.s_addr = INADDR_ANY;
	sk_addr.sin_port        = htons(portno);

	// Bind to socket to listen for connections from clients

	if(bind(sock, (struct sockaddr *)&sk_addr, sizeof(sk_addr)) < 0)
	{
		perror("Failed to bind to socket:");
		return(-1);
	}
	else
	{
		return(sock);
	}
}

// Close a client's connection, remove it from the client list, and
// tidy up select sockets afterwards.

void closeClient(int clientSocket, fd_set *openSockets, int *maxfds)
{
    close(clientSocket);      

    // If this client's socket is maxfds then the next lowest
    // one has to be determined. Socket fd's can be reused by the Kernel,
    // so there aren't any nice ways to do this.

    if(*maxfds == clientSocket)
    {
      if(clients.empty())
      {
        *maxfds = listenSock;
      }
      else
      {
        // clients is a sorted set, so the last element is the largest
        *maxfds = std::max(*clients.rbegin(), listenSock);
      }
    }

     // And remove from the list of open sockets.

     FD_CLR(clientSocket, openSockets);
}

// send a message to a client
// Note: This is not done well. E.g., send might fail if the send buffer is
// full. In that case the remaining message should be sent once the socket
// accepts more data (i.e., once it is returned in writefds by select).

void sendToClient(int clientSocket, const char *message)
{
    if (send(clientSocket, message, strlen(message), 0) < 0)
    {
        perror("Error sending message to client\n");
    }

	//clear the buffer if the buffer is full

}

//Executes a linux command and gets the output of the command as a string
//so it can be sent to the client
//The code is from
//https://www.tutorialspoint.com/How-to-execute-a-command-and-get-the-output-of-command-within-Cplusplus-using-POSIX
std::string exec(std::string command) {
	char buffer[128];
	std::string result = "";

	// Open pipe to file
	FILE* pipe = popen(command.c_str(), "r");
	if (!pipe) {
		return "popen failed!";
	}

	// read till end of process:
	while (!feof(pipe)) {

		// use buffer to read and add to result
		if (fgets(buffer, 20000, pipe) != NULL)
			result += buffer;
	}

	pclose(pipe);
	return result;
}


// Process any message received from client on the server
void clientCommand(int clientSocket, fd_set *openSockets, int *maxfds, 
                  char *buffer) 
{
	std::vector<std::string> tokens;     // List of tokens in command from client
	std::string token;                   // individual token being parsed

	// Split command from client into tokens for parsing
	std::stringstream stream(buffer);

	// By storing them as a vector - tokens[0] is first word in string
	while(stream >> token) {
		tokens.push_back(token);
	}

	if((tokens.size() >= 2) && (tokens[0].compare("SYS") == 0))
	{
		// This assumes that the supplied command has parameters
		std::string command = "";
		for(int i = 1; i <= tokens.size()-1; i++) {
			command += tokens.at(i);
			command += " ";
		}
		std::string response = exec(command);
		if (response.empty()) {
			sendToClient(clientSocket, ERROR_MESSAGE);
		}
		else {
			sendToClient(clientSocket, response.c_str());
		}
	}
	else {
		std::cout << "Unknown command from client:" << buffer << std::endl;
		sendToClient(clientSocket, MALFORMED_MESSAGE);
	}
}

int main(int argc, char* argv[])
{
    bool finished;
    int clientSock;                 // Socket of connecting client
    fd_set openSockets;             // Current open sockets 
    fd_set readSockets;             // Socket list for select()        
    fd_set exceptSockets;           // Exception socket list
    int maxfds;                     // Passed to select() as max fd in set
    struct sockaddr_in client;      // address of incoming client
    socklen_t clientLen;            // address length
    char buffer[1025];              // buffer for reading from clients
    std::vector<int> clientSocketsToClear; // List of closed sockets to remove

    if(argc != 2)
    {
        printf("Usage: server <ip port>\n");
        exit(0);
    }

    // Setup socket for server to listen to

    listenSock = open_socket(atoi(argv[1])); 

    printf("Listening on port: %d\n", atoi(argv[1]));
    printf("       (range of available ports on skel.ru.is is 4000-4100)\n");

    if(listen(listenSock, BACKLOG) < 0)
    {
        printf("Listen failed on port %s\n", argv[1]);
        exit(0);
    }
    else 
    // Add the listen socket to socket set
    {
        FD_SET(listenSock, &openSockets);
        maxfds = listenSock;
    }

    finished = false;

    //Attempt to gracefully shut down program on signal interrupt (ctrl+c)
    if (signal(SIGINT, signal_handler) == SIG_ERR)
    {
        printf("\nCannot catch SIGINT!\n");
    }

    while(!finished)
    {
        // Get modifiable copy of readSockets
        readSockets = exceptSockets = openSockets;
        memset(buffer, 0, sizeof(buffer));

        int n = select(maxfds + 1, &readSockets, NULL, &exceptSockets, NULL);

        if(n < 0)
        {
            perror("select failed - closing down\n");
            finished = true;
        }
        else
        {
            // Accept  any new connections to the server
            if(FD_ISSET(listenSock, &readSockets))
            {
               clientSock = accept(listenSock, (struct sockaddr *)&client,
                                   &clientLen);

               FD_SET(clientSock, &openSockets);
               maxfds = std::max(maxfds, clientSock);

               clients.insert(clientSock);
               n--;

               printf("Client connected on server\n");
            }
            // Check for commands from already connected clients
            while(n-- > 0)
            {
               for(auto fd : clients)
               {
                  if(FD_ISSET(fd, &readSockets))
                  {
                      if(recv(fd, buffer, sizeof(buffer), MSG_DONTWAIT) == 0)
                      {
                          printf("Client closed connection: %d", fd);

                          closeClient(fd, &openSockets, &maxfds);
                          // Remember that we need to remove the client from the list.
                          // Removing will be done outside the loop.
                          clientSocketsToClear.push_back(fd);

                      }
                      else // if something was recieved from the client
                      {
                          std::cout << buffer << std::endl;
                          // Attempt to execute client command
                          clientCommand(fd, &openSockets, &maxfds, buffer);
                      }
                  }
               }
               // Remove client from the clients list. This has to be done out of
               // the loop, since we can't modify the iterator inside the loop.
               for(auto fd : clientSocketsToClear)
               {
                   clients.erase(fd);
               }
               clientSocketsToClear.clear();
            }
        }
    }
}
