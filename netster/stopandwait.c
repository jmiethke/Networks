#include "file.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#define MAX 256 // Max allowed for client messages
#define SA struct sockaddr

/*
 *  Here is the starting point for your netster part.3 definitions. Add the 
 *  appropriate comment header as defined in the code formatting guidelines
 */

/* Add function definitions */

typedef struct packet{
	char data[MAX];
}Packet;

typedef struct frame{
	int frame_kind; // ACK = 0, SEQ = 1, FIN = 2
	int sq_no;
	Packet packet;
}Frame;


/*
This is a function that implements the Stop and Wait protocol using UDP.
It will bind to a socket using the port provided from the parameters and then listen
for incoming sent files. This file will be created server side under the name
given by the user. To make sure it has all the data of the file, it implements
a frame that is acknowledged back and forth between the server and the client
making sure no data is lost.
*/
void stopandwait_server(char* iface, long port, FILE* fp) {
	int sockfd, b, len;
	char strPort[10];
	b = sprintf(strPort, "%ld", port);
	struct timeval tv;
	struct sockaddr_in servaddr, cli;
	len = sizeof(cli);
	Frame ackframe, recv_frame;
	int frame_id, recv_result;


	struct addrinfo hints;
	struct addrinfo *res;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = IPPROTO_TCP;
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
	// set socket option - timeout is 1 second
	tv.tv_sec = 0;
	tv.tv_usec = 100000;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(struct timeval));

	if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) { 
		printf("socket bind failed...\n");
		exit(0);
	}
	frame_id = 0;
	ackframe.sq_no = 0;
	ackframe.frame_kind = 0; // 0 = ACK
	//char buffer[MAX];
	int count = 0;
	while(1) {
	//	bzero(buffer, MAX);
		memset(recv_frame.packet.data, 0x00, MAX);
		//bzero(&recv_frame.packet.data, MAX);
		while(1){
			recv_result = recvfrom(sockfd, &recv_frame, sizeof(recv_frame), MSG_WAITALL, (struct sockaddr*) &cli, (socklen_t *)&len);
			ackframe.sq_no = recv_frame.sq_no + 1;
			printf("recv_frame.pcket.data = %s\n", recv_frame.packet.data);
			//printf("receive frame_kind = %d, frame_ID = %d\n", recv_frame.frame_kind, frame_id);
			printf("recv_result == %d\n", recv_result);
			/*
			if (recv_result == -1){
				printf("recv_result was -1\n");
				sendto(sockfd, &ackframe, sizeof(ackframe), MSG_CONFIRM, (struct sockaddr*) &cli, len);
			}
			*/
			//printf("strcmp == %d\ncount == %d\n", strcmp(recv_frame.packet.data, "allo"), count);
			if(strcmp(recv_frame.packet.data, "allo") == 0 && recv_frame.frame_kind == 1 && recv_frame.sq_no == frame_id){
				//printf("in strcmp 1\n");
				sendto(sockfd, &ackframe, sizeof(ackframe), MSG_CONFIRM, (struct sockaddr*) &cli, len);
				count++;
				break;
			}
			if(recv_result > 0 && recv_frame.frame_kind == 1 && recv_frame.sq_no == frame_id){
				fwrite(recv_frame.packet.data, 1, recv_result, fp);
				//ackframe.sq_no = recv_frame.sq_no + 1;
				//printf("sending frame_kind = %d, sq_no = %d\n", ackframe.frame_kind, ackframe.sq_no);
				sendto(sockfd, &ackframe, sizeof(ackframe), MSG_CONFIRM, (struct sockaddr*) &cli, len);
				break;
			}else{
				//printf("sending frame_kind = %d, sq_no = %d\n", ackframe.frame_kind, ackframe.sq_no);
				sendto(sockfd, &ackframe, sizeof(ackframe), MSG_CONFIRM, (struct sockaddr*) &cli, len);
				//printf("Frame %d time expired\n", frame_id);
			}
		}
		if(count > 0){
			//printf("in count > 0\n");
			break;
		}
		frame_id++;
	}
	close(sockfd);
}

