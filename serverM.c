/*
Code modified from 6.1 A Simple Stream Server, Chapter 6 Client-Server Background of Beej's Guide to Network Programming. https://beej.us/guide/bgnet/examples/server.c
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include "port_util.h"

#define BACK_LOG 10

char *servers[3] = {"A", "B", "C"};
char *serverPorts[3] = {PORT_ServerA, PORT_ServerB, PORT_ServerC};

// Code from https://beej.us/guide/bgnet/examples/server.c
void sigchld_handler (int s) {
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG)>0);
    errno = saved_errno;
}

// Store TXLog list into alichain.txt
int storeTXArray(TXLog* list, int len) {
    char* fileName="alichain.txt";
	FILE *fp;
    fp = fopen(fileName, "w");
    if (fp==NULL) {
		exit(1);
    }
    for (int i=0; i<len; i++) {
        fprintf(fp, "%d %s %s %d\n", list[i].id, list[i].sender, list[i].receiver, list[i].amount);
    }
    fclose(fp);
    return 0;
}

/**
 * @brief Get balance from 3 servers,
 * 
 * Code modified from https://beej.us/guide/bgnet/examples/talker.c
 * 
 * @param serverName 
 * @param userName 
 * @param flag 1 means userName occurs, 0 otherwises
 * @param balance 
 * @param latestID largest id occurs in the server log
 * @return int 0 means successful
 */
int fetchBalance(char* serverName, char* userName, int* flag, int* balance, int* latestID) {
    char destPort[10];

    if (strcmp(serverName, "A")==0) {
        strcpy(destPort, PORT_ServerA);
    } else if (strcmp(serverName, "B")==0) {
        strcpy(destPort, PORT_ServerB);
    } else {
        strcpy(destPort, PORT_ServerC);
    }

    // Send request
    char req_content[MAX_BYTES];
    sprintf(req_content, "%d,%s", 100, userName);
    if (sendtoPort(destPort, req_content)!=0) {
        fprintf(stderr, "ServerM cannot send request from Server%s.\n", serverName);
        exit(1);
    }
    printf("The main server sent a request to server %s.\n", serverName);

    // Listen to response
    char res[MAXBUFLEN];
    if (recvfromPort(PORT_UDP, res)!=0) {
        fprintf(stderr, "ServerM cannot receive response from Server%s.\n", serverName);
        exit(1);
    }
    printf("The main server received transactions from Server %s using UDP over port %s.\n", serverName, destPort);
    // printf("DEBUG: %s\n", res);
    sscanf(res, "%d,%d,%d", flag, balance, latestID);
	return 0;

}

/**
 * @brief Get the Balance of userName from three backend servers
 * 
 * @param userName 
 * @return int -1 means user not exists. postive value means valid balance
 */
int getBalance(char* userName, int* latestID) {
    int totalBalance=1000, flag, curr, status=400, tempID=-1;
    char* destName;
    for (int i=0; i<3; i++) {
        destName = servers[i];
        fetchBalance(destName, userName, &flag, &curr, &tempID);
        if (flag!=0) {
            totalBalance += curr;
            status = 200;
        }
        if (tempID > *latestID) {
            *latestID = tempID;
        }
    }
    if (status==200) {
        return totalBalance;
    } else {
        return -1;
    }
}

/**
 * @brief Send request to backend server and store log string into logs
 * 
 * Code modified from https://beej.us/guide/bgnet/examples/talker.c
 * 
 * @param serverName 
 * @param logs 
 * @return int 
 */
int fetchLogs(char* serverName, char* logs) {
    char destPort[10];

    if (strcmp(serverName, "A")==0) {
        strcpy(destPort, PORT_ServerA);
    } else if (strcmp(serverName, "B")==0) {
        strcpy(destPort, PORT_ServerB);
    } else {
        strcpy(destPort, PORT_ServerC);
    }

    // Send request
    char req_content[MAX_BYTES];
    sprintf(req_content, "%d,TXLIST", 102);
    if (sendtoPort(destPort, req_content)!=0) {
        char errMsg[128];
        sprintf(errMsg, "ServerM cannot send request from Server%s.\n", serverName);
        perror(errMsg);
        exit(1);
    }
    printf("The main server sent a request to server %s.\n", serverName);

    // Listen to response
    char res[MAXBUFLEN];
    if (recvfromPort(PORT_UDP, res)!=0) {
        char errMsg[128];
        sprintf(errMsg, "ServerM cannot receive response from Server%s.\n", serverName);
        perror(errMsg);
        exit(1);
    }
    printf("The main server received transactions from Server %s using UDP over port %s.\n", serverName, destPort);
    strcpy(logs, res+4);
	return 0;
}

