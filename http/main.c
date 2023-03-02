#include <stdio.h>
#include <string.h>

void send_http(char* host, char* msg, char* resp, size_t len);


/*
  Implement a program that takes a host, verb, and path and
  prints the contents of the response from the request
  represented by that request.
 */
int main(int argc, char* argv[]) {
  if (argc != 4) {
    printf("Invalid arguments - %s <host> <GET|POST> <path>\n", argv[0]);
    return -1;
  }
  char* host = argv[1];
  char* verb = argv[2];
  char* path = argv[3];

  /*
    STUDENT CODE HERE
   */

char message[] = " HTTP/1.1\r\nHost: "; 
strcat(verb, " ");
strcat(verb, path);
strcat(verb, " ");
strcat(verb, message);
strcat(verb, host);
strcat(verb, "\r\n\r\n");
char response[4096];
send_http(host, verb, response, 4096);
printf("%s\n", response);
return 0; 
}
