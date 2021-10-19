/**
 * Simple chat server for TSAM-409 project 3
 * Command line: ./server <inet_ip> <port_number> 
 * Modified code from a template provided by Jacky Mallett (jacky@ru.is)
 * Authors: Anton Björn Mayböck Helgason, Bergur Tareq Tamimi & Kristofer Gauti Þórhallsson
 */
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
#include <map>
#include <vector>
#include <list>
#include <iostream>
#include <sstream>
#include <thread>
#include <map>
#include <unistd.h>
#include <net/if.h>
#include <ifaddrs.h>

//Global variables
#define CHUNK_SIZE 512

// fix SOCK_NONBLOCK for OSX
#ifndef SOCK_NONBLOCK
#include <fcntl.h>
#define SOCK_NONBLOCK O_NONBLOCK
#endif

#define BACKLOG 5 // Allowed length of queue of waiting connections

/**
 * Simple class for handling connections from clients.
 * Client(int socket) - socket to send/receive traffic from client.
 */
class Client
{
public:
	int sock;		  // socket of client connection
	std::string name; // Limit length of name of client's user
	std::string ipaddr;
	std::string portnr;
	Client(int socket) : sock(socket) {}
	~Client() {} // Virtual destructor defined for base class
};

/**
 * Note: map is not necessarily the most efficient method to use here,
 * especially for a server with large numbers of simulataneous connections,
 * where performance is also expected to be an issue.
 * Quite often a simple array can be used as a lookup table, 
 * (indexed on socket no.) sacrificing memory for speed.
 * 
 * Lookup table for per Client information
 */
std::map<int, Client *> clients;


std::string get_local_ip(){
	struct ifaddrs *myaddrs, *ifa;
    void *in_addr;
    char buf[64];

    if(getifaddrs(&myaddrs) != 0)
    {
        perror("getifaddrs");
        exit(1);
    }

    for (ifa = myaddrs; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;
        if (!(ifa->ifa_flags & IFF_UP))
            continue;

        switch (ifa->ifa_addr->sa_family)
        {
            case AF_INET:
            {
                struct sockaddr_in *s4 = (struct sockaddr_in *)ifa->ifa_addr;
                in_addr = &s4->sin_addr;
                break;
            }

            case AF_INET6:
            {
                struct sockaddr_in6 *s6 = (struct sockaddr_in6 *)ifa->ifa_addr;
                in_addr = &s6->sin6_addr;
                break;
            }

            default:
                continue;
        }

        if (!inet_ntop(ifa->ifa_addr->sa_family, in_addr, buf, sizeof(buf)))
        {
            printf("%s: inet_ntop failed!\n", ifa->ifa_name);
			exit(1);
        }
        else
        {
            if ((std::string)ifa->ifa_name == "en0") {
                return (std::string)buf;
            }
          
        }
    }
    freeifaddrs(myaddrs);
	return 0;
}

/**
 * Open socket for specified port.
 * @return -1 if unable to create the socket for any reason.
 */
int open_socket(int portno, bool is_server_socket)
{
	struct sockaddr_in sk_addr; // address settings for bind()
	int sock;					// socket opened for this port
	int set = 1;				// for setsockopt

	// Create socket for connection. Set to be non-blocking, so recv will
	// return immediately if there isn't anything waiting to be read.
#ifdef __APPLE__
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Failed to open socket");
		return (-1);
	}
#else
	if ((sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0)
	{
		perror("Failed to open socket");
		return (-1);
	}
#endif
	// Turn on SO_REUSEADDR to allow socket to be quickly reused after
	// program exit.
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set)) < 0)
	{
		perror("Failed to set SO_REUSEADDR:");
	}
	set = 1;
#ifdef __APPLE__
	if (setsockopt(sock, SOL_SOCKET, SOCK_NONBLOCK, &set, sizeof(set)) < 0)
	{
		perror("Failed to set SOCK_NOBBLOCK");
	}
#endif
	memset(&sk_addr, 0, sizeof(sk_addr));

	sk_addr.sin_family = AF_INET;
	sk_addr.sin_addr.s_addr = INADDR_ANY;
	sk_addr.sin_port = htons(portno);

	if (is_server_socket) {
		// Bind to socket to listen for connections from clients
		if (bind(sock, (struct sockaddr *)&sk_addr, sizeof(sk_addr)) < 0)
		{
			perror("Failed to bind to socket:");
			return (-1);
		}
	}
	return sock;
}

