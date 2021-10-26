# Project 3 The Botnet returns

This is the third project in the course Computer Networks (otherwise known as TSAM).
It focuses on creating our own server and connecting him to other servers

## Prerequisites
### **g++ compiler**, how to download:

For Windows: https://techsupportwhale.com/install-gcc-compiler-on-windows/

For Linux/Ubuntu:   `sudo apt install g++`

### **make**, how to download:
For Windows: http://gnuwin32.sourceforge.net/packages/make.htm

For Linux/Ubuntu:
`sudo apt install make`


## Compile
There are two ways to compile the code for this application. Open the terminal and travel to the build directory.

1. execute `make` or
2. execute
    * `g++ -Wall -std=c++11 server.cpp -o server -pthread`
    * `g++ -Wall -std=c++11 client.cpp -o client -pthread`
## Running the applications
For the server run:
> ./server <your_port>

For the client run:
> ./client <IP_address> <server_port>

The ports are integers and the ip is in the format: xxx.x.x.x and the x's are integers.

## Additional commands for Wireshark
To check the file and verify group_21.pcapng:
> ip.addr == 10.3.16.12 && tcp.port == 4021

To check the file and verify group_ 

## BONUSES we did