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
#include <pthread.h>
#include <map>
#include <unistd.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <sys/select.h>
#include <fstream>

//Global variables
#define CHUNK_SIZE 512
#define GROUP "P3_GROUP_7"

// fix SOCK_NONBLOCK for OSX
#ifndef SOCK_NONBLOCK
#include <fcntl.h>
#define SOCK_NONBLOCK O_NONBLOCK
#endif

#define BACKLOG 5 // Allowed length of queue of waiting connections

#define MAX_CONNECTIONS 15

/**
 * Simple class for handling connections from clients.
 * Client(int socket) - socket to send/receive traffic from client.
 */
class Client
{
public:
    int sock;         // socket of client connection
    std::string name; // Limit length of name of client's user
    std::string ipaddr;
    std::string portnr;
    bool isServer;

    Client(int socket, bool server)
    {
        sock = socket;
        isServer = server;
    }
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

// clients
std::map<int, Client *> clients;
// identified servers
std::map<int, Client *> servers;

std::map<int, Client *> stored_servers;
std::vector<std::string> stored_names;

// connections that do not have all of the informations
std::map<std::string, Client *> connections;

// Incoming messages
std::map<std::string, std::vector<std::string>> messages;

// Outgoing messages
std::map<std::string, std::vector<std::string>> outgoing;

std::ofstream server_log;

bool valid_message(char *buffer)
{
    return buffer[0] == 0x02 && buffer[strlen(buffer) - 1] == 0x03;
}

/*
 * Finds the ip that the server running on has
 * Returns a string of that ip address
 */
std::string get_local_ip()
{
    struct ifaddrs *myaddrs, *ifa;
    void *in_addr;
    char buf[64];

    if (getifaddrs(&myaddrs) != 0)
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
            if ((std::string)ifa->ifa_name == "en0" || (std::string)ifa->ifa_name == "wlp1s0" || (std::string)ifa->ifa_name == "ens192")
            {
                if (std::string(buf).find('.') != std::string::npos)
                {
                    return (std::string)buf;
                }
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
int open_socket(int portno, bool is_server_socket = true)

{
    struct sockaddr_in sk_addr; // address settings for bind()
    int sock;                   // socket opened for this port
    int set = 1;                // for setsockopt

    // Create socket for connection. Set to be non-blocking, so recv will
    // return immediately if there isn't anything waiting to be read.
#ifdef __APPLE__
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Failed to open socket");
        return (-1);
    }
#else
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
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

    if (is_server_socket)
    {
        // Bind to socket to listen for connections from clients
        if (bind(sock, (struct sockaddr *)&sk_addr, sizeof(sk_addr)) < 0)
        {
            perror("Failed to bind to socket:");
            return (-1);
        }
    }
    return sock;
}

std::string DateTime() {
    time_t      now = time(0);
    struct tm   tstruct;
    char        buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);
    return buf;
}

void log_to_file(std::string msg) {
    std::string filePath = "./message_log.txt";
    std::string date = DateTime();
    std::ofstream ofs(filePath.c_str(), std::ios_base::out | std::ios_base::app);
    ofs << date << '\t' << msg << '\n';
    ofs.close();

}

int establish_connection(std::string port_nr, std::string ip_addr)
{


    struct addrinfo hints, *svr;  // Network host entry for server
    struct sockaddr_in serv_addr; // Socket address for server
    int serverSocket;             // Socket used for server
    int set = 1;                  // Toggle for setsockopt

    hints.ai_family = AF_INET; // IPv4 only addresses
    hints.ai_socktype = SOCK_STREAM;

    memset(&hints, 0, sizeof(hints));

    if (getaddrinfo(ip_addr.c_str(), port_nr.c_str(), &hints, &svr) != 0)
    {
        perror("getaddrinfo failed: ");
        return -1;
    }

    struct hostent *server;
    server = gethostbyname(ip_addr.c_str());

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(atoi(port_nr.c_str()));

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    // Turn on SO_REUSEADDR to allow socket to be quickly reused after
    // program exit.

    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set)) < 0)
    {
        printf("Failed to set SO_REUSEADDR for port %s\n", port_nr.c_str());
        perror("setsockopt failed: ");
        return -1;
    }

    int connection_socket = connect(serverSocket, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    if (connection_socket < 0)
    {
        // EINPROGRESS means that the connection is still being setup. Typically this
        // only occurs with non-blocking sockets. (The serverSocket above is explicitly
        // not in non-blocking mode, so this check here is just an example of how to
        // handle this properly.)
        if (errno != EINPROGRESS)
        {
            printf("Failed to open socket to server: %s\n", port_nr.c_str());
            perror("Connect failed: ");
        }
        return -1;
    }
    return connection_socket;
}

