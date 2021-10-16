CC = g++
CPP = c++11
all: clean chat_client chat_server
chat_client:
	${CC} -Wall -std=${CPP} client.cpp -o chat_client
chat_server:
	${CC} -Wall -std=${CPP} server.cpp -o chat_server
clean:
	rm -f core chat_client chat_server
