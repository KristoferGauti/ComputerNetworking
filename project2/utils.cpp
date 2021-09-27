#include <iostream>
#include "utils.h"
#include "scanner.h"

struct pseudo_header
{
	u_int32_t source_address;
	u_int32_t dest_address;
	u_int8_t placeholder;
	u_int8_t protocol;
	u_int16_t udp_length;
};

/* Utility functions */

std::pair<unsigned int, std::string> parse_message_get_checksum_srcip(char* receive_buffer) {
	std::string message = std::string(receive_buffer);
    std::string checksum_hex = message.substr(144,6);
	unsigned int converted_checksum = stoul(checksum_hex, nullptr, 16);
    int ip_start_idx = (int)message.find("being ")+6;
	int ip_end_idx = ((int)message.find("! (") ) - ip_start_idx;
    std::string source_addr = message.substr(ip_start_idx, ip_end_idx); 
	return make_pair(converted_checksum, source_addr);
}

void print_list(std::vector<int> vec) {
    std::cout << "The ports that are in the list are as follows: \n";
    for (int i = 0; i <= vec.size()-1; i++) {
        std::cout << vec[i] << std::endl;
    }
}


u_short csum(u_short *ptr, int nbytes) {
    long sum;
    u_short oddbyte;
    short answer;

    sum = 0;
    while (nbytes > 1) {
        sum += *ptr++;
        nbytes -= 2;
    }

    if (nbytes == 1) {
        oddbyte = 0;
        *((u_char*) &oddbyte) = *(u_char *) ptr;
        sum += oddbyte;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum = sum + (sum >> 16);
    answer = (short) ~sum;

    return answer;
}


/* Socket functions */

int create_raw_socket_headerincluded() {
	// Create a raw socket of type IPPROTO
	int raw_sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);

	if (raw_sock < 0) {
        perror("Failed to create raw socket");
		exit(0);
    }

	// Set socket options with the header included
	int optval = 1;
	if(setsockopt(raw_sock, IPPROTO_IP, IP_HDRINCL, &optval, sizeof(optval)) < 0){
		perror("Failed to set socket options with IP header included");
		exit(0);
	}
	return raw_sock;
}

