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
#define MAX 128 // Max allowed for each packetSegment
#define SA struct sockaddr
/*
 *  Here is the starting point for your netster part.4 definitions. Add the 
 *  appropriate comment header as defined in the code formatting guidelines
 */

/* Add function definitions */

struct Packet{
	int type; // 1 = SEND, 2 = ACK, 3 = TERMINATE
	int seq_no;
	char data[MAX];
};

struct ACKPacket{
	int type; // 1 = SEND, 2 = ACK, 3 = TERMINATE
	int ack_no;
};

/* */
void gbn_server(char* iface, long port, FILE* fp) {
	int sockfd, b, len, recv_result;
	char strPort[10];
	b = sprintf(strPort, "%ld", port);
	struct timeval tv;
	struct sockaddr_in servaddr, cli;
	len = sizeof(cli);
	
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
	tv.tv_sec = 0;
	tv.tv_usec = 1000000;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(struct timeval));

	if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) { 
		printf("socket bind failed...\n");
		exit(0);
	}

	struct Packet dataPacket;
	struct ACKPacket ack;
	ack.type = 2;
	memset(dataPacket.data, 0, sizeof(dataPacket.data));
	int currentSeq = 0; // Current request sequence number
	
	while(1) {
		memset(dataPacket.data, 0, sizeof(dataPacket.data));
		recv_result = recvfrom(sockfd, &dataPacket, sizeof(dataPacket), 0, (struct sockaddr*) &cli, (socklen_t *)&len);
		//printf("received seqNO %d :::::: of %d bytes of data being : %s ::::: TYPE = %d\n", dataPacket.seq_no, recv_result, dataPacket.data, dataPacket.type);
		if(dataPacket.type == 3 && dataPacket.seq_no == currentSeq){
			currentSeq++;
			ack.ack_no = currentSeq;
			ack.type = 3;
			//printf("Sending ack seqNo: %d :::::: TYPE = %d\n", ack.ack_no, ack.type);
			sendto(sockfd, &ack, sizeof(ack), 0, (struct sockaddr*) &cli, len);
			break;
		}
		else if (dataPacket.seq_no == currentSeq && dataPacket.type == 1 && recv_result > 0){
			fwrite(dataPacket.data, 1, recv_result, fp);
			currentSeq++;
			ack.ack_no = currentSeq;
			//printf("Sending ack seqNo: %d :::::: TYPE = %d\n", ack.ack_no, ack.type);
			sendto(sockfd, &ack, sizeof(ack), 0, (struct sockaddr*) &cli, len);
		} else {
			//printf("Sending TIMED OUT ack seqNo: %d :::::: TYPE = %d\n", ack.ack_no, ack.type);
			sendto(sockfd, &ack, sizeof(ack), 0, (struct sockaddr*) &cli, len);
		}
	}
	close(sockfd);
}

