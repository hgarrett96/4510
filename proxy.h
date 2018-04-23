using namespace std;
#ifndef PROXY_H_
#define PROXY_H_

#include <algorithm>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream>
#include <queue>
#include <arpa/inet.h>
#include <map>

#define CRLF "\r\n"

// Send/receive message buf size
#define MAX_MSG_SIZE	1000000

int server_s;

// Error message for client
const char* errMsg = "HTTP/1.0 500 'Internal Server Error'\r\n\r\n";

// Controls termination of program
static bool done = 0;

// Custom error handling
void error(const char *err) {
	perror(err);
	exit(EXIT_FAILURE);
}

void receiveMsg(int s, int size, char *ptr, bool headerOnly);
void sendMsg(int s, int size, char *ptr);
void cleanOnError(int sock, char* &dataBuf);
void consume(int clientSocket);
#endif /* PROXY_H_ */
