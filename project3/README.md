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

Additionally to connect to any server through the client type:
> CONNECT,SERVER IP ADDRESS,SERVER PORT,GROUP ID

This command should be the first command to send from the client to the server

## Additional commands for Wireshark
We were unable to find the wireshark without using localhost so we used that for client&server.pcapng

To check the file and verify group_21.pcapng:
> ip.addr == 10.3.16.12 && tcp.port == 4021

To check the file and verify client&server.pcapng:
> tcp.port == 43100

## Groups that we sent or receievd from
Instr_1
Instr_2
NUMBER
ORACLE
P3_Group_46
P3_GROUP_38
P3_GROUP_64
P3_GROUP_99
P3_GROUP_2
P3_GROUP_21
P3_GROUP_15
P3_GROUP_62
P3_GROUP_44
P3_GROUP_41
P3_GROUP_42
P3_GROUP_33
P3_GROUP_200
P3_GROUP_128
P3_GROUP_66
P3_GROUP_43
P3_GROUP_45
P3_GROUP_51
P3_GROUP_85
P3_GROUP_54

## BONUSES we did
AK Groups that we connected to:
P3_GROUP_26
P3_GROUP57

We connected our server outside of skel as is evident by the file:
NATServer.pcapng with the filter:
> ip.addr == 153.92.153.173 || ip.addr == 130.208.243.61 || tcp 

on line 1196 is the message that we sent the server