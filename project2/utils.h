#include <iostream>
#include<stdio.h>	//for printf
#include<string.h> //memset
#include<sys/socket.h>	//for socket ofcourse
#include<stdlib.h> //for exit(0);
#include<errno.h> //For errno - the error number
#include<netinet/udp.h>	//Provides declarations for udp header
#include<netinet/ip.h>	//Provides declarations for ip header
#include <arpa/inet.h> //inet_addr
#include <ostream>

#include <vector>

using namespace std;

pair<string, string> parse_message_get_checksum_srcip(char* receive_buffer);
void print_list(std::vector<int> vec);
int create_raw_socket_headerincluded();
void send_raw_socket(int source_port, int dest_port, char* address, char* source_ip_addr);
void receivefrom_raw_socket(int port, char* address, char* local_ip_address);




