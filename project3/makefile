CC = g++
CPP = c++11
all: clean client server
client:
	${CC} -Wall -std=${CPP} client.cpp -o client
server:
	${CC} -Wall -std=${CPP} server.cpp -o server
clean:
	rm -f core client server
