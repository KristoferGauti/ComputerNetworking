#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>



using namespace std;

char buffer[4096]; 
int sockfd;

int establish_a_connection(int port_number) {
	//Establish a connection
	struct sockaddr_in server_addr; 		//Declare Server Address
	server_addr.sin_family = AF_INET; 		//IPv4 address family
	server_addr.sin_addr.s_addr = INADDR_ANY;	//Bind Socket to all available interfaces					
	server_addr.sin_port = htons(port_number);		//Port number

	//Check for errors for set socket address
	if(inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr)<=0) 
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

	//Check if the connection was successful
	int connection_successful = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (connection_successful < 0)
    {
        printf("\nConnection failed \n");
        return -1;
    }

	printf("Connection successful!\n");
	return 0;
}

int main(int argc, char* argv[]) {
	if(argc != 3) {
		printf("Not enough arguments were given!\n");
        printf("Usage: client <ip address> <ip port>\n");
        exit(0);
    }

	int port_number = atoi(argv[2]);

	//Create a socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 	//Create a IPv4 TCP socket
	if (sockfd < 0) //Check if the socket creation failed
        printf("Creating a socket failed");

	establish_a_connection(port_number); 

	//Get user input
	char u_input[64];
	while (*u_input != 'q') {
		cin.getline(u_input, sizeof(u_input));

		send(sockfd, u_input, sizeof(u_input), 0);
		recv(sockfd, buffer, sizeof(buffer), 0);
		cout << buffer << endl;
		memset(buffer, 0, sizeof(buffer));
		memset(u_input, 0, sizeof(u_input));
	}
}
