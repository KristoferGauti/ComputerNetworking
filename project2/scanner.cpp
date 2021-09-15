//To run the scanner: ./scanner 130.208.242.12 4000 4100

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>


using namespace std;

int main(int argc, char* argv[]) {
    if(argc != 4) {
	    printf("Not enough arguments were given!\n");
        printf("Usage: ./scanner <ip address> <low port> <high port>\n");
        exit(0);
    }

    int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    char* address = argv[1];
    int from_port_nr = atoi(argv[2]);
    int destination_port_number = atoi(argv[3]);
    struct sockaddr_in destaddr;
    char buffer[1400];
    strcpy(buffer, "Hi port!");
    int buffer_length = strlen(buffer) + 1;


    //Create a socket
    if (udp_sock < 0) {
        perror("Unable to open socket!");
        exit(0);
    }

    //Setup the address
    destaddr.sin_family = AF_INET;
    inet_aton(address, &destaddr.sin_addr);

    destaddr.sin_port = htons(from_port_nr);
    if (sendto(udp_sock, buffer, buffer_length, 0, (const struct sockaddr*)&destaddr, sizeof(destaddr)) < 0) {
        perror("Failed");
    }
    
    


    for (int port = from_port_nr; port <= destination_port_number; port++) {    
        if (sendto(udp_sock, buffer, buffer_length, 0, (const struct sockaddr*) &destaddr, sizeof(destaddr)) < 0) {
            perror("Failed to send!");
        }
        else {
            cout << "sent to a socket!" << endl;
        }
        //Receive the udp message from recvfrom then the assignment is done
        
        // int length = recvfrom(udp_sock, buffer, buffer_length, 0, NULL, NULL);
        // cout << length << endl;
    }

    return 0;
}
