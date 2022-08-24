/*
* Code modified from https://beej.us/guide/bgnet/examples/listener.c.
* https://pubs.opengroup.org/onlinepubs/9699919799/functions/getdelim.html
**/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "port_util.h"

char* thisName = "A";
char* thisPort = PORT_ServerA;
char* thisFile = BLOCK1;

// Set up UDP listening
int main() {
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	struct sockaddr_storage their_addr;
	char buf[MAXBUFLEN];
	socklen_t addr_len;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6; // set to AF_INET to use IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, thisPort, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("listener: bind");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
		return 2;
	}

	freeaddrinfo(servinfo);

	printf("The Server%s is up and running using UDP on port %s.\n", thisName, thisPort);

    while (1) {
        addr_len = sizeof their_addr;
        // blocking recvfrom call
        if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
            (struct sockaddr *)&their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }
        printf("The Server%s received a request from the Main Server.\n", thisName);
        buf[numbytes] = '\0';
        // printf("listener: packet contains \"%s\"\n", buf);

		// deal with request
		char buf_copy[MAX_BYTES];
		strcpy(buf_copy, buf);
		char* token = strtok(buf, ",");
		int req_type = atoi(token);
		char userName[128];

		if (req_type==100) { // check logs
			token = strtok(NULL, ",");
			strcpy(userName, token);
			int flag = 0, balance = 0, latestID = -1;
			scanLogs(thisFile, userName, &flag, &balance, &latestID);
			char content[MAX_BYTES];
			sprintf(content, "%d,%d,%d", flag, balance, latestID);
			// send back response
			if (sendtoPort(PORT_UDP, content)!=0) {
				fprintf(stderr, "Server%s cannot send response back.", thisName);
			}
			printf("The Server%s finished sending the response to the Main Server.\n", thisName);
		} else if (req_type==101) { // append transaction
			int id, amount;
			char sender[MAXBUFLEN], receiver[MAXBUFLEN];
			sscanf(buf_copy, "%d,%d,%[^,],%[^,],%d", &req_type, &id, sender, receiver, &amount);
			char newLog[MAX_BYTES];
			sprintf(newLog, "%d %s %s %d", id, sender, receiver, amount);
			// printf("DEBUG New log is: %s\n", newLog);
			// append to block file
			int status = 200;
			if (appendLog(thisFile, newLog)!=0) {
				fprintf(stderr, "Fail to add transcation to %s.", thisFile);
				status = 400;
			}
			char res[MAX_BYTES];
			sprintf(res, "%d", status);
			if (sendtoPort(PORT_UDP, res)!=0) {
				fprintf(stderr, "Server%s cannot send response back.", thisName);
			} else {
				printf("The Server%s finished sending the response to the Main Server.\n", thisName);
			}
		} else if (req_type==102) { // fetch all logs
			char logs[MAX_BYTES], res[MAX_BYTES];
			memset(logs, 0, MAX_BYTES);
			inflateLogs(thisFile, logs);
			// printf("DEBUG: logs are: %s\n", logs);
			sprintf(res, "200,%s", logs);
			if (sendtoPort(PORT_UDP, res)!=0) {
				fprintf(stderr, "Server%s cannot send response back.", thisName);
			} else {
				printf("The Server%s finished sending the response to the Main Server.\n", thisName);
			}
		}
    }
    close(sockfd);
	return 0;
}


