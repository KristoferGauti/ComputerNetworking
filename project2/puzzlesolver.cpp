// To run the program ./puzzlesolver 130.208.242.120

#include <iostream>
#include "scanner.h"
#include "utils.h"

int main(int argc, char* argv[]) {

    // Checking if correct number of arguments are given
    if(argc != 2) {
	    printf("Incorrect number of arguments!\n");
        printf("Usage: ./puzzlesolver <ip address> OR\n");       
        exit(0);
    }

    int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    //struct sockaddr_in recvaddr;
    //unsigned int recv_sock_length;
    int buffer_length = 64;
    char send_buffer[buffer_length];
    char receive_buffer[buffer_length];
    strcpy(send_buffer, "Hi port!");


    // Setup the address
    struct sockaddr_in destaddr;
    destaddr.sin_family = AF_INET;
    inet_aton(argv[1], &destaddr.sin_addr);


    vector<int> ports = scan_ports(udp_sock, send_buffer, receive_buffer, buffer_length, 4000, 4100, destaddr);

    print_list(ports);
}