void swap(TXLog* a, TXLog* b) {
    TXLog temp = *a;
    *a = *b;
    *b = temp;
}

/**
 * @brief Get the logs from 3 respective backend servers, combine together and sort.
 * 
 * @return int 
 */
int getLogList() {
    char* destName;
    char allLogs[MAX_BYTES];
    memset(allLogs, 0, MAX_BYTES);
    for (int i=0;i<3; i++) {
        char temp[MAX_BYTES];
        memset(temp, 0, MAX_BYTES);
        destName = servers[i];
        if (fetchLogs(destName, temp)!=0) {
            exit(1);
        }
        if (i>0) {
            strcat(allLogs, ",");
        }
        strcat(allLogs, temp);
    }
    // printf("DEBUG all logs are: %s\n", allLogs);
    // parse string into log array
    int len = 0;
    for (int i=0; i<strlen(allLogs); i++) {
        if (allLogs[i]==',') {
            len++;
        }
    }
    len++;
    // printf("DEBUG length of logs: %d\n", len);
    TXLog TX_Array[len];
    memset(TX_Array, 0, len*(sizeof TX_Array[0]));
    int curr = 0;
    char* token = strtok(allLogs, COMMA);
    while(token!=NULL && curr<len) {
        int id, amount;
        char sender[MAXBUFLEN], receiver[MAXBUFLEN];
        sscanf(token, "%d %s %s %d", &id, sender, receiver, &amount);
        TX_Array[curr].id = id;
        TX_Array[curr].amount = amount;
        strcpy(TX_Array[curr].sender, sender);
        strcpy(TX_Array[curr].receiver, receiver);
        curr+=1;
        token = strtok(NULL, COMMA);
    }
    // Bubble sort
    for (int i=0; i<len-1; i++) {
        for (int j=0; j<len-i-1; j++) {
            if (TX_Array[j].id > TX_Array[j+1].id) {
                swap(TX_Array+j, TX_Array+j+1);
            }
        }
    }
    if (storeTXArray(TX_Array, len)!=0) {
        exit(1);
    }
    return 0;
}

/**
 * @brief Build up server
 * 
 * Code modified from https://beej.us/guide/bgnet/examples/server.c.
 * 
 * @param portNum 
 * @param clientName 
 * @return int 
 */