// Evil bit
void evil_bit_part(int port, char* address, char* local_ip_address) {
    //Create a raw socket of type IPPROTO
	int raw_sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (raw_sock < 0) {
        perror("Failed to create raw socket");
		exit(1);
    }

	// Set the header include option
	int optval = 1;
	int sso = setsockopt(raw_sock, IPPROTO_IP, IP_HDRINCL, &optval, sizeof(optval));

	if(sso < 0){
		perror("Failed to set socket options with IP header included");
		exit(0);
	}

    //Datagram to represent the packet
    char datagram[4096] , source_ip[32] , *data;
	
    //Clear the the packet buffer
	memset(datagram, 0, 4096); 

    //IP header
	struct ip *iph = (struct ip *) datagram;
	
	//UDP header
	struct udphdr *udph = (struct udphdr *) (datagram + sizeof (struct ip));
	
	struct sockaddr_in sin;
	struct pseudo_header psh;
	
	//Data part ip for non linux users
	data = datagram + sizeof(struct ip) + sizeof(struct udphdr);
	strcpy(data , "$group_89$");

    //some address resolution
	strcpy(source_ip , local_ip_address); 
	
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	sin.sin_addr.s_addr = inet_addr(address);
	
    //Fill in the IP Header there is something wrong according to Stephan
	iph->ip_hl = 5;
	iph->ip_v = 4;
	iph->ip_tos = 0;
	iph->ip_len = sizeof(struct ip) + sizeof(struct udphdr) + strlen(data);
	iph->ip_id = htons(69);	//Id of this packet
	iph->ip_off = 1 << 15;
	iph->ip_ttl = 255;
	iph->ip_p = IPPROTO_UDP;
	iph->ip_sum = 0;		//Set the checksum 0 
	iph->ip_src.s_addr = inet_addr (source_ip);	//Spoof the source ip address
	iph->ip_dst.s_addr = sin.sin_addr.s_addr;
	
    //UDP header
    udph->uh_sport = htons(6666);
    udph->uh_dport = htons(port); 
    udph->uh_sum = 0;
    udph->uh_ulen = htons(sizeof(struct udphdr) + strlen(data)); //udp header size

    //Now the UDP checksum
	psh.source_address = 6969;
	psh.dest_address = sin.sin_addr.s_addr;
	psh.placeholder = 0;
	psh.protocol = IPPROTO_UDP;
	psh.udp_length = htons(sizeof(struct udphdr) + strlen(data));

	// Create an udp socket to receive the data from the raw socket
    int udp_receive_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (udp_receive_sock < 0) {
		perror("Failed to construct the receive udp socket");
	}

	struct sockaddr_in destaddr;

    //Set destination address
    destaddr.sin_family      = AF_INET;
    destaddr.sin_addr.s_addr = inet_addr(source_ip);
    destaddr.sin_port        = htons(6666);
	socklen_t len = sizeof(destaddr);


	if(bind(udp_receive_sock, (const sockaddr*) &destaddr, len) < 0) {
		perror("Failed to bind socket");
		exit(0);
	}

	//Send the raw socket
	if (sendto (raw_sock, datagram, iph->ip_len, 0, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
        perror("sendto failed");
    }
	else {
		std::cout << "Packet sent wohoo" << std::endl;
	}


	// // timeout socket check
    // // code from https://newbedev.com/linux-is-there-a-read-or-recv-from-socket-with-timeout
    struct timeval tv;
    tv.tv_sec = 0; //seconds 
    tv.tv_usec = 30000; //microseconds
    setsockopt(udp_receive_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*) &tv, sizeof tv);

	
	char receive_buffer[1024];
	//struct sockaddr_in recvaddr;
    unsigned int recv_sock_len = sizeof(destaddr);
	if (recvfrom(udp_receive_sock, receive_buffer, 1024, 0, (sockaddr*) &destaddr, &recv_sock_len) > 0) {
		std::cout << receive_buffer << std::endl;
	}
	else {
		std::cout << "No message" << std::endl;
	}
}

