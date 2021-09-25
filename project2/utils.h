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

void print_list(std::vector<int> vec);
void create_packet(int port, char* address);




