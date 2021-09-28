// To run the program sudo ./puzzlesolver 130.208.242.120 10.3.15.154

#include <iostream>
#include "scanner.h"
#include "utils.h"

int send_response_to_server(char* response, int response_size, char* message, int socket, sockaddr_in destaddr, int port_number) {
    destaddr.sin_port = htons(port_number);

    if (sendto(socket, message, strlen(message) + 1, 0, (const struct sockaddr *) &destaddr, sizeof(destaddr)) < 0) {
        return -1;
    }

    bzero(response, sizeof(response));

    if (recvfrom(socket, response, response_size, 0, NULL, NULL) > 0) {
        return 1;
    } else {
        return 0;
    }
}

void part3(int udp_sock,sockaddr_in destaddr)
{
    char message[1024];
    std::string text = "4033,4014";
    bzero(message, sizeof(message));
    // TODO: not hardcode the portnumber 4042
    if (send_response_to_server(message, sizeof(message), (char *) text.c_str(),udp_sock,destaddr, 4042) < 0){
        perror("ERROR in send_response_to_server");
        exit(0);
    }

    char temp_port[5];
    int formatted_port[128];
    // format the port numbers into an easier format
    int j = 0;
    int port_counter = 0;
    for (int i = 0; i < (int) sizeof(message); i++) {
            if (message[i] == '\0' || message[i] == ',') {
                temp_port[j] = *"\0";// new line
                formatted_port[port_counter] = atoi(temp_port);
                port_counter++;

                if (message[i] == '\0'){
                    break;
                }
                j = 0;
            }

             else {
                temp_port[j] = message[i];
                j++;
            }
        }


    // TODO: take this secret phrase when we recieve it, but not hard code it
    std::string secret_text = "Hey you, you’re finally awake. You were trying to cross the border right? Walked right into that Imperial ambush same as us and that thief over there.";

    int port_number;
    for (int i = 0; i < port_counter; i++) {
        port_number = formatted_port[i];
        bzero(message, sizeof(message));
        if (send_response_to_server(message, sizeof(message), (char *) secret_text.c_str(), udp_sock, destaddr, port_number) < 0){
            perror("ERROR in send_response_to_server");
            exit(0);
        }
        
        std::cout <<  message << "\n" << std::endl;
    }





}


int main(int argc, char* argv[]) {

    // Checking if correct number of arguments are given
    if(argc != 3) {
	    printf("Incorrect number of arguments!\n");
        printf("Usage: ./puzzlesolver <ip address> <your_local_ip_address>\n");       
        exit(0);
    }

    // Create an udp socket which is connectionless (no three way handshake like TCP)
    int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
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

    if (udp_sock < 0) {
        perror("Unable to open socket!");
        exit(0);
    }

    // //Part 1
    //std::vector<int> ports = scan_ports(udp_sock, send_buffer, receive_buffer, buffer_length, 4000, 4100, destaddr);
    //print_list(ports);

    // Part 2
    // evil bit
    //evil_bit_part(4099, argv[1], argv[2]); 

    // Checksum
    memset(send_buffer, 0, sizeof(send_buffer));
    memset(receive_buffer, 0, sizeof(receive_buffer));
    strcpy(send_buffer, "$group_83$");


    struct sockaddr_in recvaddr;
    unsigned int recv_sock_len;
    sendto(udp_sock, send_buffer, strlen(send_buffer), 0, (const struct sockaddr*) &destaddr, sizeof(destaddr));
    if (recvfrom(udp_sock, receive_buffer, buffer_length, 0, (sockaddr*) &recvaddr, &recv_sock_len) > 0) {
        std::cout << "Message: " << receive_buffer << "\n" << std::endl;
    }

    //Parse the message to extract the checksum hex and the source ip address
    std::pair<unsigned int, std::string> checksum_srcip = parse_message_get_checksum_srcip(receive_buffer);
    unsigned int checksum = checksum_srcip.first;
    char* source_ip = (char*) checksum_srcip.second.c_str();
    checksum_part(6667, 4097, argv[1], source_ip, argv[2], checksum);


    // // part 3
    // part3(udp_sock, destaddr);



}
