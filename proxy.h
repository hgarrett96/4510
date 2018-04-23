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
#include <semaphore.h>
#include <pthread.h>

#define MAX_THREADS		30
#define CRLF "\r\n"
// Permissions are READ/WRITE for OWNER and READ for others
// Only for use on systems where sem_init/sem_destroy are deprecated (OSX)
#define SEM_PERMISSIONS	0644

// Send/receive message buf size
#define MAX_MSG_SIZE	1000000

// Set to 0 to disable debug messages
#define DEBUG			0

// Multithreading and thread safety
pthread_mutex_t* count_mutex;
sem_t* job_queue_count;
char SEM_NAME[] = "sem";
int server_s;
std::queue<int>* socketQ;
pthread_t* pool;

// Error message for client
const char* errMsg = "HTTP/1.0 500 'Internal Server Error'\r\n\r\n";

// Controls termination of program
static bool done = 0;

// Custom error handling
void error(const char *err) {
	perror(err);
	exit(EXIT_FAILURE);
}



void termination_handler(int signum); // new
void receiveMsg(int s, int size, char *ptr, bool headerOnly);
void sendMsg(int s, int size, char *ptr);
void cleanOnError(int sock, char* &dataBuf);
void* consume(void* data);
void cleanup(); // new
void setupSigHandlers(); // new
void initSynchronization(); // new
void initThreadPool(); // new

#endif /* PROXY_H_ */
