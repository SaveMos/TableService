This is a client-server application written in C.
The application permit to:
- create table devices that manage a table order from customers (table_device.c).
- create a kitchen device that manage all the food request of the customers to the cooks (kitchen_device.c).
- Expose a command line interface for the customer (client.c) that permits him to book a table at the restaurant.
The server that accept all those request use a multithread mechanism to communicate with every client, moreover every communication is done with TCP sockets.
Use the file exec2023.sh to compile and execute a test case of the application.
