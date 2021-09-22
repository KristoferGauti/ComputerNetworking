#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <thread>
#include <list>

using namespace std;

list<int> scan_ports(int from_port_nr, int destination_port_number, struct sockaddr_in destaddr);
