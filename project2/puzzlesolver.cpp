#include <iostream>


int main(int argc, char* argv[]) {

    // Checking if correct number of arguments are given
    if(!((argc == 6) || (argc == 2))) {
	    printf("Incorrect number of arguments!\n");
        printf("Usage: ./scanner <ip address> OR\n       ./scanner <ip address> <port1> <port2> <port3> <port4>\n");
        exit(0);
    }
}
