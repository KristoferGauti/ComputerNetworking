CC = g++
CPP = c++11
all: clean client server
client:
	${CC} -Wall -std=${CPP} client.cpp -o client -pthread
server:
	${CC} -Wall -std=${CPP} server.cpp -o server -pthread
ip:
	${CC} -Wall -std=${CPP} ip.cpp -o ip
clean:
	rm -f core client server ip
