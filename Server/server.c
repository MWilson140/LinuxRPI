#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include "stream.h"     /* MAX_BLOCK_SIZE, readn(), writen() */

#define   SERV_TCP_PORT   40147   /* default server listening port */

ssize_t readLine(int fd, char *buf, size_t maxlen)
{
    ssize_t n, rc;
    char c;
    for (n = 0; n < maxlen - 1; n++) {
        rc = read(fd, &c, 1);
        if (rc == 1) {
            buf[n] = c;
            if (c == '\n')
                break;
        } else if (rc == 0) {
            break; // EOF
        } else {
            if (errno == EINTR)
                continue;
            return -1; // Error
        }
    }
    buf[n] = '\0'; // <-- Corrected from buf[n+1]
    return n;
}


void claim_children() {
    pid_t pid = 1;
    
    while (pid > 0) { /* claim as many zombies as we can */
        pid = waitpid(0, (int *)0, WNOHANG); 
    } 
}


void daemon_init(void) {       
    pid_t   pid;
    struct sigaction act;

    if ((pid = fork()) < 0) {
        perror("fork"); exit(1); 
    } else if (pid > 0) {
        printf("Server PID: %d\n", pid);
        exit(0);                  /* parent exits */
    }

    /* child continues */
    setsid();                      /* become session leader */
    chdir("/");                    /* change working directory */
    umask(0);                      /* clear file mode creation mask */

    /* catch SIGCHLD to remove zombies from system */
    act.sa_handler = claim_children; /* use reliable signal */
    sigemptyset(&act.sa_mask);       /* not to block other signals */
    act.sa_flags = SA_NOCLDSTOP;     /* not catch stopped children */
    sigaction(SIGCHLD, &act, 0);
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <arpa/inet.h>



typedef struct {
    char username[MAX_BLOCK_SIZE];
    char password[MAX_BLOCK_SIZE];
} User;

void serve_a_client(int sd) {
    User users[] = {
        {"test", "test"},
        {"user1", "password1"},
        {"user2", "password2"},
        {"user3", "password3"},
        {"user4", "password4"},
        {"user5", "password5"}
    };
    int num_users = sizeof(users) / sizeof(users[0]);
    int loggedin = 0;
    char user_input[MAX_BLOCK_SIZE], pass_input[MAX_BLOCK_SIZE], msg[MAX_BLOCK_SIZE];

    while (1) {
        memset(user_input, 0, sizeof(user_input));
        memset(pass_input, 0, sizeof(pass_input));

        // Prompt for username
        write(sd, "Enter a Username:\n", 18);
		int len = readLine(sd, msg, MAX_BLOCK_SIZE);
        if (len <= 0) break;
		strncpy(user_input, msg, MAX_BLOCK_SIZE);
		user_input[strcspn(user_input, "\r\n")] = 0;


        if (strcmp(user_input, "exit") == 0) break;

        // Prompt for password
        write(sd, "Enter a password:\n", 19);
		int len = readLine(sd, msg, MAX_BLOCK_SIZE);
		if (len <= 0) break;
		strncpy(pass_input, msg, MAX_BLOCK_SIZE);
		pass_input[strcspn(pass_input, "\r\n")] = 0;

        // Check credentials
        loggedin = 0;
        for (int i = 0; i < num_users; i++) {
            if (strcmp(user_input, users[i].username) == 0 && strcmp(pass_input, users[i].password) == 0) {
                loggedin = 1;
                break;
            }
        }

        if (loggedin) {
            write(sd, "Logged in successfully.\n", 25);
        } else {
            write(sd, "Invalid credentials.\n", 22);
            continue;
        }

        // Command loop
        while (loggedin) {
            write(sd, "Enter a command (or 'exit'):\n", 30);
            memset(msg, 0, sizeof(msg));
			int len = readLine(sd, msg, MAX_BLOCK_SIZE);
            if (len <= 0) break;
            msg[strcspn(msg, "\r\n")] = 0;

            if (strcmp(msg, "exit") == 0) {
                write(sd, "Logging out.\n", 13);
                loggedin = 0;
                break;
            }

            // Run the command using pipe
            int p[2];
            if (pipe(p) < 0) {
                perror("pipe");
                write(sd, "Error creating pipe.\n", 22);
                continue;
            }

            pid_t pid = fork();
            if (pid == 0) {
                dup2(p[1], STDOUT_FILENO);
                dup2(p[1], STDERR_FILENO);
                close(p[0]); close(p[1]);
                execlp(msg, msg, (char *)NULL);
                perror("execlp failed");
                exit(1);
            } else {
                close(p[1]);
                waitpid(pid, NULL, 0);

                char buffer[1024];
                ssize_t bytes;
                while ((bytes = read(p[0], buffer, sizeof(buffer)-1)) > 0) {
                    buffer[bytes] = '\0';
                    write(sd, buffer, bytes);
                }
                close(p[0]);
            }
        }
    }

    close(sd);
}
	

int main(int argc, char *argv[]) {
    int sd, nsd;
    pid_t pid;
    unsigned short port;
    socklen_t cli_addrlen;  
    struct sockaddr_in ser_addr, cli_addr; 

    if (argc == 1) {
        port = SERV_TCP_PORT;
    } else if (argc == 2) {
        int n = atoi(argv[1]);   
        if (n >= 1024 && n < 65536) 
            port = n;
        else {
            printf("Error: port number must be between 1024 and 65535\n");
            exit(1);
        }
    } else {
        printf("Usage: %s [server listening port]\n", argv[0]);     
        exit(1);
    }

    daemon_init(); 

    if ((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("server:socket"); exit(1);
    } 

    bzero((char *)&ser_addr, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_port = htons(port);
    ser_addr.sin_addr.s_addr = htonl(INADDR_ANY); 

    if (bind(sd, (struct sockaddr *) &ser_addr, sizeof(ser_addr)) < 0){
        perror("server bind"); exit(1);
    }

    listen(sd, 5);

    while (1) {
        cli_addrlen = sizeof(cli_addr);
        nsd = accept(sd, (struct sockaddr *) &cli_addr, &cli_addrlen);
        if (nsd < 0) {
            if (errno == EINTR) continue;
            perror("server:accept"); exit(1);
        }

        if ((pid = fork()) < 0) {
            perror("fork"); exit(1);
        } else if (pid > 0) { 
            close(nsd);
            continue; /* parent to wait for next client */
        }
        serve_a_client(nsd);
        close(nsd);
        exit(0);
    }
}
