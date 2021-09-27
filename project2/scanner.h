#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <thread>
#include <vector>
#include <algorithm>

void send_to_server(int port, int udp_sock, char* send_buffer, char* receive_buffer, int buffer_length, sockaddr_in destaddr);
std::vector<int> scan_ports(int udp_sock, char* send_buffer, char* receive_buffer, int buffer_length, int from_port_nr, int destination_port_number, struct sockaddr_in destaddr);
