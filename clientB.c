/*
Code modified from 6.2 A Simple Stream Client, Chapter 6 Client-Server Background of Beej's Guide to Network Programming. (https://beej.us/guide/bgnet/examples/client.c)
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "port_util.h"

#define MAXDATASIZE 1000

char* thisName = "B";
char* thisPort = PORT_ClientB;


int main (int argc, char* argv[]) {

	if (argc < 2) {
	    fprintf(stderr,"usage: ./client%s username\n", thisName);
	    exit(1);
	}

    printf("The client %s is up and running.\n", thisName);

	int sockfd, numbytes;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(HOST, thisPort, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			perror("client: connect");
			close(sockfd);
			continue;
		}
		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	// printf("DEBUG client: connecting to %s\n", s); 

	freeaddrinfo(servinfo); // all done with this structure

    // connection is up now
    char req_content[MAXDATASIZE];
    if (argc == 2) { 
        if (strcmp(argv[1], "TXLIST")==0) { // TXLIST
            char userName[128];
            sprintf(userName, "client%s", thisName);
            sprintf(req_content, "%d,%s", 102, userName);
            int bytes_sent;
            if ((bytes_sent = send(sockfd, req_content, sizeof req_content, 0)) == -1) {
                perror("send TXLIST");
                exit(1);
            }
            printf("%s sent a sorted list request to the main server.\n", userName);
        } else { // Check Wallet
            // send request to main server
            char userName[MAX_BYTES];
            strcpy(userName, argv[1]);
            sprintf(req_content, "%d,%s", 100, userName);
            int bytes_sent;
            if ((bytes_sent = send(sockfd, req_content, sizeof req_content, 0)) == -1) {
                perror("send CHECK Wallet");
                exit(1);
            }
            printf("%s sent a balance enquiry request to the main server.\n", userName);
            
            if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
                perror("recv");
                exit(1);
            }
            buf[numbytes] = '\0';
            int status, balance;
            sscanf(buf, "%d,%d", &status, &balance);
            if (status==200) {
                printf("The current balance of %s is : %d alicoins.\n", userName, balance);
            } else if (status==400) {
                printf("Unable to proceed with the request as %s is not part of the network\n", userName);
            }
            // printf("client: received '%s'\n",buf);
        }
    } else if (argc == 4) { // TXCOINS
        // TODO: input checking
        char sender[MAXDATASIZE], receiver[MAXDATASIZE];
        int amount = 0;
        strcpy(sender, argv[1]);
        strcpy(receiver, argv[2]);
        amount = atoi(argv[3]);
        sprintf(req_content,"%d,%s,%s,%d",101,sender,receiver,amount);

        int bytes_sent;
        if ((bytes_sent = send(sockfd, req_content, sizeof req_content, 0)) == -1) {
            perror("send TXCOINS");
            exit(1);
        }
        printf("%s has requested to transfer %d coins to %s.\n", sender, amount, receiver);

        // printf("Req content is %s\n", req_content); // DEBUG


        if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
            perror("recv");
            exit(1);
        }
        buf[numbytes] = '\0';
        int status = 0, senderBalance = 0;
        sscanf(buf, "%d,%d", &status, &senderBalance);
        if (status==200) {
            printf("%s successfully transferred %d alicoins to %s.\n",sender, amount, receiver);
            printf("The current balance of %s is : %d alicoins.\n", sender, senderBalance);
        } else if (status==401) { // sender not in network
            printf("Unable to proceed with the transaction as %s is not part of the network.\n", sender);
        } else if (status==402) { // recevier not in network
            printf("Unable to proceed with the transaction as %s is not part of the network.\n", receiver);
        } else if (status==403) { // Both not in network
            printf("Unable to proceed with the transaction as %s and %s are not part of the network.\n", sender, receiver);
        } else if (status==404) { // no enough balance
            printf("%s was unable to transfer %d alicoins to %s because of insufficient balance.\n", sender, amount, receiver);
            printf("The current balance of %s is : %d alicoins.\n", sender, senderBalance);
        }
    }
    
	close(sockfd);

	return 0;
}