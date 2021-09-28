# Project 2 TSAM

To run this massive project you have to find your local ip first. In order to find your local ip you must use this command in the terminal

```
    ifconfig
```

You will be prompted with your computer's interfaces. Scroll to interface en0 and there you will find your local ip address under the label inet.

# How to run the program

Change your current working directory to project2 and run the program using this command.

```
    make && sudo ./puzzlesolver <src_port> <your_local_ip_address>
```

# puzzlesolver.cpp

line 38 - create the list of ports to send to
line 44 - assign the secret port from the evil bit part to the variable evil_bit_secret_port
