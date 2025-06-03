all: client server

client: client.o stream.o
	gcc client.o stream.o -o client

server: server.o stream.o
	gcc server.o stream.o -o server

client.o: client.c stream.h
	gcc -c client.c -o client.o

server.o: server.c stream.h
	gcc -c server.c -o server.o

stream.o: stream.c stream.h
	gcc -c stream.c -o stream.o

clean:
	rm -f *.o client server