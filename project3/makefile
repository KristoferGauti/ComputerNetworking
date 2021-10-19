CC = g++
CPP = c++11
all: clean client server
client:
	${CC} -Wall -std=${CPP} -pthread client.cpp -o client
server:
	${CC} -Wall -std=${CPP} server.cpp -o server
ip:
	${CC} -Wall -std=${CPP} ip.cpp -o ip
clean:
	rm -f core client server ip
