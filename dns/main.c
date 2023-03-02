#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>

/*
  Use the `getaddrinfo` and `inet_ntop` functions to convert a string host and
  integer port into a string dotted ip address and port.
 */
int main(int argc, char* argv[]) {
  if (argc != 3) {
    printf("Invalid arguments - %s <host> <port>", argv[0]);
    return -1;
  }
  char* host = argv[1];
  long port = atoi(argv[2]);

  /*
    STUDENT CODE HERE
   */


struct addrinfo hints;
struct addrinfo *res, *rp;
int a;

a = port; // Currently using argv[2] directly because it is a str and getaddrinfo requires a str. 
// Maybe use htons(port) == he used this in lecture for generic thing but it didnt work for me right away
memset(&hints, 0, sizeof(hints));
hints.ai_family = PF_UNSPEC;
hints.ai_socktype = SOCK_STREAM;
hints.ai_flags = AI_PASSIVE;
hints.ai_protocol = IPPROTO_TCP;

a = getaddrinfo(host, argv[2], &hints, &res);
if (a != 0) {
               fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(a));
               exit(EXIT_FAILURE);
            }

char buffer[4096];
void* raw_addr;
for (rp = res; rp != NULL; rp = rp->ai_next) {
	if (rp->ai_family == AF_INET) { // Address is IPv4
  		struct sockaddr_in* tmp = (struct sockaddr_in*)rp->ai_addr; // Cast addr into AF_INET container
  		raw_addr = &(tmp->sin_addr); // Extract the address from the container
		inet_ntop(AF_INET, raw_addr, buffer, 4096);
		if (raw_addr != NULL) {
			printf("IPv4 %s\n", buffer);
		}
		else { printf("IPv4 Error"); }
	}
	else { // Address is IPv6
  		struct sockaddr_in6* tmp = (struct sockaddr_in6*)rp->ai_addr; // Cast addr into AF_INET6 container
  		raw_addr = &(tmp->sin6_addr); // Extract the address from the container
		inet_ntop(AF_INET6, raw_addr, buffer, 4096);
		if (raw_addr != NULL) {
			printf("IPv6 %s\n", buffer);
		}
		else { printf("IPv6 Error"); }
	}
}
return 0;
}