/*
 * Take the message that was received and inserts 0x02 at the front and 0x03 at the back
 * Returns a filled out char array with the relevant information
 */
void construct_message(char *send_buffer, std::string message)
{
    char temp_buffer[message.size() + 2];
    strcpy(temp_buffer, message.c_str());
    send_buffer[0] = 0x02;
    int index = 0;
    for (int i = 1; i <= strlen(temp_buffer); i++)
    {
        send_buffer[i] = temp_buffer[index];
        index++;
    }
    send_buffer[strlen(temp_buffer) + 1] = 0x03;
}

void send_queryservers(int connection_socket)
{
    std::string message = "QUERYSERVERS,P3_GROUP_7;";
    char sendBuffer[message.size() + 2];
    construct_message(sendBuffer, message);
    if (send(connection_socket, sendBuffer, message.length() + 2, 0) < 0)
    {
        perror("Sending message failed");
    }
}

/*
 * Split the command received into the relevant server info that is needed
 */
void server_vector(std::string message, std::vector<std::string> *servers_info)
{
    std::stringstream ss(message);
    int index = 0;

    while (ss.good())
    {
        std::string substr;
        std::getline(ss, substr, ';');
        if (index == 0)
        { //Erasing SERVERS from the first string to get the string: groupId,IP,port
            substr = substr.erase(0, 8);
        }

        if (substr.size() != 1)
        { //does not append the last line whereas it is an empty string. Don't ask, it works!!!!
            servers_info->push_back(substr);
        }
        index++;
    }
}
/*
 * Split the tokenized command that we get and insert the port nr into a separate vector
 */

