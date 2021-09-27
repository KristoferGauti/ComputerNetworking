// To run the program sudo ./puzzlesolver 130.208.242.120 10.3.15.154

#include <iostream>
#include "scanner.h"
#include "utils.h"

int main(int argc, char* argv[]) {

    // Checking if correct number of arguments are given
    if(argc != 3) {
	    printf("Incorrect number of arguments!\n");
        printf("Usage: ./puzzlesolver <ip address> <your_local_ip_address>\n");       
        exit(0);
    }

    // Create an udp socket which is connectionless (no three way handshake like TCP)
    int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);

    // Setup the address
    struct sockaddr_in destaddr;
    destaddr.sin_family = AF_INET;
    destaddr.sin_port        = htons(6665);
    destaddr.sin_addr.s_addr = inet_addr(argv[2]); // Get my ip
    inet_aton(argv[1], &destaddr.sin_addr);

    if (udp_sock < 0) {
        perror("Unable to open socket!");
        exit(0);
    }

    int buffer_length = 1024;
    char send_buffer[buffer_length];
    char receive_buffer[buffer_length];
    strcpy(send_buffer, "Hi");


    // //Part 1
    // std::vector<int> ports = scan_ports(udp_sock, send_buffer, receive_buffer, buffer_length, 4000, 4100, destaddr);
    // print_list(ports);

    //Part 2
    evil_bit_part(4099, argv[1], argv[2]); //evil bit
    //receivefrom_raw_socket(4099, argv[1], argv[2]); //Evil bit

    send_to_server(4097, udp_sock, (char *) "$group_83$", receive_buffer, buffer_length, destaddr); //Checksum
    
    //Parse the message to extract the checksum hex and the source ip address
    std::pair<unsigned int, std::string> checksum_srcip = parse_message_get_checksum_srcip(receive_buffer);
    unsigned int checksum = checksum_srcip.first;
    char* source_ip = (char*) checksum_srcip.second.c_str();
    checksum_part(6667, 4097, argv[1], source_ip, checksum);

    

}
