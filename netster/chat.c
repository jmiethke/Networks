#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#define MAX 256 // Max allowed for client messages
#define SA struct sockaddr
/*
 *  Here is the starting point for your netster part.1 definitions. Add the 
 *  appropriate comment header as defined in the code formatting guidelines
 */

/* Add function definitions */

/*
Author: John Miethke username: jmiethke
CREATED: 10/12/22
This is a program that allows a user to start a server and a client and communicate through sockets.
There are three special phrases "hello", "goodbye", and "exit" that the server responds to uniquely,
whereas all other messages the server will send the message back to the client.
*/



void chat_server(char* iface, long port, int use_udp) {
	int sockfd, connfd, len, a, count, current, b;
	unsigned int clientPort;
	char clientIP[16];
	char strPort[10];
	b = sprintf(strPort, "%ld", port);
	count = 0; // Starting at connection 0 will iterate up by 1
	current = 0; // The amount of current connections
	struct sockaddr_in servaddr, cli;
	socklen_t length;
		
	len = sizeof(cli);
	char buffer[MAX];
	
	struct addrinfo hints;
	struct addrinfo *res;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = IPPROTO_TCP;
	
	// UDP is OFF, USING TCP
	if (use_udp == 0) {
		
		b = getaddrinfo(iface, strPort, &hints, &res);
		if (b != 0) {
			   printf("error getaddrinfo\n");
               exit(EXIT_FAILURE);
        }
			
		sockfd = socket(AF_INET, SOCK_STREAM, 0);

		if (sockfd == -1) {
				printf("socket creation failed...\n");
				exit(0);
		}
		bzero(&servaddr, sizeof(servaddr));

		servaddr.sin_family = AF_INET;
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		servaddr.sin_port = htons(port);
		// Setting a socket option to reuse the port
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
			printf("set socket option failed...\n");
		}
		// Binding the socket
		if ((bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) != 0) {
				printf("socket bind failed...\n");
				exit(0);
		}
		if ((listen(sockfd, 5)) != 0) {
				printf("Listen failed...\n");
				exit(0);
		}
		connfd = accept(sockfd, (struct sockaddr*)&cli, (socklen_t *restrict)&len);
		if (connfd < 0) {
			printf("server accept failed...\n");
			exit(0);
		} else { // Client connected
			length = sizeof(servaddr);
			a = getsockname(sockfd, (struct sockaddr *) &servaddr, &length);
			inet_ntop(AF_INET, &cli.sin_addr, clientIP, sizeof(clientIP));
			clientPort = ntohs(cli.sin_port);
			printf("connection %d from ('%s', %d)\n", count, clientIP, clientPort);
			count++;
			current++;
		}
		for(;;){
			if (current != 0) {
				a = read(connfd, buffer, sizeof(buffer));
				a = a + 1; // Fluff, using a so I can compile. I don't know why the compiler forces me to "use" a.
				printf("got message from ('%s', %d)\n", clientIP, clientPort);

				if (strncmp(buffer, "hello", 5) == 0) {
					a = write(connfd, "world", 5);
				}
				// Closes connection with client and keeps server running
				else if (strncmp(buffer, "goodbye", 7) == 0 || strncmp(buffer, "farewell", 8) == 0) {
					a = write(connfd, "farewell", 8);
					current--;
				}
				// Turns off the server
				else if (strncmp(buffer, "exit", 4) == 0) {
					a = write(connfd, "ok", 2);
					break;
				}
				else {
					a = write(connfd, buffer, sizeof(buffer));
				}
			} else {
				if ((listen(sockfd, 5)) != 0) {
						printf("Listen failed...\n");
						exit(0);
					}
				len = sizeof(cli);

				connfd = accept(sockfd, (SA*)&cli, (socklen_t *restrict)&len);
				if (connfd < 0) {
					printf("server accept failed...\n");
					exit(0);
				} else { // Client connected
					bzero(&buffer, sizeof(buffer));
					length = sizeof(servaddr);
					a = getsockname(sockfd, (struct sockaddr *) &servaddr, &length);
					inet_ntop(AF_INET, &cli.sin_addr, clientIP, sizeof(clientIP));
					clientPort = ntohs(cli.sin_port);
					printf("connection %d from ('%s', %d)\n", count, clientIP, clientPort);
					count++;
					current++;
				}
			}
		}
	} else { // UDP IS ON

		char input[MAX];
		char output[MAX];
		
		b = getaddrinfo(iface, strPort, &hints, &res);
		if (b != 0) {
			   printf("error getaddrinfo\n");
               exit(EXIT_FAILURE);
        }
		
		if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			printf("socket creation failed...\n");
			exit(0);
		}
		
		bzero(&servaddr, sizeof(servaddr));
		
		servaddr.sin_family = AF_INET;
		servaddr.sin_addr.s_addr = INADDR_ANY;
		servaddr.sin_port = htons(port);
		
		if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) { 
			printf("socket bind failed...\n");
			exit(0);
		}
		
		for (;;) {
			bzero(input, sizeof(input));
			bzero(output, sizeof(output));
			if (recvfrom(sockfd, (char *)input, sizeof(input), 0, (struct sockaddr*) &cli, (socklen_t *restrict)&len) <0) {
				printf("Didn't receive\n");
			} else {
				a = getsockname(sockfd, (struct sockaddr *) &servaddr, &length);
				inet_ntop(AF_INET, &cli.sin_addr, clientIP, sizeof(clientIP));
				clientPort = ntohs(cli.sin_port);
				printf("got message from ('%s', %d)\n", clientIP, clientPort);
				if (strncmp(input, "hello", 5) == 0) {
						strcpy(output, "world");
						sendto(sockfd, (const char *)output, strlen(output), MSG_CONFIRM, (struct sockaddr *) &cli, sizeof(cli));
				}
				// Closes connection with client and keeps server running
				else if (strncmp(input, "goodbye", 7) == 0 || strncmp(buffer, "farewell", 8) == 0) {
					strcpy(output, "farewell");
					sendto(sockfd, (const char *)output, strlen(output), MSG_CONFIRM, (struct sockaddr *) &cli, sizeof(cli));
				}
				// Turns off the server
				else if (strncmp(input, "exit", 4) == 0) {
					strcpy(output, "ok");
					sendto(sockfd, (const char *)output, strlen(output), MSG_CONFIRM, (struct sockaddr *) &cli, sizeof(cli));
					break;
				}
				else {
					strcpy(output, input);
					sendto(sockfd, (const char *)output, strlen(output), MSG_CONFIRM, (struct sockaddr *) &cli, sizeof(cli));
				}
			}			
		}
	}
}

