/* cli6.c - 	For ICT374 Topic 8
*              Hong Xie
*              Last modified: 16/10/2020
*		An improved version of "cli5.c". Since TCP does not preserve the message 
*              boundaries, each message is preceeded by a two byte value which is the
*              length of the message. 
*/

#include  <stdlib.h>
#include  <stdio.h>
#include  <sys/types.h>        
#include  <sys/socket.h>
#include  <netinet/in.h>       /* struct sockaddr_in, htons, htonl */
#include  <netdb.h>            /* struct hostent, gethostbyname() */ 
#include  <string.h>
#include  "stream.h"           /* MAX_BLOCK_SIZE, readn(), writen() */
#include <semaphore.h>
#include <unistd.h>



#define   SERV_TCP_PORT  40147 /* default server listening port */

void getUserInput(char *large)
{
	int nr;
	nr = read(STDIN_FILENO, large, 10000-1);
	if (nr == 0)
	{
		perror("read error");
		exit(0);
	}
	 if (nr > 0) {
        if (large[nr - 1] == '\n') 
		{
            large[nr - 1] = '\0'; 
        } else 
		{
            large[nr] = '\0'; 
        }
    } else 
	{
        large[0] = '\0'; 
    }
		
}
int main(int argc, char *argv[])
{

	int sd, n, nr, nw, i=0;
	char buf[MAX_BLOCK_SIZE], host[60];
	unsigned short port;
	struct sockaddr_in ser_addr; struct hostent *hp;

	if (argc==1) { 
	strcpy(host, "localhost");
	port = SERV_TCP_PORT;
	} else if (argc == 2) { /* use the given host name */ 
	strcpy(host, argv[1]);
	port = SERV_TCP_PORT;
	} else if (argc == 3) { // use given host and port for server
	strcpy(host, argv[1]);
	int n = atoi(argv[2]);
	if (n >= 1024 && n < 65536) 
	  port = n;
	else {
	  printf("Error: server port number must be between 1024 and 65535\n");
	  exit(1);
	}
	} else { 
	printf("Usage: %s [ <server host name> [ <server listening port> ] ]\n", argv[0]); 
	exit(1); 
	}

	/* get host address, & build a server socket address */
	bzero((char *) &ser_addr, sizeof(ser_addr));
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_port = htons(port);
	if ((hp = gethostbyname(host)) == NULL){
	printf("host %s not found\n", host); exit(1);   
	}
	ser_addr.sin_addr.s_addr = * (u_long *) hp->h_addr;

	/* create TCP socket & connect socket to server address */
	sd = socket(PF_INET, SOCK_STREAM, 0);
	if (connect(sd, (struct sockaddr *) &ser_addr, sizeof(ser_addr))<0) { 
	perror("client connect"); exit(1);
	}

	unsigned char messageno;
	unsigned char totalmsg;
	short header = 0;
	char response[MAX_BLOCK_SIZE];
	char large[10000];
	int loggedin = 0;
	
	//while not logged in
	while(1)
	{	
		readLarge(sd, large); //read user
			printf("%s\n", large);
			memset(large, '\0', sizeof(large)); 
			fflush(stdout);
		getUserInput(large);
			if (strcmp(large, "exit") == 0) 
			{
				sendLarge(sd, large);
				sleep(1);
				printf("exiting\n");
				break; // Break out of the login loop
			}
		sendLarge(sd, large);  //write user
			memset(large, '\0', sizeof(large));
		readLarge(sd, large); //read pass
			printf("%s\n", large);
			memset(large, '\0', sizeof(large));
		getUserInput(large);
			if (strcmp(large, "exit") == 0) 
			{
				sendLarge(sd, large);
				sleep(1);
				printf("exiting\n");
				break; // Break out of the login loop
			}
		sendLarge(sd, large); //write pass
			memset(large, '\0', sizeof(large));
		readLarge(sd, large); //read login response
			printf("read response %s.\n", large);
			if (strcmp(large, "loggedin") == 0)
			{
				loggedin = 1;
				printf("i have logged in\n");
			}
			memset(large, '\0', sizeof(large));
			sendLarge(sd, large); //write pass
			memset(large, '\0', sizeof(large));
		while(loggedin)
		{
			fflush(stdout);
			readLarge(sd, large); //read command:
			printf("%s", large);
			memset(large, '\0', sizeof(large));
			getUserInput(large);
			if (strcmp(large, "exit") == 0) 
			{
				sendLarge(sd, large);
				sleep(1);
				printf("exiting\n");
				break; // Break out of the login loop
			}
			sendLarge(sd, large); //send command
			memset(large, '\0', sizeof(large));
			readLarge(sd, large); //read response
			printf("%s", large); //print response
			memset(large, '\0', sizeof(large));
		}
	}
}

	