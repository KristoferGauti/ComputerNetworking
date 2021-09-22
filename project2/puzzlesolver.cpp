// To run the program ./puzzlesolver 130.208.242.120

#include <iostream>
#include "scanner.h"
#include "utils.h"

int main(int argc, char* argv[]) {

    // Checking if correct number of arguments are given
    if(argc != 2) {
	    printf("Incorrect number of arguments!\n");
        printf("Usage: ./puzzlesolver <ip address> OR\n");       
        exit(0);
    }

    // Setup the address
    struct sockaddr_in destaddr;
    destaddr.sin_family = AF_INET;
    inet_aton(argv[1], &destaddr.sin_addr);


    list<int> ports = scan_ports(4000, 4100, destaddr);

    print_list(ports);
}
