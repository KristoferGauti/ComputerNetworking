#include "scanner.h"

using namespace std;

vector<int> ports;
int send_to_server(int port, int udp_sock, char* send_buffer, char* receive_buffer, int buffer_length, sockaddr_in destaddr) {
    destaddr.sin_port = htons(port);
    struct sockaddr_in recvaddr;
    unsigned int recv_sock_len;
   
    if (sendto(udp_sock, send_buffer, buffer_length, 0, (const struct sockaddr*) &destaddr, sizeof(destaddr)) < 0) {
        return -1;
    }

    // recvfrom is a blocking function. 
    if (recvfrom(udp_sock, receive_buffer, buffer_length, 0, (sockaddr*) &recvaddr, &recv_sock_len) > 0) {
        cout << "\n" << "Message: " << receive_buffer << endl;
        cout << "port " << htons(recvaddr.sin_port) << " is open!" << endl;
        if (find(ports.begin(), ports.end(), htons(recvaddr.sin_port)) == ports.end()) {
            ports.push_back(htons(recvaddr.sin_port));
        }
        return 1;
    }
    else {
        return -1;
    }
}

vector<int> scan_ports(int udp_sock, char* send_buffer, char* receive_buffer, int buffer_length, int from_port_nr, int destination_port_number, struct sockaddr_in destaddr) {
    

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
        for (int i = 0; i <= 5; i++) {
            send_to_server(port, udp_sock, send_buffer, receive_buffer, buffer_length, destaddr);
        }
    }
    return ports;
}

