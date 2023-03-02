#include <stdio.h>
#include <string.h>

int connect_smtp(const char* host, int port);
void send_smtp(int sock, const char* msg, char* resp, size_t len);



/*
  Use the provided 'connect_smtp' and 'send_smtp' functions
  to connect to the "lunar.open.sice.indian.edu" smtp relay
  and send the commands to write emails as described in the
  assignment wiki.
 */
int main(int argc, char* argv[]) {
  if (argc != 3) {
    printf("Invalid arguments - %s <email-to> <email-filepath>", argv[0]);
    return -1;
  }

  char* rcpt = argv[1];
  char* filepath = argv[2];

  /* 
     STUDENT CODE HERE
   */

	FILE *fp;
	char buffer[4096];
	fp = fopen(filepath, "r");
	char fullEmail[4096] = "";
	while(fgets(buffer, 4096, fp)){
		strcat(fullEmail, buffer);
	}
	fclose(fp);
	int socket = connect_smtp("lunar.open.sice.indiana.edu", 25);
	char response[4096];
	char hello[] = "HELO iu.edu\r\n";
	send_smtp(socket, hello, response, 4096);
	printf("%s\n", response);
	char mailFrom[4096] = "MAIL FROM:";
	strcat(mailFrom, rcpt);
	strcat(mailFrom, "\r\n");
	send_smtp(socket, mailFrom, response, 4096);
	printf("%s\n", response);
	char receive[4096] = "RCPT TO:";
	strcat(receive, rcpt);
	strcat(receive, "\r\n");
	send_smtp(socket, receive, response, 4096);
	printf("%s\n", response);
	char data[4096] = "DATA\r\n";
	send_smtp(socket, data, response, 4096);
	printf("%s\n", response);
	strcat(fullEmail, "\r\n.\r\n");
	send_smtp(socket, fullEmail, response, 4096);
	printf("%s\n", response); 

 	return 0;
}
