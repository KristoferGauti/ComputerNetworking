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
#include <queue>

//Global variables
#define CHUNK_SIZE 512
#define GROUP "P3_GROUP_7"

// fix SOCK_NONBLOCK for OSX
#ifndef SOCK_NONBLOCK
#include <fcntl.h>
#define SOCK_NONBLOCK O_NONBLOCK
#endif

#define BACKLOG 5 // Allowed length of queue of waiting connections

#define MAX_CONNECTIONS 16

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
    bool sent;
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

std::queue<int> server_queue;

std::ofstream server_log;

std::string logger;

/*
 * Checks if the messsage received is valid
 */
bool valid_message(char *buffer)
{
    return buffer[0] == 0x02 && buffer[strlen(buffer) - 1] == 0x03;
}
/*
 * Take the message that was received and takes away 0x02 at the front and 0x03 at the back
 * Returns a filled out char array with the relevant information
 */
void parse_message(char *buffer, char *newBuffer)
{

    for (std::size_t i = 1; i < strlen(buffer); i++)
    {
        newBuffer[i - 1] = buffer[i];
    }
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
            if ((std::string)ifa->ifa_name == "en0" || (std::string)ifa->ifa_name == "wlp1s0" || (std::string)ifa->ifa_name == "ens192")            {
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

    struct sockaddr_in server_addr;                      //Declare Server Address
    server_addr.sin_family = AF_INET;                    //IPv4 address family
    server_addr.sin_addr.s_addr = INADDR_ANY;            //Bind Socket to all available interfaces
    server_addr.sin_port = htons(atoi(port_nr.c_str())); //Convert the ASCII port number to integer port number

    //Check for errors for set socket address
    if (inet_pton(AF_INET, ip_addr.c_str(), &server_addr.sin_addr) <= 0)
    {
        printf("\nInvalid address/ Address not supported \n");
        exit(0);
    }

    //Create a tcp socket
    int connection_socket = open_socket(stoi(port_nr), false);

    if (connect(connection_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {

        perror("\nConnection failed");
        exit(0);
    }

    std::cout << "CONNECTED TO SERVER ON IP: " + ip_addr + ", PORT: " + port_nr << std::endl;

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
    for (std::size_t i = 1; i <= message.size(); i++)
    {
        send_buffer[i] = temp_buffer[index];
        index++;
    }
    send_buffer[strlen(temp_buffer) + 1] = 0x03;
}

void send_queryservers(int connection_socket, std::string src_port)
{
    std::string message = "QUERYSERVERS,P3_GROUP_7,"; //+ get_local_ip() + "," + src_port;
    char sendBuffer[message.size() + 2];
    construct_message(sendBuffer, message);
    if (send(connection_socket, sendBuffer, message.length() + 2, 0) < 0)
    {
        perror("Sending message failed sendqueryservers");
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
    for (std::size_t i = 0; i < stored_names.size(); i++)
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
	
	// Initialize the string to the command that was received
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
        if(!servers.empty()){
            for (auto const &pair : servers)

            {
                Client *client = pair.second;
                server_msg += client->name + ",";
            }
            server_msg.pop_back();
        }

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

            char send_buffer[server_msg.size() + 2];

            construct_message(send_buffer, server_msg);
            if(send(clientSocket, send_buffer, server_msg.size()+2, 0) < 0){
                perror("Unable to send");
            }
            else{
                printf("Message: %s sent succesfully", server_msg.c_str());
            }
            //from_group.erase(from_group.begin());
            //messages[tokens[1]] = from_group;
        }
        else
        {
            server_msg = "No message";
        }
    }
        // Sends the message that the client sent to the group
    else if (tokens[0].compare("SEND_MSG") == 0 && tokens.size() == 3)
    {
        // some SEND MSG stuff
        std::cout << "I am a message from SEND_MSG" << std::endl;
        if ( connections.find(tokens[1]) != connections.end() ) {
            // found
            // send the msg content tokens[3]
            //
            std::string namefrom = connections[tokens[1]]->name;
            std::string ipAddrfrom = connections[tokens[1]]->ipaddr;
            std::string portnrfrom = connections[tokens[1]]->portnr;
            //int sockfrom = connections[tokens[1]]->sock;


            server_msg = "SEND_MSG," + tokens[1] + ',' + tokens[2];

            char send_buffer[message.size() + 2];

            construct_message(send_buffer, message);
            int connection_socket = establish_connection(portnrfrom, ipAddrfrom);

            // Store the connection into the connected_servers
            servers[connection_socket] = new Client(connection_socket, true);
            servers[connection_socket]->name = namefrom;
            servers[connection_socket]->ipaddr = ipAddrfrom;
            servers[connection_socket]->portnr = portnrfrom;

            std::string logger = "SENDING: " + server_msg + " to group: " + tokens[1];
            log_to_file(logger);

            if(send(connection_socket, send_buffer, message.size()+2, 0) < 0){
                perror("Unable to send");
            }
            else{
                printf("Message: %s sent succesfully", message.c_str());
            }

        } else {
            // not found
            // cache the message and wait until someone fetches the message
            std::vector<std::string> message;
            message.push_back(tokens[2]);
            messages.insert({tokens[1], message});
            //messages[tokens[1]] = message.push_back(tokens[4]);
        }
    }

    else if (tokens[0].compare("CONNECT") == 0 && tokens.size() == 4)
    {

        int connection_socket = establish_connection(tokens[2], tokens[1]);
        servers[connection_socket] = new Client(connection_socket, true);
        FD_SET(connection_socket, openSockets);
        *maxfds = std::max(*maxfds, connection_socket);
        send_queryservers(connection_socket, std::to_string(5000));
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
    else
    {
        server_msg = "Unknown command: " + message + "\n" + "COMMANDS:\n" + "  - QUERYSERVERS\n" + "  - FETCH_MSG,<GROUP_ID>\n" + "  - SEND_MSG,<GROUP_ID>,<MESSAGE>";
    }

    char send_buffer[server_msg.size() + 2];
    construct_message(send_buffer, server_msg);
	std::string logger = "SENDING: " + server_msg + " to group: " + tokens[1];
    log_to_file(logger);
    send(clientSocket, send_buffer, server_msg.size() + 2, 0);
}

// Process command from client on the server
void serverCommand(int serverSocket, fd_set *openSockets, int *maxfds, char *buffer, std::string src_port)
{
    std::string server_msg = "";

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
    //std::cout << "The message we got: " << message << std::endl;
    logger = "The message we got back: " + message;
    log_to_file(logger);

    if (!message.empty())
    {
        std::cout << "The message we got: " << message << std::endl;
    }

    std::vector<std::string> tokens;
    std::string token;
    std::stringstream ss(message);

    while (ss.good())
    {
        getline(ss, token, ',');
        tokens.push_back(token);
    }

    if ((tokens[0].compare("QUERYSERVERS") == 0) && tokens.size() == 2 || tokens.size() == 4)
    {
        server_msg = "SERVERS,P3_GROUP_7," + get_local_ip() + "," + src_port + ";";

        for (auto const &pair : servers)
        {
            Client *client = pair.second;
            if (!client->name.empty() && !client->ipaddr.empty() && !client->portnr.empty())
            {
                server_msg += client->name + "," + client->ipaddr + "," + client->portnr + ';';
            }
        }
    }

    else if (tokens[0].compare("SERVERS") == 0)
    {
        std::cout << "RESPONDING TO: " << message << std::endl;
        logger = "RESPONDING TO: " + message;
        log_to_file(logger);

        std::vector<std::string> servers_info;

        server_vector(message, &servers_info);

        std::vector<std::string> group_IP_portnr_list;

        split_commas(&servers_info, &group_IP_portnr_list);

        for (std::size_t i = 0; i < group_IP_portnr_list.size(); i += 3)
        {

            std::string group_id = group_IP_portnr_list[i];
            std::string ip_address = group_IP_portnr_list[i + 1];
            std::string port_number = group_IP_portnr_list[i + 2];

            int sockfd = open_socket(stoi(port_number), false);

            if (stoi(port_number) != -1 && group_id != "P3_GROUP_7" && !isStored(group_id, stored_names))
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
                    stored_servers[sockfd] = new Client(sockfd, true);
                    stored_servers[sockfd]->name = group_id;
                    stored_servers[sockfd]->ipaddr = ip_address;
                    stored_servers[sockfd]->portnr = port_number;

                    stored_names.push_back(group_id);
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
        // some KEEPALIVE stuff
        //std::cout << "I am a message from KEEPALIVE" << std::endl;
        // next three comments are for the periodically KEEPALIVE that our server sends
        // use threads to wait a minute
        // check if we have some message stored for a server
        // send to the server KEEPALIVE,how many messages
        int count = stoi(tokens[1]);
        // check if the message that we got from another server has any message for us
        if (count > 0)
        {
            // initialize variables so that we can send FETCH_MSGS to the server that sent to us a KEEPALIVE message
            std::string nameto = clients[serverSocket]->name;
            std::string ipAddrto = clients[serverSocket]->ipaddr;
            std::string portnrto = clients[serverSocket]->portnr;
            // initialize the message variable with a global variable that has the value "P3_GROUP_7"
            std::string message = "FETCH_MSGS,P3_GROUP_7";
            char send_buffer[message.size() + 2];
            construct_message(send_buffer, message);

            if (send(serverSocket, send_buffer, message.size() + 2, 0) < 0)
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
                message += data + ',';
            }

            message.pop_back();
            // create a char array with the size of the message + STX and ETX
            char send_buffer[message.size() + 2];

            construct_message(send_buffer, message);

            if (send(serverSocket, send_buffer, message.size() + 2, 0) < 0)
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
    else if (tokens[0].compare("SEND_MSG") == 0 && tokens.size() == 4)
    {

        std::cout << "I am a message from SEND_MSG" << std::endl;
        // initialize variables so that we can send the message that the one server has for the other or if we don't have it stored, caching it

        // the message that we received
        std::string message = tokens[3];

        // check if we are connected or have their information stored in connection map
        if (connections.find(tokens[1]) != connections.end())
        {
            if(!connections[tokens[1]]->sent){
                std::string nameto = connections[tokens[1]]->name;
                std::string ipAddrto = connections[tokens[1]]->ipaddr;
                std::string portnrto = connections[tokens[1]]->portnr;
                int socket = connections[tokens[1]]->sock;

                // the message that we received
                std::string message = tokens[3];

                char send_buffer[message.size() + 2];
                construct_message(send_buffer, message);
                int connection_socket = establish_connection(portnrto, ipAddrto);

                if (send(connection_socket, send_buffer, message.size() + 2, 0) < 0)
                {
                    connections[tokens[1]]->sent = true;
                    perror("Unable to send");
                }
                else
                {
                    printf("Message: %s sent succesfully", message.c_str());
                }
            }
        }

        else
        { // if we have not connected to the receiving then we have to cache it and wait for when someone fetches the message
            // not found
            // cache the message and wait until someone fetches the message
            std::vector<std::string> storedMessage;
            storedMessage.push_back(message);

            messages.insert({tokens[1], storedMessage});
            //messages[tokens[1]] = message.push_back(tokens[4]);
        }
    }
    // Responds with the amount of messages we have for each group that we have reached a message for
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
    // Responds with the amount of messages we have for each group that we have reached a message for
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
		logger = "SENDING: " + server_msg + " to group: " + tokens[1];
    	log_to_file(logger);

        if (send(serverSocket, send_buffer, response.size() + 2, 0) < 0)
        {
            perror("Sending STATUSRESP failed!");
        }
    }

    char send_buffer[server_msg.size() + 2];
    construct_message(send_buffer, server_msg);
    send(serverSocket, send_buffer, server_msg.size() + 2, 0);
}

// Sends on KEEPALIVE on 2:30 minutes interval
void sendKeepAlive(){
    // next three comments are for the periodically KEEPALIVE that our server sends
    // use threads to wait a minute
    // check if we have some message stored for a server
    // send to the server KEEPALIVE,how many messages

    //std::this_thread::sleep_for(std::chrono::minutes(2));
    //std::this_thread::sleep_for(std::chrono::seconds (30));
    bool FINISHED = false;
    while (!FINISHED)
    {
        if(!servers.empty()) {
            sleep(150);
            for (auto const &pair: servers) {
                if (messages.count(pair.second->name) >= 0) {
                    std::vector <std::string> storedmessages = messages[pair.second->name];
                    std::string keepalive = "KEEPALIVE," + std::to_string(storedmessages.size());

                    char send_buffer[keepalive.size() + 2];
                    construct_message(send_buffer, keepalive);
                    if (send(pair.second->sock, send_buffer, keepalive.length() + 2, 0) < 0) {
                        perror("Sending message failed sendKeepAlive");
                        break;
                    }
                    std::cout << "KEEPALIVE," + std::to_string(storedmessages.size()) << std::endl;
                    logger = "KEEPALIVE," + std::to_string(storedmessages.size()) + " to group" +
                             servers[pair.first]->name;
                    log_to_file(logger);
                    std::cout << 1 << std::endl;
                }
                //std::cout << "KEEPALIVE," + std::to_string(storedmessages.size()) << std::endl;
            }
        }
    }
}



// Sends on QUERYSERVERS on 7:30 minutes interval
void sendUpdates(std::string port){
    //std::this_thread::sleep_for(std::chrono::minutes(7));
    //std::this_thread::sleep_for(std::chrono::seconds (30));
    //sleep(10);


    bool FINISHED = false;
    while (!FINISHED) {
        if(!servers.empty()) {
            sleep(450);
            for (auto const &pair: servers) {
                std::string query = "QUERYSERVERS," + std::string(GROUP) + ',' + get_local_ip() + ',' + port;

                char send_buffer[query.size() + 2];

                construct_message(send_buffer, query);

                if (send(pair.second->sock, send_buffer, query.length() + 2, 0) < 0) {
                    perror("Sending message failed sendUpdates");
                    break;
                } else {
                    printf("Message succesful: %s", query.c_str());
                }
                std::cout << "QUERYSERVERS," + std::string(GROUP) + ',' + get_local_ip() + ',' + port << std::endl;
            }
        }
    }
}

int main(int argc, char *argv[])
{

    bool FINISHED;
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
        exit(0);
    }
    else
    {
        // Add listen socket to socket set we are monitoring
        // Enables the server_listen_sock to enter "loops" where it listens for events
        FD_SET(client_listen_sock, &openSockets);
        maxfds = client_listen_sock;
    }

    std::thread keepAlive(sendKeepAlive);
    std::string port = std::string(argv[1]);
    std::thread Updates(sendUpdates, port);

    while (!FINISHED)
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
            FINISHED = true;
        }
        else
        {

            // First, accept  any new connections to the server on the listening socket
            if (FD_ISSET(server_listen_sock, &readSockets))
            {
                if(server_queue.size() < MAX_CONNECTIONS){
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

                    server_queue.push(serverSock);

                    printf("Client connected on server: %d\n", serverSock);
                }
                else{
                    int remSocket = server_queue.front();
                    server_queue.pop();
                    closeClient(remSocket, &openSockets, &maxfds);
                }
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
    keepAlive.join();
    Updates.join();
}
