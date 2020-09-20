all: server client
server: server.c
	gcc server.c -o bin/server -lpthread
client: client.c
	gcc client.c -o bin/client -lpthread
clean:
	rm $(shell pwd)/bin/server
	rm $(shell pwd)/bin/client