void gbn_client(char* host, long port, FILE* fp) {
	int sockfd, b, data;
	int recv_result = 1;
	recv_result++; // To get rid of error of unused variable
	char strPort[10];
	b = sprintf(strPort, "%ld", port);
	struct timeval tv;
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
	tv.tv_usec = 1000000;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(struct timeval));

	struct Packet sendingPacket;
	struct ACKPacket ack;
	sendingPacket.type = 1;
	sendingPacket.seq_no = 0;
	
	int chunkAmount = 5;
	int base = 0;
	int seqNumber = 0;
	char currentBuffer[MAX*chunkAmount];
	char lastBuffer[MAX*chunkAmount];
	char seqBuffer[MAX];
	int difference = chunkAmount;
	int lastBufferEndPoint = chunkAmount;
	int terminateSeqNo = -1;
	memset(currentBuffer,0,sizeof(currentBuffer));
	data = fread(lastBuffer, 1, MAX*chunkAmount, fp);
	while((data = fread(currentBuffer, 1, MAX*chunkAmount, fp)) > 0) {
		//printf("data : %d ::::::: DIFFERENCE %d\n", data, difference);
		seqNumber = 0;
		while(difference - seqNumber > 0){
			memset(seqBuffer,0,sizeof(seqBuffer));
			memcpy(seqBuffer, &lastBuffer[MAX*chunkAmount-((difference-seqNumber)*MAX)], MAX);
			memset(sendingPacket.data,0,sizeof(sendingPacket.data));
			memcpy(sendingPacket.data, seqBuffer, MAX);
			sendingPacket.type = 1;
			sendingPacket.seq_no = base + chunkAmount - difference + seqNumber;
			//printf("In difference :: seqNumber %d : Sending ackSeqNO %d ::::::: data : %s ::::::: TYPE %d\n\n", seqNumber, sendingPacket.seq_no, sendingPacket.data, sendingPacket.type);
			sendto(sockfd, &sendingPacket, sizeof(sendingPacket), MSG_CONFIRM, (struct sockaddr *) &servaddr, sizeof(servaddr));
			seqNumber++;
		}
		while(!(seqNumber-difference >= chunkAmount) && (data >= (seqNumber-difference+1)*MAX)){
			memset(seqBuffer,0,sizeof(seqBuffer));
			memcpy(seqBuffer, &currentBuffer[0+((seqNumber-difference)*MAX)], MAX);
			memset(sendingPacket.data,0,sizeof(sendingPacket.data));
			memcpy(sendingPacket.data, seqBuffer, MAX);
			sendingPacket.type = 1;
			sendingPacket.seq_no = base + chunkAmount - difference + seqNumber;
			seqNumber++;
			//printf("In data >= : Sending ackSeqNO %d ::::::: data : %s ::::::: TYPE %d\n\n", sendingPacket.seq_no, sendingPacket.data, sendingPacket.type);
			sendto(sockfd, &sendingPacket, sizeof(sendingPacket), MSG_CONFIRM, (struct sockaddr *) &servaddr, sizeof(servaddr));
		}
		if (!(seqNumber-difference >= chunkAmount) && (data < (seqNumber-difference+1)*MAX)) {
			memset(seqBuffer,0,sizeof(seqBuffer));
			memcpy(seqBuffer, &currentBuffer[0+((seqNumber-difference)*MAX)], MAX);
			memset(sendingPacket.data,0,sizeof(sendingPacket.data));
			memcpy(sendingPacket.data, seqBuffer, MAX);
			sendingPacket.type = 1;
			sendingPacket.seq_no = base + chunkAmount - difference + seqNumber;
			//printf("In data < : Sending ackSeqNO %d ::::::: data : %s ::::::::: TYPE %d\n\n", sendingPacket.seq_no, sendingPacket.data, sendingPacket.type);
			sendto(sockfd, &sendingPacket, sizeof(sendingPacket), MSG_CONFIRM, (struct sockaddr *) &servaddr, sizeof(servaddr));
			// Sending termination packet with correct seqNumber
			seqNumber++;
			memset(sendingPacket.data,0,sizeof(sendingPacket.data));
			sendingPacket.seq_no = base + chunkAmount - difference + seqNumber; // Termination seqNumber
			terminateSeqNo = base + chunkAmount - difference + seqNumber;
			sendingPacket.type = 3;
			//printf("In data < TERMINATE : Sending ackSeqNO %d ::::::: data : %s ::::::::: TYPE %d\n", sendingPacket.seq_no, sendingPacket.data, sendingPacket.type);
			sendto(sockfd, &sendingPacket, sizeof(sendingPacket), MSG_CONFIRM, (struct sockaddr *) &servaddr, sizeof(servaddr));
		}
		
		while(1){
			recv_result = recvfrom(sockfd, &ack, sizeof(ack), MSG_WAITALL, (struct sockaddr*) &cli, (socklen_t *) sizeof(cli));
			//printf("received ackSeqNo %d :::::::: ackType %d\n", ack.ack_no, ack.type);
			//printf("terminateSeqNo %d\n", terminateSeqNo);
			if (terminateSeqNo+1 == ack.ack_no && ack.type == 3) {
				break;
			}
			else if (ack.ack_no >= lastBufferEndPoint) {
				break;
			} 
			
			
		
			difference = base + chunkAmount - ack.ack_no; // How much I need to go back in lastBuffer by segment Count.
			seqNumber = 0;
			while(difference - seqNumber > 0){
				memset(seqBuffer,0,sizeof(seqBuffer));
				memcpy(seqBuffer, &lastBuffer[MAX*chunkAmount-((difference-seqNumber)*MAX)], MAX);
				memset(sendingPacket.data,0,sizeof(sendingPacket.data));
				memcpy(sendingPacket.data, seqBuffer, MAX);
				sendingPacket.type = 1;
				sendingPacket.seq_no = base + chunkAmount - difference + seqNumber;
				//printf("In ELSE IF difference :: seqNumber %d : Sending ackSeqNO %d ::::::: data : %s ::::::: TYPE %d\n\n", seqNumber, sendingPacket.seq_no, sendingPacket.data, sendingPacket.type);
				sendto(sockfd, &sendingPacket, sizeof(sendingPacket), MSG_CONFIRM, (struct sockaddr *) &servaddr, sizeof(servaddr));
				seqNumber++;
			}
			
			while(!((seqNumber-difference) >= chunkAmount)){
				memset(seqBuffer,0,sizeof(seqBuffer));
				memcpy(seqBuffer, &currentBuffer[0+((seqNumber-difference)*MAX)], MAX);
				memset(sendingPacket.data,0,sizeof(sendingPacket.data));
				memcpy(sendingPacket.data, seqBuffer, MAX);
				sendingPacket.type = 1;
				sendingPacket.seq_no = base + chunkAmount - difference + seqNumber;
				seqNumber++;
				//printf("In ELSE IF seqNum >= chunkAmount : Sending ackSeqNO %d ::::::: data : %s ::::::: TYPE %d\n\n", sendingPacket.seq_no, sendingPacket.data, sendingPacket.type);
				sendto(sockfd, &sendingPacket, sizeof(sendingPacket), MSG_CONFIRM, (struct sockaddr *) &servaddr, sizeof(servaddr));
			}
			
		}
		memcpy(lastBuffer, currentBuffer, MAX*chunkAmount);
		memset(currentBuffer,0,sizeof(currentBuffer));
		lastBufferEndPoint = lastBufferEndPoint + chunkAmount;
		difference = lastBufferEndPoint - ack.ack_no; // How much I need to go back in lastBuffer by segment Count.
		base = lastBufferEndPoint - chunkAmount;			
	}
	
	while(1) {
		recv_result = recvfrom(sockfd, &ack, sizeof(ack), MSG_WAITALL, (struct sockaddr*) &cli, (socklen_t *) sizeof(cli));
		//printf("received ackSeqNo %d :::::::: ackType %d\n", ack.ack_no, ack.type);
		//printf("terminateSeqNo %d\n", terminateSeqNo);
		if (ack.type == 3 && terminateSeqNo+1 == ack.ack_no) {
			break;
		}
		if (ack.ack_no >= lastBufferEndPoint) {
			break;
		} 
		
		difference = base + chunkAmount - ack.ack_no; // How much I need to go back in lastBuffer by segment Count.
		//base = ack.ack_no;
		seqNumber = 0;
		while(difference - seqNumber > 0){
			memset(seqBuffer,0,sizeof(seqBuffer));
			memcpy(seqBuffer, &lastBuffer[MAX*chunkAmount-((difference-seqNumber)*MAX)], MAX);
			memset(sendingPacket.data,0,sizeof(sendingPacket.data));
			memcpy(sendingPacket.data, seqBuffer, MAX);
			sendingPacket.type = 1;
			sendingPacket.seq_no = base + chunkAmount - difference + seqNumber;
			//printf("In TERMINATE ELSE IF difference :: seqNumber %d : Sending ackSeqNO %d ::::::: data : %s ::::::: TYPE %d\n\n", seqNumber, sendingPacket.seq_no, sendingPacket.data, sendingPacket.type);
			sendto(sockfd, &sendingPacket, sizeof(sendingPacket), MSG_CONFIRM, (struct sockaddr *) &servaddr, sizeof(servaddr));
			seqNumber++;
		}
		
		memset(sendingPacket.data,0,sizeof(sendingPacket.data));
		sendingPacket.seq_no = base + chunkAmount - difference + seqNumber; // Termination seqNumber
		sendingPacket.type = 3;
		//printf("In LAST TERMINATE : Sending ackSeqNO %d ::::::: data : %s ::::::::: TYPE %d\n", sendingPacket.seq_no, sendingPacket.data, sendingPacket.type);
		sendto(sockfd, &sendingPacket, sizeof(sendingPacket), MSG_CONFIRM, (struct sockaddr *) &servaddr, sizeof(servaddr));
	}
	
	sendto(sockfd, "", 0, MSG_CONFIRM, (struct sockaddr *) &servaddr, sizeof(servaddr));
	close(sockfd);
}