void split_commas(std::vector<std::string> *servers_info, std::vector<std::string> *group_IP_portnr_list)
{
    for (auto server_info : *servers_info)
    {
        std::stringstream ss(server_info);
        std::string str;
        while (getline(ss, str, ','))
        {
            group_IP_portnr_list->push_back(str);
        }
    }
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
    std::string logger = "Client closed connection on server: " + std::to_string(clientSocket);
    log_to_file(logger);

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

bool isStored(std::string id, std::vector<std::string> stored_names)
{
    bool stored = false;
    for (int i = 0; i < stored_names.size(); i++)
    {
        if (stored_names[i] == id)
        {
            stored = true;
            break;
        }
    }
    return stored;
}

void clientCommand(int clientSocket, fd_set *openSockets, int *maxfds, char *buffer, std::string src_port)
{

    if (!valid_message(buffer))
    {
        std::string server_msg = "You are nothing but a scam";
        char send_buffer[server_msg.size() + 2];
        construct_message(send_buffer, server_msg);
        send(clientSocket, send_buffer, server_msg.size() + 2, 0);
        std::cout << "NOT A VALID MESSAGE" << std::endl;
        std::string logger = "NOT A VALID MESSAGE";
        log_to_file(logger);
        return;
    }

    std::string message = std::string(buffer);
    message.erase(0, 1);
    message.erase(message.size() - 1);

    std::vector<std::string> tokens;
    std::string token;
    std::stringstream ss(message);

    while (ss.good())
    {
        getline(ss, token, ',');
        tokens.push_back(token);
    }

    std::string server_msg;

    if (tokens[0].compare("QUERYSERVERS") == 0 && tokens.size() == 1)
    {
        for (auto const &pair : servers)

        {
            Client *client = pair.second;
            std::cout << client->name <<  client->ipaddr << client->portnr << std::endl;

            server_msg += client->name + ",";
        }
        server_msg.pop_back();
        if (server_msg == "")
        {
            server_msg = "No connected servers";
        }
    }

    else if (tokens[0].compare("FETCH_MSG") == 0 && tokens.size() == 2)
    {
        std::vector<std::string> from_group = messages[tokens[1]];

        if (!from_group.empty())
        {
            server_msg = from_group.front();
            from_group.erase(from_group.begin());
            messages[tokens[1]] = from_group;
        }
        else
        {
            server_msg = "No message";
        }
    }
    else if (tokens[0].compare("SEND_MSG") == 0 && tokens.size() == 3)
    {
        outgoing[tokens[1]].push_back(tokens[2]);
        server_msg = "Message sent";
    }

    else if (tokens[0].compare("CONNECT") == 0 && tokens.size() == 3)
    {

        int connection_socket = establish_connection(tokens[2], tokens[1]);
        servers[clientSocket] = new Client(clientSocket, true);
        servers[clientSocket]->name = "P3_GROUP_6";
        servers[clientSocket]->portnr = tokens[2];
        servers[clientSocket]->ipaddr = tokens[1];
        FD_SET(clientSocket, openSockets);
        *maxfds = std::max(*maxfds, clientSocket);
        send_queryservers(clientSocket);
        server_msg = "Sucessfully sent QUERYSERVERS";
    }
    else if (tokens[0].compare("STORED") == 0 && tokens.size() == 1)

    {
        for (auto const &pair : stored_servers)
        {
            Client *client = pair.second;
            server_msg += client->name + "," + client->ipaddr + "," + client->portnr + ';';
        }

        if (server_msg == "")
        {
            server_msg = "You have no stored servers\n";
        }
    }

    else if (tokens[0].compare("CONNECTIONS") == 0 && tokens.size() == 1)
    {
        for (auto const &pair : servers)
        {
            Client *client = pair.second;
            server_msg += client->name + ",";
        }

        server_msg.pop_back();

        if (server_msg == "")
        {
            server_msg = "You have no connections\n";
        }
    }
    else
    {
        server_msg = "Unknown command: " + message + "\n" + "COMMANDS:\n" + "  - QUERYSERVERS\n" + "  - FETCH_MSG,<GROUP_ID>\n" + "  - SEND_MSG,<GROUP_ID>,<MESSAGE>";
    }

    char send_buffer[server_msg.size() + 2];
    std::string logger = "SENDING: " + server_msg + " to group: " + tokens[1];
    log_to_file(logger);
    construct_message(send_buffer, server_msg);
    send(clientSocket, send_buffer, server_msg.size() + 2, 0);
}

// Process command from client on the server
void serverCommand(int serverSocket, fd_set *openSockets, int *maxfds, char *buffer, std::string src_port)
{
    std::string server_msg;

    if (!valid_message(buffer))
    {
        server_msg = "You are nothing but a scam";
        char send_buffer[server_msg.size() + 2];
        construct_message(send_buffer, server_msg);
        send(serverSocket, send_buffer, server_msg.size() + 2, 0);
        std::cout << "NOT A VALID MESSAGE" << std::endl;
        return;
    }

    std::string message = std::string(buffer);
    message.erase(0, 1);
    message.erase(message.size() - 1);
    std::cout << "The message we got: " << message << std::endl;

    std::vector<std::string> tokens;
    std::string token;
    std::stringstream ss(message);

    while (ss.good())
    {
        getline(ss, token, ',');
        tokens.push_back(token);
    }

    if ((tokens[0].compare("QUERYSERVERS") == 0) && tokens.size() == 2)
    {
        server_msg = "SERVERS,P3_GROUP_7," + get_local_ip() + ',' + src_port + ';';
        if(!servers.empty()){
            for (auto const &pair : servers)
            {
                Client *client = pair.second;
                std::cout << client->name <<  client->ipaddr << client->portnr << std::endl;

                server_msg += client->name + ',' + client->ipaddr + ',' + client->portnr + ';';
            }
        }
    }

    else if (tokens[0].compare("SERVERS") == 0)
    {
        std::cout << "RESPONDING TO: " << message << std::endl;
        std::string log = tokens[1] + "GETS: " + message;
        log_to_file(log);

        std::vector<std::string> servers_info;

        server_vector(message, &servers_info);

        std::vector<std::string> group_IP_portnr_list;

        split_commas(&servers_info, &group_IP_portnr_list);

        for (int i = 0; i < group_IP_portnr_list.size(); i += 3)
        {

            std::string group_id = group_IP_portnr_list[i];
            std::string ip_address = group_IP_portnr_list[i + 1];
            std::string port_number = group_IP_portnr_list[i + 2];

            int sockfd = open_socket(stoi(port_number), false);
            //std::cout << "Name: " << group_id << std::endl;
            if (stoi(port_number) != -1 && group_id != "P3_GROUP_7" && (group_id.substr(0, 3) == "P3_" || group_id.substr(0, 3) == "Ins") && port_number.size() == 4)
            {
                if (i == 0)
                {
                    servers[sockfd] = new Client(sockfd, true);
                    servers[sockfd]->name = group_id;
                    servers[sockfd]->ipaddr = ip_address;
                    servers[sockfd]->portnr = port_number;
                }
                else
                {
                    if (!isStored(group_id, stored_names))
                    {
                        stored_servers[sockfd] = new Client(sockfd, true);
                        stored_servers[sockfd]->name = group_id;
                        stored_servers[sockfd]->ipaddr = ip_address;
                        stored_servers[sockfd]->portnr = port_number;

                        stored_names.push_back(group_id);
                    }
                }
            }
        }
    }

    else if (tokens[0].compare("LEAVE") == 0)
    {
        // Close the socket, and leave the socket handling
        // code to deal with tidying up clients etc. when
        // select() detects the OS has torn down the connection.
        closeClient(serverSocket, openSockets, maxfds);
    }

    else if (tokens[0].compare("KEEPALIVE") == 0)
    {
        std::cout << "I am a message from KEEPALIVE" << std::endl;

        int count = stoi(tokens[1]);
        // check if the message that we got from another server has any message for us
        if (count > 0)
        {
            // initialize variables so that we can send FETCH_MSGS to the server that sent to us a KEEPALIVE message
            std::string nameto = clients[serverSocket]->name;
            std::string ipAddrto = clients[serverSocket]->ipaddr;
            std::string portnrto = clients[serverSocket]->portnr;
            // initialize the message variable with a global variable that has the value "P3_GROUP_7"
            std::string message = std::string("FETCH_MSGS,") + std::string(GROUP);
            char send_buffer[message.size() + 2];

            construct_message(send_buffer, message);
            int connection_socket = establish_connection(portnrto, ipAddrto);

            if (send(connection_socket, send_buffer, message.size() + 2, 0) < 0)
            {
                perror("Unable to send");
            }
            else
            {
                printf("Message: %s sent successfully", message.c_str());
            }
        }
    }
    //server command
    else if (tokens[0].compare("FETCH_MSGS") == 0 && tokens.size() == 3)
    {
        // some FETCH MSGS stuff
        std::cout << "I am a message from FETCH_MSGS" << std::endl;

        // get group number
        std::string group = tokens[1];
        // find the relevant group from connections using the given group id
        std::string nameto = connections[group]->name;
        std::string ipAddrto = connections[group]->ipaddr;
        std::string portnrto = connections[group]->portnr;

        // check if messages contains this group as key
        if (messages.find(group) != messages.end())
        {
            // is so create a new vector
            //std::vector<std::string> requested_messages = messages[group];
            std::string message;
            // go through each message in messages and append it to the message string
            for (auto const &data : messages[group])
            {
                message += ',' + data;
            }
            // create a char array with the size of the message + STX and ETX
            char send_buffer[message.size() + 2];

            construct_message(send_buffer, message);
            int connection_socket = establish_connection(portnrto, ipAddrto);

            if (send(connection_socket, send_buffer, message.size() + 2, 0) < 0)
            {
                perror("Unable to send");
            }
            else
            {
                // print the message if successful in sending it to the receiver
                printf("Message: %s sent succesfully", message.c_str());
            }
            //store the message in the txt file

            // delete this group id messages as we have sent them to the one who requested them
            messages[group].clear();
            messages[group] = {};
        }
    }
    else if (tokens[0].compare("SEND_MSG") == 0 && tokens.size() == 5)
    {

        std::cout << "I am a message from SEND_MSG" << std::endl;
        // initialize variables so that we can send the message that the one server has for the other or if we don't have it stored, caching it
        std::string nameto = connections[tokens[1]]->name;
        std::string ipAddrto = connections[tokens[1]]->ipaddr;
        std::string portnrto = connections[tokens[1]]->portnr;

        // the message that we received
        std::string message = tokens[3];

        // check if we are connected or have their information stored in connection map
        if (connections.find(tokens[1]) != connections.end())
        {

            char send_buffer[message.size() + 2];

            construct_message(send_buffer, message);
            int connection_socket = establish_connection(portnrto, ipAddrto);

            if (send(connection_socket, send_buffer, message.size() + 2, 0) < 0)
            {
                perror("Unable to send");
            }
            else
            {
                printf("Message: %s sent succesfully", message.c_str());
            }
        }
        else
        { // if we have not connected to the receiving then we have to cache it and wait for when someone fetches the message
            // not found
            // cache the message and wait until someone fetches the message
            std::vector<std::string> storedMessage;
            storedMessage.push_back(message);

            messages.insert({nameto, storedMessage});
            //messages[tokens[1]] = message.push_back(tokens[4]);
        }
    }
    else if (tokens[0].compare("STATUSREQ") == 0)
    {
        // some STATUSREQ stuff
        std::cout << "I am a message from STATUSREQ" << std::endl;

        std::string response = "STATUSRESP,P3_GROUP_7," + servers[serverSocket]->name + ",";
        for (auto const &msg_pair : messages)
        {
            if (msg_pair.second.size() == 0)
            {
                continue;
            }
            response += msg_pair.first + "," + std::to_string(msg_pair.second.size()) + ",";
        }
        char send_buffer[response.size() + 2];

        if (!response.empty())
        {
            response.pop_back();
        }

        construct_message(send_buffer, response);

        if (send(serverSocket, send_buffer, response.size() + 2, 0) < 0)
        {
            perror("Sending STATUSREQ failed!");
        }
    }

    else if (tokens[0].compare("STATUSRESP") == 0)
    {
        // some STATUSRESP stuff
        std::cout << "I am a message from STATUSRESP" << std::endl;

        std::string response = "STATUSRESP,P3_GROUP_7," + servers[serverSocket]->name + ",";

        for (auto const &msg_pair : messages)
        {
            if (msg_pair.second.size() == 0)
            {
                continue;
            }
            response += msg_pair.first + "," + std::to_string(msg_pair.second.size()) + ",";
        }
        char send_buffer[response.size() + 2];

        if (!response.empty())
        {
            response.pop_back();
        }

        construct_message(send_buffer, response);

        if (send(serverSocket, send_buffer, response.size() + 2, 0) < 0)
        {
            perror("Sending STATUSRESP failed!");
        }
    }
    //client command
    else if ((tokens[0].compare("SEND_MSG") == 0) && tokens.size() == 4)
    {
    }
    //client command
    else if ((tokens[0].compare("FETCH_MSG") == 0) && tokens.size() == 4)
    {
    }

    char send_buffer[server_msg.size() + 2];
    std::string logger;

    logger = "SENDING: " + server_msg + " to group: " + tokens[1];
    log_to_file(logger);
    construct_message(send_buffer, server_msg);

    if(send(serverSocket, send_buffer, server_msg.size() + 2, 0) < 0){
        perror("Message failed to send");
    }else{

        printf("Message:%s succesfully sent \n", server_msg.c_str());
    }
}

int main(int argc, char *argv[])
{
    bool finished;
    int server_listen_sock;
    int client_listen_sock;
    int serverSock;
    int clientSock;
    int clientPort;
    int serverPort;

    fd_set openSockets;   // Current open sockets
    fd_set readSockets;   // Socket list for select()
    fd_set exceptSockets; // Exception socket list
    int maxfds;           // Passed to select() as max fd in set
    struct sockaddr_in client;
    socklen_t clientLen;
    char buffer[1025]; // buffer for reading from clients

    if (argc != 2)
    {
        printf("Usage: chat_server <ip port>\n");
        exit(0);
    }

    serverPort = atoi(argv[1]);

    // Setup socket for server to listen to
    server_listen_sock = open_socket(atoi(argv[1]), true);
    printf("Server listening on port: %d\n", serverPort);

    if (listen(server_listen_sock, BACKLOG) < 0)

    {
        printf("Listen failed on port %s\n", argv[1]);
        std::string logger = "Listen failed on port: " + std::string(argv[1]);
        log_to_file(logger);
        exit(0);
    }
    else
    {
        // Add listen socket to socket set we are monitoring
        // Enables the server_listen_sock to enter "loops" where it listens for events
        FD_SET(server_listen_sock, &openSockets);
        maxfds = server_listen_sock;
    }

    clientPort = serverPort + 1;

    client_listen_sock = open_socket(atoi(argv[1]) + 1, true);
    printf("Client listening on port: %d\n", clientPort);

    if (listen(client_listen_sock, BACKLOG) < 0)
    {
        printf("Listen failed on port %s\n", argv[1]);
        std::string logger = "Listen failed on port: " + std::string(argv[1]);
        log_to_file(logger);
        exit(0);
    }
    else
    {
        // Add listen socket to socket set we are monitoring
        // Enables the server_listen_sock to enter "loops" where it listens for events
        FD_SET(client_listen_sock, &openSockets);
        maxfds = client_listen_sock;
    }

    finished = false;

    while (!finished)
    {
        // Get modifiable copy of readSockets
        readSockets = exceptSockets = openSockets;
        memset(buffer, 0, sizeof(buffer));

        // Look at sockets and see which ones have something to be read()
        // EVENTS - n
        int n = select(maxfds + 1, &readSockets, NULL, &exceptSockets, NULL);

        if (n < 0)
        {
            perror("select failed - closing down\n");
            std::string logger = "select failed - closing down";
            log_to_file(logger);
            finished = true;
        }
        else
        {

            // First, accept  any new connections to the server on the listening socket
            if (FD_ISSET(server_listen_sock, &readSockets))
            {
                serverSock = accept(server_listen_sock, (struct sockaddr *)&client,

                                    &clientLen);
                printf("accept***\n");
                // Add new client to the list of open sockets
                FD_SET(serverSock, &openSockets);

                // And update the maximum file descriptor
                maxfds = std::max(maxfds, serverSock);

                // create a new client to store information.
                servers[serverSock] = new Client(serverSock, true);

                // Decrement the number of sockets waiting to be dealt with
                n--;

                printf("Client connected on server: %d\n", serverSock);
                std::string logger = "Client connected on server: " + std::to_string(serverSock);
                log_to_file(logger);
            }

            // First, accept  any new connections to the server on the listening socket
            if (FD_ISSET(client_listen_sock, &readSockets))
            {
                clientSock = accept(client_listen_sock, (struct sockaddr *)&client,
                                    &clientLen);
                printf("accept***\n");
                // Add new client to the list of open sockets
                FD_SET(clientSock, &openSockets);

                // And update the maximum file descriptor
                maxfds = std::max(maxfds, clientSock);

                // create a new client to store information.
                clients[clientSock] = new Client(clientSock, true);

                // Decrement the number of sockets waiting to be dealt with
                n--;

                printf("Client connected on server: %d\n", clientSock);
                std::string logger = "Client connected on server: " + std::to_string(clientSock);
                log_to_file(logger);
            }

            // Now check for commands from clients(the servers that are already connected)
            std::list<Client *> disconnectedClients;

            while (n-- > 0)
            {

                // SERVERS
                for (auto const &pair : servers)

                {
                    Client *client = pair.second;

                    if (FD_ISSET(client->sock, &readSockets))
                    {
                        // recv() == 0 means client has closed connection
                        if (recv(client->sock, buffer, sizeof(buffer), MSG_DONTWAIT) == 0)
                        {
                            disconnectedClients.push_back(client);
                            closeClient(client->sock, &openSockets, &maxfds);
                            break;
                        }
                        // We don't check for -1 (nothing received) because select()
                        // only triggers if there is something on the socket for us.
                        if (valid_message(buffer))
                        {
                            serverCommand(client->sock, &openSockets, &maxfds, buffer, std::to_string(serverPort));
                        }
                    }
                    // Remove client from the servers list
                    for (auto const &c : disconnectedClients)
                        servers.erase(c->sock);
                }
                // CLIENTS
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
                            break;
                        }
                        // We don't check for -1 (nothing received) because select()
                        // only triggers if there is something on the socket for us.
                        if (valid_message(buffer))
                        {
                            clientCommand(client->sock, &openSockets, &maxfds, buffer, std::to_string(clientPort));
                        }
                    }
                    // Remove client from the clients list
                    for (auto const &c : disconnectedClients)
                        clients.erase(c->sock);
                }
            }
        }
    }
}
