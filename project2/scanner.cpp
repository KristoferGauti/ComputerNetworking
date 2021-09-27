#include "scanner.h"

std::vector<int> sorted_port_list(4);
void send_to_server(int port, int udp_sock, char* send_buffer, char* receive_buffer, int buffer_length, sockaddr_in destaddr) {
    for (int i = 0; i <= 5; i++) {
        destaddr.sin_port = htons(port);
        struct sockaddr_in recvaddr;
        unsigned int recv_sock_len;
        std::cout << "hello" << std::endl;
        sendto(udp_sock, send_buffer, buffer_length, 0, (const struct sockaddr*) &destaddr, sizeof(destaddr));
        
        //cout << "send message: " << send_buffer << std::endl;

        // recvfrom is a blocking function. 
        if (recvfrom(udp_sock, receive_buffer, buffer_length, 0, (sockaddr*) &recvaddr, &recv_sock_len) > 0) {
            std::cout << "Port number: " << htons(recvaddr.sin_port) << std::endl;
            std::cout << "Message: " << receive_buffer << "\n" << std::endl;
            if (receive_buffer[0] == 'S') {
                sorted_port_list[0] = htons(recvaddr.sin_port);
            }
            else if (receive_buffer[0] == 'I') {
                sorted_port_list[1] = htons(recvaddr.sin_port);
            }
            else if (receive_buffer[0] == 'M') {
                sorted_port_list[2] = htons(recvaddr.sin_port);
            }
            else if (receive_buffer[0] == 'T') {
                sorted_port_list[3] = htons(recvaddr.sin_port);
            }
            break;
        }
    }
}

std::vector<int> scan_ports(int udp_sock, char* send_buffer, char* receive_buffer, int buffer_length, int from_port_nr, int destination_port_number, struct sockaddr_in destaddr) {
    

    // timeout socket check
    // code from https://newbedev.com/linux-is-there-a-read-or-recv-from-socket-with-timeout
    struct timeval tv;
    tv.tv_sec = 0; //seconds 
    tv.tv_usec = 70000; //microseconds
    setsockopt(udp_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*) &tv, sizeof tv);

    /*
        Some packets might be dropped, thus we need to send 10 times in a row to check on that
        The inner for loop does that. The more you send with UDP, the more reliable UDP becomes
    */
    for (int port = from_port_nr; port <= destination_port_number; port++) {  
        send_to_server(port, udp_sock, send_buffer, receive_buffer, buffer_length, destaddr);
    }

    return sorted_port_list;
}

