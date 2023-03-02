#include "file.h"
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
 *  Here is the starting point for your netster part.2 definitions. Add the 
 *  appropriate comment header as defined in the code formatting guidelines
 */

/* Add function definitions */
/*
This function is the server. It opens a socket using the host/port and depending on if
UDP is checked on or off. Then it accepts a client. The client will send 256 bytes of data
over a buffer until it has sent the whole file. This funciton will write that
data to a file as it is received.
*/
void file_server(char* iface, long port, int use_udp, FILE* fp) {
  int sockfd, connfd, len, b;
	char strPort[10];
	b = sprintf(strPort, "%ld", port);
	struct sockaddr_in servaddr, cli;
	char buffer[MAX];
	len = sizeof(cli);

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
		}
		while(1) {
			b = recv(connfd, buffer, sizeof(buffer), 0);
			if(b<=0) {
				break;
			}
			fwrite(buffer, 1, b, fp);
			bzero(buffer, MAX);
		}
	} else { // UDP IS ON
		
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
		while(1) {
			b = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*) &cli, (socklen_t *restrict)&len);
			if(b<=0) {
				break;
			}
			fwrite(buffer, 1, sizeof(buffer), fp);
			bzero(buffer, MAX);
		}
	}
}


/*
This is the client function. This function creates a socket and binds
it to the correct host/port and uses UDP if UDP was turned on in the parameters.
Then it will send the file from the parameter to the server in 256 byte buffers.
After it has reached the the of the file, it will send a 0 byte message to indicate
it is done and then it will close the socket.
*/
void file_client(char* host, long port, int use_udp, FILE* fp) {
  	int sockfd, b, data;
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
	char buffer[MAX];
	char buff_addr[4096];
	
	// UDP is OFF, USING TCP
	if (use_udp == 0) {
		b = getaddrinfo(host, strPort, &hints, &res);
		if (b != 0) {
			printf("error getaddrinfo\n");
			exit(EXIT_FAILURE);
		}
		
		struct sockaddr_in* tmp = (struct sockaddr_in*)res->ai_addr; // Cast addr into AF_INET container
  		raw_addr = &(tmp->sin_addr); // Extract the address from the container
		inet_ntop(AF_INET, raw_addr, buff_addr, 4096);

		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0) {
			printf("socket creation failed...\n");
			exit(0);
		}
		bzero(&servaddr, sizeof(servaddr));
		 
		// assign IP, port
		servaddr.sin_family = AF_INET;
		servaddr.sin_addr.s_addr = inet_addr(buff_addr);
		servaddr.sin_port = htons(port);
		// connect the client socket to server socket
		if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
			printf("connection with the server failed...\n");
			exit(0);
		}
		while((data = fread(&buffer, 1, MAX, fp)) > 0) {
			if(send(sockfd, buffer, data, 0) == -1) {
				printf("Error sending file");
				exit(0);
			}
			bzero(buffer, MAX);
		}
		send(sockfd, "", 0, 0);
		close(sockfd);
	} else { // UDP IS ON
	
		b = getaddrinfo(host, strPort, &hints, &res);
		if (b != 0) {
			printf("error getaddrinfo\n");
			exit(EXIT_FAILURE);
		}
		
		struct sockaddr_in* tmp = (struct sockaddr_in*)res->ai_addr; // Cast addr into AF_INET container
  		raw_addr = &(tmp->sin_addr); // Extract the address from the container
		inet_ntop(AF_INET, raw_addr, buff_addr, 4096);
		
		if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) <0) {
			printf("Socket creation failed...\n");
			exit(0);
		}
		
		bzero(&servaddr, sizeof(servaddr));
		
		servaddr.sin_family = AF_INET;
		servaddr.sin_addr.s_addr = inet_addr(buff_addr);
		servaddr.sin_port = htons(port);
		while((data = fread(&buffer, 1, MAX, fp)) > 0) {
			sendto(sockfd, (const char *)buffer, data, MSG_CONFIRM, (struct sockaddr *) &servaddr, sizeof(servaddr));
			bzero(buffer, MAX);
		}
		send(sockfd, "", 0, 0);
		close(sockfd);
	}
}


