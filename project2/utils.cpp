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

void send_raw_socket(int source_port, int dest_port, char* dest_ip_addr, char* source_ip_addr, int evil_bit, unsigned int check_sum) {
	char datagram[4096], *data, *pseudogram; 									// Datagram to represent the packet
	int raw_sock 		= create_raw_socket_headerincluded();
	struct ip *iph 		= (struct ip *) datagram; 								// IP header
	struct udphdr *udph = (struct udphdr *) (datagram + sizeof(struct ip)); 	// UDP header
	struct sockaddr_in sin;
	struct pseudo_header psh;

	// Set connection to server
	sin.sin_family = AF_INET;
	sin.sin_port = htons(dest_port);
	sin.sin_addr.s_addr = inet_addr(dest_ip_addr);
	printf("correct:    0x%x\n", check_sum);
	for (unsigned short int i = 0; i < 65536; i++) {
		// Set data
		memset(datagram, 0, sizeof(datagram));
		char k[1] = {(char)(i + '0')};
		data = datagram + sizeof(struct ip) + sizeof(struct udphdr);
        strcpy(data , (char *) k); 
		

		// Fill in the IP Header 
		iph->ip_hl = 5;
		iph->ip_v = 4;
		iph->ip_len = sizeof(struct ip) + sizeof(struct udphdr) + strlen(data);
		iph->ip_id = htons(69);
		iph->ip_off = evil_bit;
		iph->ip_ttl = 255;
		iph->ip_p = IPPROTO_UDP;
		iph->ip_sum = csum((u_short *) datagram, iph->ip_len);
		iph->ip_src.s_addr = inet_addr(source_ip_addr);	// Spoof the source ip address
		iph->ip_dst.s_addr = sin.sin_addr.s_addr;
		
		// Fill in the UDP header
		udph->uh_sport = htons(source_port);
		udph->uh_dport = htons(dest_port);
		udph->uh_ulen = htons(sizeof(struct udphdr) + strlen(data)); // UDP header size

		// Create a pseudoheader to calculate the UDP checksum
		psh.source_address = iph->ip_src.s_addr;
		psh.dest_address = iph->ip_dst.s_addr;
		psh.placeholder = 0;
		psh.protocol = IPPROTO_UDP;
		psh.udp_length = udph->uh_ulen;

		// Calculate checksum
		int psize = sizeof(struct pseudo_header) + sizeof(struct udphdr) + strlen(data);
		pseudogram = (char *) malloc(psize);
		memcpy(pseudogram, (char *) &psh , sizeof(struct pseudo_header));
		memcpy(pseudogram + sizeof(struct pseudo_header) , udph , sizeof(struct udphdr) + strlen(data));

		udph->uh_sum = csum((u_short*) pseudogram , psize);
		
		//std::cout << udph->uh_sum << std::endl;

		// Print for debug
		if (udph->uh_sum == check_sum) {
			break;
		}
	}
	printf("calculated: 0x%x\n", udph->uh_sum);
	
	// Send the raw socket
	if (sendto(raw_sock, datagram, iph->ip_len, 0, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
        perror("Failed to send the raw socket!");
    }
}

void receivefrom_raw_socket(int port, char* dest_ip_addr, char* local_ip_address) {
	// Send a raw socket with ipv4 header
	int source_port_for_raw_socket = 6666;
	send_raw_socket(source_port_for_raw_socket, port, dest_ip_addr, local_ip_address, 1 << 15, 0);

	// Create udp socket to receive the data from the raw socket
	int udp_receive_sock = socket(AF_INET, SOCK_DGRAM, 0);

    // Set destination address for the udp_receive_sock
	struct sockaddr_in destaddr;
    destaddr.sin_family = AF_INET;
    destaddr.sin_port = htons(source_port_for_raw_socket);
	destaddr.sin_addr.s_addr = inet_addr(local_ip_address);
	socklen_t socklen = sizeof(destaddr);

	// Timeout socket check (https:// Newbedev.com/linux-is-there-a-read-or-recv-from-socket-with-timeout)
    struct timeval tv;
    tv.tv_sec = 0; // Seconds 
    tv.tv_usec = 30000; // Microseconds
    setsockopt(udp_receive_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*) &tv, sizeof tv);

	// Receive buffer and receive socket length for the recvfrom function
	char receive_buffer[1024];
    unsigned int recv_sock_len = sizeof(destaddr);

	//  Create an udp socket to receive the data from the raw socket
	if (udp_receive_sock < 0) {
		perror("Failed to construct the receive udp socket");
	}

	if(bind(udp_receive_sock, (const sockaddr*) &destaddr, socklen) < 0) {
		perror("Failed to bind socket");
		exit(0);
	}

	if (recvfrom(udp_receive_sock, receive_buffer, 1024, 0, (sockaddr*) &destaddr, &recv_sock_len) > 0) {
		std::cout << receive_buffer << std::endl;
	}
	else {
		std::cout << "No message" << std::endl;
	}
}