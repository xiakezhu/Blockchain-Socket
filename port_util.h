#define PORT_ClientA "25899"
#define PORT_ClientB "26899"
#define PORT_ServerA "21899"
#define PORT_ServerB "22899"
#define PORT_ServerC "23899"
#define PORT_UDP "24899"

#define MAX_BYTES 1500
#define HOST "localhost"
#define MAXBUFLEN 1500

#define COMMA ","
#define WHITESPACE " "
#define BLOCK1 "block1.txt"
#define BLOCK2 "block2.txt"
#define BLOCK3 "block3.txt"

// struct for transaction log
typedef struct TransationLog {
	int id;
	char sender[MAXBUFLEN];
	char receiver[MAXBUFLEN];
	int amount;
} TXLog;

// Fill TXLog object with logStr
void fillTXLog(TXLog log, char* logStr) {
	int id, amount;
	char sender[MAXBUFLEN], receiver[MAXBUFLEN];
	sscanf(logStr, "%d %s %s %d", &id, sender, receiver, &amount);
	log.id = id;
	log.amount = amount;
	strcpy(log.sender, sender);
	strcpy(log.receiver, receiver);
}

// Print TXLog details
void PrintTXLog(TXLog log) {
	printf("ID: %d, Sender: %s, Receiver: %s, Amount: %d\n", log.id, log.sender, log.receiver, log.amount);
}

/**
 * @brief From https://beej.us/guide/bgnet/examples/listener.c.
 * 
 */
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// Parse input log into id, sender, receiver and amount.
void parseSingleLog(char* log, int* id, char* sender, char* receiver, int* amount) {
	char* token = strtok(log, WHITESPACE);
	*id = atoi(token);
	token = strtok(NULL, WHITESPACE);
	strcpy(sender, token);
	token = strtok(NULL, WHITESPACE);
	strcpy(receiver, token);
	token = strtok(NULL, WHITESPACE);
	*amount = atoi(token);
}

/**
 * @brief Scan specific text file and get flag and balance in the logs
 * 
 * Code modified from https://pubs.opengroup.org/onlinepubs/9699919799/functions/getdelim.html
 * 
 * @param fileName 
 * @param uName 
 * @param flag 
 * @param balance 
 */
void scanLogs(char* fileName, char* uName, int* flag, int* balance, int* latestID) {
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	fp = fopen(fileName, "r");
	if (fp == NULL)
		exit(1);
	while ((read = getline(&line, &len, fp)) != -1) {
		int id, amount;
		char sender[MAX_BYTES];
		char receiver[MAX_BYTES];
		parseSingleLog(line, &id, sender, receiver, &amount);
		if (id > *latestID) {
			*latestID = id;
		}
		if (strcmp(sender, uName)==0) {
			*flag = 1;
			*balance -= amount;
		} else if (strcmp(receiver, uName)==0) {
			*flag = 1;
			*balance += amount;
		}
	}
	free(line);
	fclose(fp);
}

/**
 * @brief Send UDP datagram to certain port with content.
 * 
 * Code modified from https://beej.us/guide/bgnet/examples/talker.c.
 * 
 * @param port 
 * @param content 
 */
int sendtoPort(char* port, char* content) {
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;

    memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6; // set to AF_INET to use IPv4
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo(HOST, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("talker: socket");
			continue;
		}
		break;
	}

	if (p == NULL) {
		fprintf(stderr, "talker: failed to create socket\n");
		return 2;
	}

	if ((numbytes = sendto(sockfd, content, strlen(content), 0,
			 p->ai_addr, p->ai_addrlen)) == -1) {
		perror("talker: sendto");
		exit(1);
	}

	freeaddrinfo(servinfo);
    
	close(sockfd);

	return 0;

}

/**
 * @brief Receive content from certain port. Not listening all the time.
 * 
 * Code modified from https://beej.us/guide/bgnet/examples/listener.c.
 * 
 * @param port 
 * @param content 
 * @return int 
 */
int recvfromPort(char* port, char* content) {
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	struct sockaddr_storage their_addr;
	char buf[MAX_BYTES];
	socklen_t addr_len;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6; // set to AF_INET to use IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
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

	addr_len = sizeof their_addr;
	// blocking recvfrom call
	if ((numbytes = recvfrom(sockfd, buf, MAX_BYTES-1 , 0,
		(struct sockaddr *)&their_addr, &addr_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}
	buf[numbytes] = '\0';
	strcpy(content, buf);
    close(sockfd);
	return 0;
}

/**
 * @brief Append one log to the certain text file
 * 
 * Code modified from https://pubs.opengroup.org/onlinepubs/9699919799/functions/getdelim.html
 * 
 * @param fileName 
 * @param log 
 * @return int 
 */
int appendLog(char* fileName, char* log) {
	FILE *fp;
	fp = fopen(fileName, "a");
	if (fp == NULL)
		exit(1);
	else {
		fprintf(fp, "%s\n", log); // add newline
	}
	fclose(fp);
	return 0;
}

/**
 * @brief Get the Random Server index
 * 
 * @return int [0,1,2]
 */
int getRandomServer() {
	return rand() % 3;
}

/**
 * @brief Inflate logs into a c-string
 * 
 * Code modified from https://pubs.opengroup.org/onlinepubs/9699919799/functions/getdelim.html
 * 
 * @param fileName 
 * @param logs need to be initialized already
 * @return int 
 */
int inflateLogs(char* fileName, char* logs) {
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	fp = fopen(fileName, "r");
	if (fp == NULL)
		exit(1);
	while ((read = getline(&line, &len, fp)) != -1) {
		line[strlen(line)-1] = '\0';
		strcat(logs, line);
		strcat(logs, ",");
	}
	logs[strlen(logs)-1] = '\0';
	free(line);
	fclose(fp);
	return 0;
}