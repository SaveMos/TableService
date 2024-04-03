all: server cli td kd

# make rule per il server
server: server.c funzioni.h strutture.h
	gcc server.c -o server -Wall -std=c99 -lpthread

# make rule per il client
cli: client.c funzioni.h strutture.h
	gcc -Wall client.c -o cli -std=c99

# make rule per il table_device
td:	table_device.c funzioni.h strutture.h
	gcc -Wall table_device.c -o td -std=c99

# make rule per il kitchen_device
kd:	kitchen_device.c funzioni.h strutture.h
	gcc -Wall kitchen_device.c -o kd -std=c99

clean:
	rm *o server cli td kd