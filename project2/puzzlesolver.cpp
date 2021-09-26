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
    inet_aton(argv[1], &destaddr.sin_addr);

    //bind socket
    if (::bind(udp_sock, (const sockaddr*) &destaddr, sizeof(destaddr)) < 0){
        perror("Binding the udp socket failed!");
    }

    if (udp_sock < 0) {
        perror("Unable to open socket!");
        exit(0);
    }

    int buffer_length = 1024;
    char send_buffer[buffer_length];
    char receive_buffer[buffer_length];
    strcpy(send_buffer, "Hi");


    // //Part 1
    // vector<int> ports = scan_ports(udp_sock, send_buffer, receive_buffer, buffer_length, 4000, 4100, destaddr);
    // print_list(ports);

    //Part 2
    receivefrom_raw_socket(4099, argv[1], argv[2]); //Evil bit

    send_to_server(4097, udp_sock, (char *) "$group_83$", receive_buffer, buffer_length, destaddr); //Checksum
    
    //Parse the message to extract the checksum hex and the source ip address
    pair<unsigned int, string> checksum_srcip = parse_message_get_checksum_srcip(receive_buffer);
    unsigned int checksum = checksum_srcip.first;
    char* source_ip = (char*) checksum_srcip.second.c_str();
    cout << "Source address: " << source_ip << endl;
    cout << "Checksum: " << checksum << endl;
    send_raw_socket(6667, 4097, argv[1], source_ip, 0, checksum);

    

}
