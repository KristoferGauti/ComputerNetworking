//To run the scanner: ./scanner 130.208.242.120 4000 4100

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>


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
    struct sockaddr_in destaddr;
    struct sockaddr_in recvaddr;
    unsigned int recv_sock_length;
    int buffer_length = 1500;
    char send_buffer[buffer_length];
    char receive_buffer[buffer_length];
    strcpy(send_buffer, "Hi port!");


    //Create an udp socket which is connectionless (no three way handshake like TCP)
    if (udp_sock < 0) {
        perror("Unable to open socket!");
        exit(0);
    }

    //Setup the address
    destaddr.sin_family = AF_INET;
    inet_aton(address, &destaddr.sin_addr);

    //timeout socket check
    //code from https://newbedev.com/linux-is-there-a-read-or-recv-from-socket-with-timeout
    struct timeval tv;
    tv.tv_sec = 0; //seconds 
    tv.tv_usec = 10000; //microseconds
    setsockopt(udp_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*) &tv, sizeof tv);

    /*
        some packets might be dropped, thus we need to send 5 times in a row to check on that
        inner for loop to do that
    */
    for (int port = from_port_nr; port <= destination_port_number; port++) {  
        destaddr.sin_port = htons(port);
        if (sendto(udp_sock, send_buffer, buffer_length, 0, (const struct sockaddr*) &destaddr, sizeof(destaddr)) < 0) {
            perror("Failed to send!");
        }
        else {
            //recvfrom is a blocking function. 
            recvfrom(udp_sock, receive_buffer, buffer_length, 0, (sockaddr*) &recvaddr, &recv_sock_length); 
            //cout << receive_buffer << endl;

            //print the port number when a msg is received
            if (strlen(receive_buffer) > 0) {
                cout << "port " << port << " is open!" << endl;
            }
            
            //Clear the buffer
            memset(receive_buffer, 0, sizeof(receive_buffer));
            
        }


    }

    return 0;
}