/*
This is a function that implements the client side of the Stop and Wait protocol of UDP.
It will send the file to the port using a frame. This frame has a header that makes the
server and client be on the same page using ackknowledgements allowing for no data loss to 
occur under unreliable connections.
*/
void stopandwait_client(char* host, long port, FILE* fp) {
	int sockfd, b, data;
	char strPort[10];
	b = sprintf(strPort, "%ld", port);
	struct timeval tv;
	Frame frame;
	Frame recv_frame;
	int recv_result;
	struct sockaddr_in servaddr, cli; //cli
	struct addrinfo hints;
	struct addrinfo *res;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = IPPROTO_TCP;
	void* raw_addr;
	char buff_addr[4096];
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
	
	tv.tv_sec = 0;
	tv.tv_usec = 100000;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(struct timeval));
	int frame_id = 0;
	frame.frame_kind = 1;
	frame.sq_no = frame_id;
	memset(frame.packet.data, 0x00, MAX);
	char end[MAX] = "allo";
	while((data = fread(&frame.packet.data, 1, MAX, fp)) > 0) {
		while(1) {
			frame.sq_no = frame_id;
			printf("sending data = %s\n", frame.packet.data);
			//printf("sending sq_no = %d, frame kind = %d\n", frame.sq_no, frame.frame_kind);
			//memcpy(frame.packet.data, end, sizeof(end));
			sendto(sockfd, &frame, sizeof(frame), MSG_CONFIRM, (struct sockaddr *) &servaddr, sizeof(servaddr));
			recv_result = recvfrom(sockfd, &recv_frame, sizeof(recv_frame), MSG_WAITALL, (struct sockaddr*) &cli, (socklen_t *) sizeof(cli));
			//printf("recv_result == %d\n", recv_result);
			//printf("receiving = sq_no = %d, frame_kind = %d\n", recv_frame.sq_no, recv_frame.frame_kind);
			//printf("recv_frame.packet.data = %s\n", recv_frame.packet.data);
			//printf("current frame_id = %d\n", frame_id);
			/*
			if (recv_result == -1) {
				printf("recv_result was -1\n");
			}
			*/
			recv_result++;
			if (recv_frame.sq_no == frame_id + 1 && recv_frame.frame_kind == 0){ //recv_result > 0
				//printf("breaking frame %d\n", frame_id);
				break; // ack received
			} else {
				//printf("Frame %d time expired\n", frame_id);
			}
		}
		memset(frame.packet.data, 0x00, MAX);
		frame_id++;
	}
	int count = 0;
	memcpy(frame.packet.data, end, sizeof(end));
	while (count == 0){
		frame.sq_no = frame_id;
		//printf("sending data = %s\n", frame.packet.data);
		//printf("sending sq_no = %d, frame kind = %d\n", frame.sq_no, frame.frame_kind);
		sendto(sockfd, &frame, sizeof(frame), MSG_CONFIRM, (struct sockaddr *) &servaddr, sizeof(servaddr));
		recv_result = recvfrom(sockfd, &recv_frame, sizeof(recv_frame), MSG_WAITALL, (struct sockaddr*) &cli, (socklen_t *) sizeof(cli));
		//printf("recv_result == %d\n", recv_result);
		//printf("receiving = sq_no = %d, frame_kind = %d\n", recv_frame.sq_no, recv_frame.frame_kind);
		//printf("recv_frame.packet.data = %s\n", recv_frame.packet.data);
		//printf("current frame_id = %d\n", frame_id);
		if (recv_frame.sq_no == frame_id + 1 && recv_frame.frame_kind == 0){ //recv_result > 0
			//printf("breaking frame %d\n", frame_id);
			break; // ack received
		} else {
			//printf("Frame %d time expired\n", frame_id);
		}
	}
	//memcpy(frame.packet.data, end, sizeof(end));
	//sendto(sockfd, &frame, sizeof(frame), MSG_CONFIRM, (struct sockaddr *) &servaddr, sizeof(servaddr));
	/*
	while(1){
		printf("sending %s\n", frame.packet.data);
		sendto(sockfd, &frame, sizeof(frame), MSG_CONFIRM, (struct sockaddr *) &servaddr, sizeof(servaddr));
	}
	*/
	sendto(sockfd, "", 0, MSG_CONFIRM, (struct sockaddr *) &servaddr, sizeof(servaddr));
	close(sockfd);
}

