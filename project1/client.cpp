#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>


using namespace std;

char buffer[4096]; 
int sockfd;

int establish_a_connection(char port_number[], char ip_addr[]) {
	cout << ip_addr << endl;
	//Establish a connection
	struct sockaddr_in server_addr; 					//Declare Server Address
	server_addr.sin_family = AF_INET; 					//IPv4 address family
	server_addr.sin_addr.s_addr = INADDR_ANY;			//Bind Socket to all available interfaces					
	server_addr.sin_port = htons(atoi(port_number));	//Convert the ASCII port number to integer port number

	//Check for errors for set socket address
	if(inet_pton(AF_INET, ip_addr, &server_addr.sin_addr)<=0) 
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


	//Create a socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 	//Create a IPv4 TCP socket
	if (sockfd < 0) 							//Check if the socket creation failed
        printf("Creating a socket failed");

	//Initiates a connection on the socket
	establish_a_connection(argv[1], argv[2]); 

	//Get user input
	char u_input[64];
	while (*u_input != 'q') {
		cin.getline(u_input, sizeof(u_input));

		//Sends the user input to the socket
		send(sockfd, u_input, sizeof(u_input), 0);
		//receives a message from the socket and puts it in the buffer
		recv(sockfd, buffer, sizeof(buffer), 0);
		cout << buffer << endl;
		//Clears the buffer and user input and sets all the items in the char array to 0
		memset(buffer, 0, sizeof(buffer));
		memset(u_input, 0, sizeof(u_input));
	}
}
