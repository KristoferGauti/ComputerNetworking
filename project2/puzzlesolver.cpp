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
    if (udp_sock < 0) {
        perror("Unable to open socket!");
        exit(0);
    }

    int buffer_length = 1024;
    char send_buffer[buffer_length];
    char receive_buffer[buffer_length];
    strcpy(send_buffer, "Hi port!");

    // Setup the address
    struct sockaddr_in destaddr;
    destaddr.sin_family = AF_INET;
    destaddr.sin_port        = htons(4097);
    destaddr.sin_addr.s_addr = inet_addr(argv[2]); // Get my ip
    inet_aton(argv[1], &destaddr.sin_addr);


    //Part 1
    std::cout << "\nPart 1 - Port Scanner" << std::endl;
    std::vector<int> ports = scan_ports(udp_sock, send_buffer, receive_buffer, buffer_length, 4000, 4100, destaddr);
    print_list(ports);

    // // Part 2
    // // evil bit
    // std::cout << "\nPart 2 - Evil bit" << std::endl;
    // std::string evil_bit_secret_port = evil_bit_part(ports[3], argv[1], argv[2]); 

    // Checksum
    std::cout << "\nPart 3 - Checksum" << std::endl;
    memset(send_buffer, 0, sizeof(send_buffer));
    memset(receive_buffer, 0, sizeof(receive_buffer));
    strcpy(send_buffer, "$group_83$");

    struct sockaddr_in recvaddr;
    unsigned int recv_sock_len;

    memset(receive_buffer, 0, sizeof(receive_buffer));
    while(strlen(receive_buffer) == 0) {
        if (sendto(udp_sock, send_buffer, strlen(send_buffer), 0, (const struct sockaddr*) &destaddr, sizeof(destaddr)) < 0){
            perror("Failed to send!");
        }
        if (recvfrom(udp_sock, receive_buffer, buffer_length, 0, (sockaddr*) &recvaddr, &recv_sock_len) > 0) {
            std::cout << "Message: " << receive_buffer << "\n" << std::endl;
        }
    }


    //Parse the message to extract the checksum hex and the source ip address
    std::pair<unsigned int, std::string> checksum_srcip = parse_message_get_checksum_srcip(receive_buffer);
    unsigned int checksum = checksum_srcip.first;
    char* source_ip = (char*) checksum_srcip.second.c_str();
    std::string secret_phrase = checksum_part(6667, ports[0], argv[1], source_ip, argv[2], checksum);
    

    //Get the hidden port from My boss told me...
    std::string hidden_port = std::to_string(ports[2]);

    // // part 3
    // std::cout << "\nPart 3 - Oracle port knocking" << std::endl;
    // std::cout << secret_phrase << std::endl;


    //Set the Oracle port address
    struct sockaddr_in oracleaddr;
    oracleaddr.sin_family = AF_INET;
    //oracleaddr.sin_port        = htons(ports[1]);
    oracleaddr.sin_port        = htons(4042);
    oracleaddr.sin_addr.s_addr = inet_addr(argv[2]); // Get my ip
    inet_aton(argv[1], &oracleaddr.sin_addr);
    std::cout << "INDEX"<< ports[1] << std::endl;
    part3(udp_sock, oracleaddr, "4014", secret_phrase, hidden_port, ports[1]);
    //part3(udp_sock, oracleaddr, "4014", "Hey you, you’re finally awake. You were trying to cross the border right? Walked right into that Imperial ambush same as us and that thief over there.", "4033", 4042);
}
