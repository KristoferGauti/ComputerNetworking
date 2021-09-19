//To run the scanner: ./scanner 130.208.242.120 4000 4100

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <thread>


using namespace std;


void scan_ports(int from_port_nr, int destination_port_number, struct sockaddr_in destaddr) {
    int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in recvaddr;
    unsigned int recv_sock_length;
    int buffer_length = 64;
    char send_buffer[buffer_length];
    char receive_buffer[buffer_length];
    strcpy(send_buffer, "Hi port!");
    //printf("port from: %d port to: %d\n", from_port_nr, destination_port_number);

    // Create an udp socket which is connectionless (no three way handshake like TCP)
    if (udp_sock < 0) {
        perror("Unable to open socket!");
        exit(0);
    }

    // timeout socket check
    // code from https://newbedev.com/linux-is-there-a-read-or-recv-from-socket-with-timeout
    struct timeval tv;
    tv.tv_sec = 0; //seconds 
    tv.tv_usec = 50000; //microseconds
    setsockopt(udp_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*) &tv, sizeof tv);

    /*
        Some packets might be dropped, thus we need to send 10 times in a row to check on that
        The inner for loop does that. The more you send with UDP, the more reliable UDP becomes
    */
    for (int port = from_port_nr; port <= destination_port_number; port++) {  
        destaddr.sin_port = htons(port);
        for (int i = 0; i <= 10; i++) {
            if (sendto(udp_sock, send_buffer, buffer_length, 0, (const struct sockaddr*) &destaddr, sizeof(destaddr)) < 0) {
                perror("Failed to send!");
            }
            else {
                // recvfrom is a blocking function. 
                if (recvfrom(udp_sock, receive_buffer, buffer_length, 0, (sockaddr*) &recvaddr, &recv_sock_length) > 0) {
                    cout << "port " << port << " is open!" << endl;
                    break;
                }
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if(argc != 4) {
	    printf("Not enough arguments were given!\n");
        printf("Usage: ./scanner <ip address> <low port> <high port>\n");
        exit(0);
    }

    struct sockaddr_in destaddr;
    char* address = argv[1];
    int from_port_nr = atoi(argv[2]);
    int destination_port_number = atoi(argv[3]);
    int number_of_threads = 50;

    // Calculating the workload on each thread
    int total_ports = destination_port_number - from_port_nr + 1;
    int thread_workload = total_ports / number_of_threads;
    int overwork = total_ports - thread_workload * number_of_threads;
    thread thread_list[number_of_threads];

    // Setup the address
    destaddr.sin_family = AF_INET;
    inet_aton(address, &destaddr.sin_addr);

    // Threads
    /* 
        Got a help from my friend ymir19@ru.is for understanding how 
        to use threads to speed up the execution
    */
    int not_last_worker_port = 1;
    for (int i=0; i < number_of_threads; i++) {
        int work = thread_workload;
        if (i < overwork) {
            work++;
        }
        if ((i + 1) == number_of_threads) {
            not_last_worker_port = 0;
        }
        int port_range_from = from_port_nr + i * work;
        int port_range_to = from_port_nr + (i + 1) * work - not_last_worker_port;
        // This is like thread(scan_ports(udp_sock, from_port_nr, destination_port_number, destaddr));
        thread_list[i] = thread(scan_ports, port_range_from, port_range_to, destaddr);
    }

    // To join the threads to avoid the program to close early
    for (int i=0; i < number_of_threads; i++) {
        thread_list[i].join();
    }

    return 0;
}
