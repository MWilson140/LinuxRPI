/*
 *  stream.c  -	For ICT374 Topic 8
 *              Hong Xie
 *              Last modified: 16/10/2020
 *	 	routines for stream read and write. 
 */

#include  <unistd.h>
#include  <sys/types.h>
#include  <netinet/in.h> /* struct sockaddr_in, htons(), htonl(), */
#include  "stream.h"
#include <stdio.h>

int readn(int fd, char *buf, int bufsize)
{
    short data_size;    /* sizeof (short) must be 2 */ 
    int n, nr, len;

    /* check buffer size len */
    if (bufsize < MAX_BLOCK_SIZE)
         return (-3);     /* buffer too small */
    if (read(fd, (char *) &data_size, 1) != 1) return (-1);
    if (read(fd, (char *) (&data_size)+1, 1) != 1) return (-1);
    len = (int) ntohs(data_size);  /* convert to host byte order */ 
    for (n=0; n < len; n += nr) {
        if ((nr = read(fd, buf+n, len-n)) <= 0) 
            return (nr);       /* error in reading */
    }
    return (len); 
}

int writen(int fd, char *buf, int nbytes)
{
    short data_size = nbytes;     /* short must be two bytes long */
    int n, nw;
    if (nbytes > MAX_BLOCK_SIZE) 
         return (-3);    /* too many bytes to send in one go */ 
	//printf("writing data size %d\n", data_size);
    /* send the data size */
    data_size = htons(data_size); 
    if (write(fd, (char *) &data_size, 1) != 1) return (-1);      
    if (write(fd, (char *) (&data_size)+1, 1) != 1) return (-1);       

    /* send nbytes */
    for (n=0; n<nbytes; n += nw) {
         if ((nw = write(fd, buf+n, nbytes-n)) <= 0)  
             return (nw);    /* write error */
    } 
    return (n);
}



void sendLarge(int fd, const char *msg) 
{
    unsigned char messageno = 1; // Start message number at 1
    unsigned char totalmsg;
    int nw;
    char buf[MAX_BLOCK_SIZE];
    short header;
    
    size_t message_length = strlen(msg);
    size_t max_payload_size = MAX_BLOCK_SIZE - sizeof(short); 
    totalmsg = (message_length + max_payload_size - 1) / max_payload_size; 

    for (size_t offset = 0; offset < message_length; offset += max_payload_size, messageno++) {
        memset(buf, '\0', sizeof(buf)); // Clear buffer for each packet
        
        // Prepare header with messageno and totalmsg
        header = htons((messageno << 8) | totalmsg);
        memcpy(buf, &header, sizeof(header));
        
        // Copy part of the message into the buffer, up to the max payload size
        size_t chunk_size = (message_length - offset > max_payload_size) ? max_payload_size : (message_length - offset);
        memcpy(buf + sizeof(header), msg + offset, chunk_size);

        // Write the packet
        nw = writen(fd, buf, sizeof(header) + chunk_size);
        if (nw <= 0) {
            perror("sendLarge: write error");
            exit(1);
        }
    }
}


void readLarge(int sd, char msg[]) // Read multiple packets and assemble the message
{
    unsigned char messageno, totalmsg;
    char response[MAX_BLOCK_SIZE];
    int nr;
    char buf[MAX_BLOCK_SIZE];
    short header;
    int bytes_read = 0;
    msg[0] = '\0'; 
    do {
        if ((nr = readn(sd, buf, sizeof(buf))) <= 0)  // read
        {
            printf("client: receive error\n");
            exit(1); 
        }
        
        buf[nr] = '\0';
        
        if (nr < sizeof(short)) {
            printf("client: received message too short\n");
            exit(1); 
        }

        memcpy(&header, buf, sizeof(header)); 
        header = ntohs(header); 
        messageno = (header >> 8) & 0xFF;
        totalmsg = header & 0xFF;

        strncat(msg, buf + sizeof(short), nr - sizeof(short));

        bytes_read += nr - sizeof(short);
    } while (messageno < totalmsg);
    msg[bytes_read] = '\0';
}