int setUpTCP(char* portNum, char* clientName) {
    int sockfd, new_fd;
    struct addrinfo *servinfo, *p, hints;
    struct sockaddr_storage their_addr;
    socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv; // return value

    // fill in hints
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM; // choose TCP connection
    hints.ai_flags = AI_PASSIVE; // host's IP

    if ((rv = getaddrinfo(NULL, portNum, &hints, &servinfo))!=0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(rv));
        return 1;
    }

    for (p = servinfo; p!=NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))==-1) {
            perror("serverM: socket");
            continue;
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))==-1) {
            perror("setsocketopt");
            exit(1);
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen)==-1) {
            close(sockfd);
            perror("serverM: bind");
            continue;
        }
        break; // bind successful
    }

    freeaddrinfo(servinfo);


    if (p==NULL) {
        fprintf(stderr, "serverM: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACK_LOG)==-1) {
        perror("listen");
        exit(1);
    }
    
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL)==-1) {
        perror("sigaction");
        exit(1);
    }

    if (strcmp(clientName, "A")==0) {
        printf("server: is up and running.\n");
    }

    while(1) {
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }
        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);

        // printf("serverM: got connection from %s\n", s);

        if (!fork()) { // child process
            close(sockfd);

            char buf[MAX_BYTES];
            int bytes_recv;
            if ((bytes_recv = recv(new_fd, buf, MAX_BYTES-1, 0))==-1) {
                perror("recv");
                exit(1);
            }
            buf[bytes_recv] = '\0';

            // split the received string
            char buf_copy[MAX_BYTES];
            strcpy(buf_copy, buf); // copy of buf
            char * token = strtok(buf, ",");
            int req_type = atoi(token); // req type
            char userName[MAX_BYTES];

            // response to send back to client
            char res[MAX_BYTES];
            // Phase 1
            if (req_type==100) { // CHECK WALLET
                token = strtok(NULL, ","); // get userName
                strcpy(userName, token);
        	    printf("The main server received input='%s' from the client using TCP over port %s.\n",token, portNum);

                int latestID = -1;
                int balance = getBalance(userName, &latestID);
                // printf("Balance is: %d, %d\n", balance, latestID);
                // prepare result of CHECK WALLET
                int status = 200;
                if (balance==-1) {
                    status = 400;
                }
                sprintf(res, "%d,%d", status, balance);

                printf("The main server sent the current balance to client %s.\n", clientName);

            } else if (req_type == 101) {  // TXCOINS
                int type, amount;
                char sender[MAX_BYTES], receiver[MAX_BYTES];
                sscanf(buf_copy, "%d,%[^,],%[^,],%d", &type, sender, receiver, &amount);
                printf("The main server received from %s to transfer %d coins to %s using TCP over port %s.\n", sender, amount, receiver, portNum);

                int status = 200, latestID = -1;
                // get Sender's balance
                int senderBalance = getBalance(sender, &latestID);
                // printf("DEBUG: Sender's Balance is: %d, %d\n", senderBalance, latestID);
                int receiverBalance = getBalance(receiver, &latestID);
                // printf("DEBUG: Receiver's Balance is: %d, %d\n", receiverBalance, latestID);

                if (senderBalance < 0 && receiverBalance < 0) {
                    status = 403;
                } else if (senderBalance < 0) {
                    status = 401;
                } else if (receiverBalance < 0) {
                    status = 402;
                } else if (senderBalance < amount){
                    status = 404;
                } else { // valid transaction
                    int next = getRandomServer();
                    // int next = 2;
                    senderBalance -= amount;
                    char req[MAX_BYTES];
                    sprintf(req, "%d,%d,%s,%s,%d", 101, latestID+1, sender, receiver, amount);
                    sendtoPort(serverPorts[next], req);
                    printf("The main server sent a request to server %s.\n", servers[next]);
                    
                    // listen to feedback from backend server
                    char res[MAX_BYTES];
                    if (recvfromPort(PORT_UDP, res)!=0) {
                        fprintf(stderr,"ServerM cannot receive response from Server%s.\n", servers[next]);
                        exit(1);
                    }
                    int res_status;
                    sscanf(res, "%d", &res_status);
                    if (res_status==200) {
                        printf("The main server received the feedback from server %s using UDP over port %s.\n", servers[next], serverPorts[next]);
                    }
                }
                // prepare res content
                sprintf(res, "%d,%d", status, senderBalance);
                printf("The main server sent the result of the transaction to client %s.\n", clientName);
            } else if (req_type == 102) { // TXLIST
                printf("A TXLIST request has been received from client %s.\n", clientName);
                sscanf(buf_copy, "%d,%s", &req_type, userName);
                getLogList();
                printf("The sorted file is up and ready.\n");
            }
            // send response
            if (send(new_fd, res, strlen(res), 0)==-1) {
                perror("send");
            }
            close(new_fd);
            exit(0);
        }
        close(new_fd);
    }
    return 0;
}



int main (void) {

    if (fork()==0) {
        setUpTCP(PORT_ClientA, "A");
        exit(0);
    } else {
        ;
    }

    if (fork()==0) {
        setUpTCP(PORT_ClientB, "B");
        exit(0);
    } else {
        ;
    }

    // wait for child process to die
    while (waitpid(-1, NULL, 0) != -1) {
        ;
    } 

    return 0;
}