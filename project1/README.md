# Project 1 - Client/Server
A client sends commands by executing SYS <command> where command is a linux command

# How to run the project
Use the make command to compile both the server and the client code. You need to be located in the right directory, project1.
```
    make
```

After the compilation you must run the server by executing this command along with an ip port
```
    ./server <ip port>
```

Run the client on another terminal by executing this command along with an ip address and an ip port
```
    ./client <ip address> <ip port>
```