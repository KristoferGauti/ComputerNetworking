#include <iostream>
#include "utils.h"
#include "scanner.h"

using namespace std;

struct pseudo_header
{
	u_int32_t source_address;
	u_int32_t dest_address;
	u_int8_t placeholder;
	u_int8_t protocol;
	u_int16_t udp_length;
};


pair<string, string> parse_message_get_checksum_srcip(char* receive_buffer) {
	string message = string(receive_buffer);
    string checksum_hex = message.substr(143,7);
    int ip_start_idx = (int)message.find("being ")+6;
    string source_addr = message.substr(ip_start_idx, 14); //find
    source_addr.erase(remove(source_addr.begin(), source_addr.end(), '!'), source_addr.end());
	return make_pair(checksum_hex, source_addr);
}

void print_list(vector<int> vec) {
    cout << "The ports that are in the list are as follows: \n";
    for (int i = 0; i <= vec.size()-1; i++) {
        cout << vec[i] << endl;
    }
}

int create_raw_socket_headerincluded() {
	//Create a raw socket of type IPPROTO
	int raw_sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);

	if (raw_sock < 0) {
        perror("Failed to create raw socket");
		exit(0);
    }

	// Set the header include option
	int optval = 1;
	if(setsockopt(raw_sock, IPPROTO_IP, IP_HDRINCL, &optval, sizeof(optval)) < 0){
		perror("Failed to set socket options with IP header included");
		exit(0);
	}
	return raw_sock;
}

void send_raw_socket(int source_port, int dest_port, char* address, char* source_ip_addr) {
	int raw_sock = create_raw_socket_headerincluded();

	//Datagram to represent the packet
    char datagram[4096] , source_ip[32] , *data , *pseudogram;
	
    //Clear the the packet buffer
	memset(datagram, 0, 4096); 

    //IP header
	struct ip *iph = (struct ip *) datagram;
	
	//UDP header
	struct udphdr *udph = (struct udphdr *) (datagram + sizeof (struct ip));
	
	struct sockaddr_in sin;
	struct pseudo_header psh;

	sin.sin_family = AF_INET;
	sin.sin_port = htons(dest_port);
	sin.sin_addr.s_addr = inet_addr(address);

	//Data part ip for non linux users
	data = datagram + sizeof(struct ip) + sizeof(struct udphdr);
	strcpy(data , "$group_83$");

    //some address resolution
	strcpy(source_ip , source_ip_addr); 

	
    //Fill in the IP Header there is something wrong according to Stephan
	iph->ip_hl = 5;
	iph->ip_v = 4;
	iph->ip_len = sizeof (struct ip) + sizeof (struct udphdr) + strlen(data);
	iph->ip_off = 1 << 15;
	iph->ip_ttl = 255;
	iph->ip_p = IPPROTO_UDP;
	iph->ip_dst.s_addr = sin.sin_addr.s_addr;
	
    //UDP header
    udph->uh_sport = htons(source_port); //Port does not matter
    udph->uh_dport = htons(dest_port); 


	int psize = sizeof(struct pseudo_header) + sizeof(struct udphdr) + strlen(data);
	pseudogram = (char*)malloc(psize);
	
	
	memcpy(pseudogram , (char*) &psh , sizeof (struct pseudo_header));
	memcpy(pseudogram + sizeof(struct pseudo_header) , udph , sizeof(struct udphdr) + strlen(data));

	//Send the raw socket
	if (sendto (raw_sock, datagram, iph->ip_len, 0, (struct sockaddr *) &sin, sizeof (sin)) < 0) {
        perror("sendto failed");
    }
	else {
		cout << "Packet sent wohoo" << endl;
	}
}

void receivefrom_raw_socket(int port, char* address, char* local_ip_address) {
	//Send a raw socket with ipv4 header
	int source_port_for_raw_socket = 6666;
	send_raw_socket(source_port_for_raw_socket, port, address, local_ip_address);

	//Create udp socket to receive the data from the raw socket
	int udp_receive_sock = socket(AF_INET, SOCK_DGRAM, 0);

    //Set destination address for the udp_receive_sock
	struct sockaddr_in destaddr;
    destaddr.sin_family = AF_INET;
    destaddr.sin_port = htons(source_port_for_raw_socket);
	destaddr.sin_addr.s_addr = inet_addr(local_ip_address);
	socklen_t socklen = sizeof(destaddr);

	// timeout socket check (https://newbedev.com/linux-is-there-a-read-or-recv-from-socket-with-timeout)
    struct timeval tv;
    tv.tv_sec = 0; //seconds 
    tv.tv_usec = 30000; //microseconds
    setsockopt(udp_receive_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*) &tv, sizeof tv);

	//Receive buffer and receive socket length for the recvfrom function
	char receive_buffer[1024];
    unsigned int recv_sock_len = sizeof(destaddr);

	// Create an udp socket to receive the data from the raw socket
	if (udp_receive_sock < 0) {
		perror("Failed to construct the receive udp socket");
	}

	if(bind(udp_receive_sock, (const sockaddr*) &destaddr, socklen) < 0) {
		perror("Failed to bind socket");
	}

	if (recvfrom(udp_receive_sock, receive_buffer, 1024, 0, (sockaddr*) &destaddr, &recv_sock_len) > 0) {
		cout << receive_buffer << endl;
	}
	else {
		cout << "No message" << endl;
	}

}