/**
 * Close a client's connection, remove it from the client list, and
 * tidy up select sockets afterwards.
 * 
 * If this client's socket is maxfds then the next lowest
 * one has to be determined. Socket fd's can be reused by the Kernel,
 * so there aren't any nice ways to do this.
 */
void closeClient(int clientSocket, fd_set *openSockets, int *maxfds)
{
	printf("Client closed connection: %d\n", clientSocket);

	close(clientSocket);

	if (*maxfds == clientSocket)
	{
		for (auto const &p : clients)
		{
			*maxfds = std::max(*maxfds, p.second->sock);
		}
	}

	// And remove from the list of open sockets.
	FD_CLR(clientSocket, openSockets);
}

// Process command from client on the server
void clientCommand(int clientSocket, fd_set *openSockets, int *maxfds, char *buffer, std::string src_port)
{
	// we might have to change the size of the tokens vector
	std::vector<std::string> tokens(4);
	std::string token;

	// Split command from client into tokens for parsing
	std::stringstream stream(buffer);
	int j = 0;

	for (int i = 0; i <= strlen(buffer); i++)
	{

		if (buffer[i] == ',' || i == strlen(buffer))
		{
			tokens[j] = token;
			j += 1;
			token = "";
		}
		else
		{
			token.push_back(buffer[i]);
		}
	}

	// std::cout << "Printing each element in tokens list" << std::endl;
	// for (auto v : tokens)
	// {
	// 	std::cout << v << std::endl;
	// }
	while (stream >> token)
	{
		tokens.push_back(token);
	}

	//CONNECT,<Group id>,<IP_address>,<port number>       CONNECT,P3_GROUP_7,130.208.243.61,4001
	if ((tokens[0].compare("CONNECT") == 0))
	{
		clients[clientSocket]->name = tokens[1];
		clients[clientSocket]->ipaddr = tokens[2];
		clients[clientSocket]->portnr = tokens[3];
		
		//Establish a connection
		struct sockaddr_in server_addr; 											//Declare Server Address
		server_addr.sin_family = AF_INET; 											//IPv4 address family
		server_addr.sin_addr.s_addr = INADDR_ANY;									//Bind Socket to all available interfaces					
		server_addr.sin_port = htons(atoi(clients[clientSocket]->portnr.c_str()));	//Convert the ASCII port number to integer port number

		//Check for errors for set socket address
		if(inet_pton(AF_INET, clients[clientSocket]->ipaddr.c_str(), &server_addr.sin_addr) <= 0) 
		{
			printf("\nInvalid address/ Address not supported \n");
			exit(0);
		}

		//Create a tcp socket
		int connection_socket = open_socket(stoi(clients[clientSocket]->portnr), false);

		//Check if the connection was successful
		int connection_successful = connect(connection_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
		if (connection_successful < 0)
		{
			printf("\nConnection failed \n");
			exit(0);
		}
		printf("Connection successful!\n");

		char temp_buffer[128];
		char send_buffer[128];
		char receive_buffer[CHUNK_SIZE];
		int index = 0;
		std::string local_ip = get_local_ip();
		std::string message = "QUERYSERVERS,P3_GROUP_7," + local_ip + "," + src_port;

		strcpy(temp_buffer, message.c_str());
		send_buffer[0] = 0x02;
        for (int i = 1; i < strlen(temp_buffer); i++)
        {
            send_buffer[i] = temp_buffer[index];
            index++;
        }
        send_buffer[strlen(temp_buffer)] = 0x03;

		if (send(connection_socket, send_buffer, sizeof(send_buffer), 0) < 0) {
			perror("No message was sent!");
		}
		while(true) {
			memset(receive_buffer ,0 , CHUNK_SIZE);
			if (recv(connection_socket, receive_buffer, CHUNK_SIZE, 0) < 0) {
				perror("Message was received!");
				break;
			}
			else {
				std::cout << receive_buffer << std::endl;
			}
		}


	}
	else if (tokens[0].compare("LEAVE") == 0)
	{
		// Close the socket, and leave the socket handling
		// code to deal with tidying up clients etc. when
		// select() detects the OS has torn down the connection.
		closeClient(clientSocket, openSockets, maxfds);
	}

	else if (tokens[0].compare("QUERYSERVERS") == 0)
	{
		// some query stuff
		std::cout << "I am a message from QUERYSERVERS" << std::endl;
	}
	else if (tokens[0].compare("KEEPALIVE") == 0)
	{
		// some KEEPALIVE stuff
		std::cout << "I am a message from KEEPALIVE" << std::endl;
	}
	else if (tokens[0].compare("FETCH_MSGS") == 0)
	{
		// some FETCH MSGS stuff
		std::cout << "I am a message from FETCH_MSGS" << std::endl;
	}
	else if (tokens[0].compare("SEND_MSG") == 0)
	{
		// some SEND MSG stuff
		std::cout << "I am a message from SEND_MSG" << std::endl;
	}
	else if (tokens[0].compare("STATUSREQ") == 0)
	{
		// some STATUSREQ stuff
		std::cout << "I am a message from STATUSREQ" << std::endl;
	}
	else if (tokens[0].compare("STATUSRESP") == 0)
	{
		// some STATUSRESP stuff
		std::cout << "I am a message from STATUSRESP" << std::endl;
	}


	else
	{
		std::cout << "Unknown command from client:" << buffer << std::endl;
	}
}

int main(int argc, char *argv[])
{
	bool finished;
	int listenSock;		  // Socket for connections to server
	int clientSock;		  // Socket of connecting client
	fd_set openSockets;	  // Current open sockets
	fd_set readSockets;	  // Socket list for select()
	fd_set exceptSockets; // Exception socket list
	int maxfds;			  // Passed to select() as max fd in set
	struct sockaddr_in client;
	socklen_t clientLen;
	char buffer[1025]; // buffer for reading from clients

	if (argc != 2)
	{
		printf("Usage: chat_server <ip port>\n");
		exit(0);
	}

	// Setup socket for server to listen to
	listenSock = open_socket(atoi(argv[1]), true);
	printf("Listening on port: %d\n", atoi(argv[1]));

	if (listen(listenSock, BACKLOG) < 0)
	{
		printf("Listen failed on port %s\n", argv[1]);
		exit(0);
	}
	else
	{
		// Add listen socket to socket set we are monitoring
		FD_ZERO(&openSockets);
		FD_SET(listenSock, &openSockets);
		maxfds = listenSock;
	}

	finished = false;

	while (!finished)
	{
		// Get modifiable copy of readSockets
		readSockets = exceptSockets = openSockets;
		memset(buffer, 0, sizeof(buffer));

		// Look at sockets and see which ones have something to be read()
		int n = select(maxfds + 1, &readSockets, NULL, &exceptSockets, NULL);

		if (n < 0)
		{
			perror("select failed - closing down\n");
			finished = true;
		}
		else
		{
			// First, accept  any new connections to the server on the listening socket
			if (FD_ISSET(listenSock, &readSockets))
			{
				clientSock = accept(listenSock, (struct sockaddr *)&client,
									&clientLen);
				printf("accept***\n");
				// Add new client to the list of open sockets
				FD_SET(clientSock, &openSockets);

				// And update the maximum file descriptor
				maxfds = std::max(maxfds, clientSock);

				// create a new client to store information.
				clients[clientSock] = new Client(clientSock);

				// Decrement the number of sockets waiting to be dealt with
				n--;

				printf("Client connected on server: %d\n", clientSock);
			}
			// Now check for commands from clients
			std::list<Client *> disconnectedClients;
			while (n-- > 0)
			{
				for (auto const &pair : clients)
				{
					Client *client = pair.second;

					if (FD_ISSET(client->sock, &readSockets))
					{
						// recv() == 0 means client has closed connection
						if (recv(client->sock, buffer, sizeof(buffer), MSG_DONTWAIT) == 0)
						{
							disconnectedClients.push_back(client);
							closeClient(client->sock, &openSockets, &maxfds);
						}
						// We don't check for -1 (nothing received) because select()
						// only triggers if there is something on the socket for us.
						else
						{
							if (buffer[0] == 0x02 && buffer[strlen(buffer) - 1] == 0x03)
							{
								char newBuffer[strlen(buffer)];
								int index = 0;
								for (int i = 1; i < strlen(buffer) - 1; i++)
								{
									newBuffer[index] = buffer[i];
									index++;
								}

								clientCommand(client->sock, &openSockets, &maxfds, newBuffer, (std::string)argv[1]);
							}
							else
							{
								std::cout << "Nothing received" << std::endl;
							}
						}
					}
				}
				// Remove client from the clients list
				for (auto const &c : disconnectedClients)
					clients.erase(c->sock);
			}
		}
	}
}
