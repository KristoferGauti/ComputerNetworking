//To run the scanner: ./scanner 130.208.242.12 4000 4100

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


using namespace std;

int main(int argc, char* argv[]) {
    if(argc != 4) {
	    printf("Not enough arguments were given!\n");
        printf("Usage: ./scanner <ip address> <low port> <high port>\n");
        exit(0);
    }

    int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    char* address = argv[1];
    int from_port_nr = atoi(argv[2]);
    int destination_port_number = atoi(argv[3]);
    char buffer[1400];
    struct sockaddr_in destaddr;


    if (udp_sock < 0) {
        perror("Unable to open socket!");
        exit(0);
    }


    destaddr.sin_family = AF_INET;
    inet_aton(address, &destaddr.sin_addr);


    cout << buffer << endl;
    cout << from_port_nr << endl;
    cout << destination_port_number << endl;

    return 0;
}