void chat_client(char* host, long port, int use_udp) {
	int sockfd, a, b;
	char strPort[10];
	b = sprintf(strPort, "%ld", port);
	struct sockaddr_in servaddr;
	struct addrinfo hints;
	struct addrinfo *res;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = IPPROTO_TCP;
	void* raw_addr;
	char buffer[4096];
	
	char input[MAX];
	char output[MAX];
	
	// UDP is OFF, USING TCP
	if (use_udp == 0) {
		b = getaddrinfo(host, strPort, &hints, &res);
		if (b != 0) {
			printf("error getaddrinfo\n");
			exit(EXIT_FAILURE);
		}
		
		struct sockaddr_in* tmp = (struct sockaddr_in*)res->ai_addr; // Cast addr into AF_INET container
  		raw_addr = &(tmp->sin_addr); // Extract the address from the container
		inet_ntop(AF_INET, raw_addr, buffer, 4096);

		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd == -1) {
			printf("socket creation failed...\n");
			exit(0);
		}
		bzero(&servaddr, sizeof(servaddr));
		 
		// assign IP, port
		servaddr.sin_family = AF_INET;
		servaddr.sin_addr.s_addr = inet_addr(buffer);
		servaddr.sin_port = htons(port);
		// connect the client socket to server socket
		if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
			printf("connection with the server failed...\n");
			exit(0);
		}
		for (;;) {
			bzero(input, sizeof(input));
			bzero(output, sizeof(output));
			if(fgets(output, MAX, stdin) == NULL) {
				printf("error");
			}
			a = write(sockfd, output, sizeof(output));
			a = a + 1; // added fluff so I can compile. Don't know why I need to "use" a but the compiler forces me to
			a = read(sockfd, input, sizeof(input));
			printf("%s", input);
			if (strncmp(input, "farewell", 8) == 0) {
				break;
			}
			else if (strncmp(input, "ok", 2) == 0) {
				break;
			}
		}
	} else { // UDP IS ON
	
		b = getaddrinfo(host, strPort, &hints, &res);
		if (b != 0) {
			printf("error getaddrinfo\n");
			exit(EXIT_FAILURE);
		}
		
		struct sockaddr_in* tmp = (struct sockaddr_in*)res->ai_addr; // Cast addr into AF_INET container
  		raw_addr = &(tmp->sin_addr); // Extract the address from the container
		inet_ntop(AF_INET, raw_addr, buffer, 4096);
		
		if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) <0) {
			printf("Socket creation failed...\n");
			exit(0);
		}
		
		bzero(&servaddr, sizeof(servaddr));
		
		servaddr.sin_family = AF_INET;
		servaddr.sin_addr.s_addr = inet_addr(buffer); 
		servaddr.sin_port = htons(port);
		
		int len;
		for (;;) {
			bzero(input, sizeof(input));
			bzero(output, sizeof(output));
			if(fgets(output, MAX, stdin) == NULL) {
					printf("error");
			}
			sendto(sockfd, (const char *)output, strlen(output), MSG_CONFIRM, (struct sockaddr *) &servaddr, sizeof(servaddr));
			if (recvfrom(sockfd, (char *)input, sizeof(input), 0, (struct sockaddr*) &servaddr, (socklen_t *restrict)&len) <0) {
				printf("Didn't receive\n");
			} else {
				printf("%s", input);
				if (strncmp(input, "farewell", 8) == 0) {
					break;
				}
				else if (strncmp(input, "ok", 2) == 0) {
					break;
				}
			}
		}
	}
}

