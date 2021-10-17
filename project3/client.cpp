/**
 * Simple chat client for TSAM-409
 * Command line: ./chat_client 4000 
 * Modified code from a template provided by Jacky Mallett (jacky@ru.is)
 * Authors: Bergur Tareq Tamimi & Kristofer Gauti Þórhallsson
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
#include <thread>

#include <iostream>
#include <sstream>
#include <thread>
#include <map>

// Threaded function for handling responss from server
void listenServer(int serverSocket)
{
    int nread;         // Bytes read from socket
    char buffer[1025]; // Buffer for reading input

    while (true)
    {
        // clear the buffer
        memset(buffer, 0, sizeof(buffer));
        // read the response
        nread = read(serverSocket, buffer, sizeof(buffer));

        // if there is no message then we stop
        if (nread == 0)
        { // Server has dropped us
            printf("Over and Out\n");
            exit(0);
        }
        // if there is message we read it from the buffer
        else if (nread > 0)
        {
            printf("%s\n", buffer);
        }
        printf("here\n");
    }
}

int main(int argc, char *argv[])
{
    struct addrinfo hints, *svr;  // Network host entry for server
    struct sockaddr_in serv_addr; // Socket address for server
    int serverSocket;             // Socket used for server
    int nwrite;                   // No. bytes written to server
    char buffer[1025];            // buffer for writing to server
    bool finished;
    int set = 1; // Toggle for setsockopt
    int index = 0;
    char newBuffer[2050];

    if (argc != 3)
    {
        printf("Usage: ./chat_client <your_local_ip_address> <ip port>\n");
        printf("Ctrl-C to terminate\n");
        exit(0);
    }

    hints.ai_family = AF_INET; // IPv4 only addresses
    hints.ai_socktype = SOCK_STREAM;
    memset(&hints, 0, sizeof(hints));
    if (getaddrinfo(argv[1], argv[2], &hints, &svr) != 0)
    {
        perror("getaddrinfo failed: ");
        exit(0);
    }

    // hostent is used to store informations about a given host
    struct hostent *server;
    server = gethostbyname(argv[1]); // gethostbyname() returns a pointer to the hostent

    // SERV_ADDR
    // clear serv_addr
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    // copy h_addr to ther serv_addr.sin_addr.s_addr
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(atoi(argv[2]));

    // create server socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    // Turn on SO_REUSEADDR to allow socket to be quickly reused after
    // program exit.
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set)) < 0)
    {
        printf("Failed to set SO_REUSEADDR for port %s\n", argv[2]);
        perror("setsockopt failed: ");
    }

    // connect to server using the socket and serv_addr
    if (connect(serverSocket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        // EINPROGRESS means that the connection is still being setup. Typically this
        // only occurs with non-blocking sockets. (The serverSocket above is explicitly
        // not in non-blocking mode, so this check here is just an example of how to
        // handle this properly.)
        if (errno != EINPROGRESS)
        {
            printf("Failed to open socket to server: %s\n", argv[1]);
            perror("Connect failed: ");
            exit(0);
        }
    }

    // Listen and print replies from server
    std::thread serverThread(listenServer, serverSocket);
    finished = false;
    while (!finished)
    {
        // clear buffers
        bzero(buffer, sizeof(buffer));
        bzero(newBuffer, sizeof(newBuffer));

        // take input on the buffer
        fgets(buffer, sizeof(buffer), stdin);

        newBuffer[0] = 0x02;
        for (int i = 1; i < strlen(buffer); i++)
        {
            newBuffer[i] = buffer[index];
            index++;
        }
        newBuffer[strlen(buffer)] = 0x03;

        index = 0;

        // send and use nwrite to see if there was any reply
        nwrite = send(serverSocket, newBuffer, strlen(newBuffer), 0);

        if (nwrite == -1)
        {
            perror("send() to server failed: ");
            finished = true;
        }
    }
}