//Checksum part
void checksum_part(int source_port, int dest_port, char* dest_ip_addr, char* source_ip_addr, char* local_ip_address, unsigned int check_sum) {
	char datagram[4096], *pseudogram; 									// Datagram to represent the packet
	struct ip *iph 		= (struct ip *) datagram; 								// IP header
	struct udphdr *udph = (struct udphdr *) (datagram + sizeof(struct ip)); 	// UDP header
	struct sockaddr_in sin;
	struct pseudo_header psh;
	char* buffer = (char*) (datagram + sizeof(struct ip) + sizeof(struct udphdr));

	// Set connection to server
	sin.sin_family = AF_INET;
	sin.sin_port = htons(dest_port);
	sin.sin_addr.s_addr = inet_addr(dest_ip_addr);
	
	// clear the datagram
	memset(datagram, 0, 4096);

	// Fill in the IP Header 
	iph->ip_hl = 5;
	iph->ip_v = 4;
	iph->ip_tos = 0;
	iph->ip_len = htons(sizeof(struct ip) + sizeof(struct udphdr) + 2);
	iph->ip_id = 69;
	iph->ip_off = 0;
	iph->ip_ttl = 255;
	iph->ip_p = IPPROTO_UDP;
	iph->ip_sum = 0;
	iph->ip_src.s_addr = inet_addr(source_ip_addr);	// Spoof the source ip address
	iph->ip_dst.s_addr = sin.sin_addr.s_addr;
	
	// Fill in the UDP header
	udph->uh_sport = htons(source_port);
	udph->uh_dport = htons(dest_port);
	udph->uh_ulen = htons(sizeof(struct udphdr) + 2); // UDP header size
	udph->uh_sum = 0;

	// Create a pseudoheader to calculate the UDP checksum
	psh.source_address = inet_addr(source_ip_addr);
	psh.dest_address = sin.sin_addr.s_addr;
	psh.placeholder = 0;
	psh.protocol = IPPROTO_UDP;
	psh.udp_length = htons(udph->uh_ulen);

	iph->ip_sum = htons(csum((u_short *) datagram, iph->ip_len));
	udph->uh_sum = htons((u_short) check_sum);

	// Calculate checksum
	int psize = sizeof(struct pseudo_header) + sizeof(struct udphdr) + 2;
	pseudogram = (char *) malloc(psize);
	unsigned short data_checksum = csum((unsigned short*) pseudogram, psize);
	memcpy(pseudogram, (char *) &psh , sizeof(struct pseudo_header));
	memcpy(pseudogram + sizeof(struct pseudo_header) , udph , sizeof(struct udphdr) + 2);
	memcpy(pseudogram + sizeof(struct pseudo_header) + sizeof(struct udphdr), buffer, 2);
	memcpy(buffer, &data_checksum, 2);

	
	printf("correct:    0x%x\n", check_sum);
	printf("calculated: 0x%x\n", udph->uh_sum);
	

	int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (udp_socket < 0) {
		perror("Failed to construct the receive udp socket");
	}

	struct sockaddr_in destaddr;

    //Set destination address
    destaddr.sin_family      = AF_INET;
    destaddr.sin_addr.s_addr = inet_addr(local_ip_address);
    destaddr.sin_port        = htons(dest_port);
	socklen_t len = sizeof(destaddr);


	if(bind(udp_socket, (const sockaddr*) &destaddr, len) < 0) {
		perror("Failed to bind socket");
	}
	if (sendto(udp_socket, datagram, htons(iph->ip_len), 0, (struct sockaddr*) &sin, sizeof(sin)) < 0) {
		perror("Failed to send the udp socket with the right checksum");
	}
	else {
		std::cout << "Checksum ipv4 header packet sent" << std::endl;
	}

	// // timeout socket check
    // // code from https://newbedev.com/linux-is-there-a-read-or-recv-from-socket-with-timeout
    struct timeval tv;
    tv.tv_sec = 0; //seconds 
    tv.tv_usec = 30000; //microseconds
    setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*) &tv, sizeof tv);

	
	char receive_buffer[1024];
	//struct sockaddr_in recvaddr;
    unsigned int recv_sock_len = sizeof(destaddr);
	if (recvfrom(udp_socket, receive_buffer, sizeof(receive_buffer), 0, (sockaddr*) &destaddr, &recv_sock_len) > 0) {
		std::cout << receive_buffer << std::endl;
	}
	else {
		std::cout << "No message" << std::endl;
	}
}

// void receivefrom_raw_socket(int port, char* dest_ip_addr, char* local_ip_address) {
// 	// Create udp socket to receive the data from the raw socket
// 	int udp_receive_sock = socket(AF_INET, SOCK_DGRAM, 0);

//     // Set destination address for the udp_receive_sock
// 	struct sockaddr_in destaddr;
//     destaddr.sin_family = AF_INET;
//     destaddr.sin_port = htons(port);
// 	destaddr.sin_addr.s_addr = inet_addr(local_ip_address);
// 	socklen_t socklen = sizeof(destaddr);

// 	// Timeout socket check (https:// Newbedev.com/linux-is-there-a-read-or-recv-from-socket-with-timeout)
//     struct timeval tv;
//     tv.tv_sec = 0; // Seconds 
//     tv.tv_usec = 30000; // Microseconds
//     setsockopt(udp_receive_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*) &tv, sizeof tv);

// 	// Receive buffer and receive socket length for the recvfrom function
// 	char receive_buffer[1024];
//     unsigned int recv_sock_len = sizeof(destaddr);

// 	//  Create an udp socket to receive the data from the raw socket
// 	if (udp_receive_sock < 0) {
// 		perror("Failed to construct the receive udp socket");
// 		exit(0);
// 	}

// 	if(bind(udp_receive_sock, (const sockaddr*) &destaddr, socklen) < 0) {
// 		perror("Failed to bind socket");
// 		exit(0);
// 	}

// 	if (recvfrom(udp_receive_sock, receive_buffer, 1024, 0, (sockaddr*) &destaddr, &recv_sock_len) > 0) {
// 		std::cout << receive_buffer << std::endl;
// 	}
// 	else {
// 		std::cout << "No message" << std::endl;
// 	}
// }