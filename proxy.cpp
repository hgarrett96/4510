#include "proxy.h"

// Receive a message to a given socket file descriptor
void receiveMsg(int s, int size, char *ptr, bool headerOnly) {
	int received = 0;
	// First time we enter the ptr points to start of buffer
	char* buf = ptr;
	// Subtract 1 to avoid completely filling buffer
	// Leave space for '\0'
	while ((size > 0) && (received = read(s, ptr, size - 1))) {
		// Check error
		if (received < 0 && !done) {
			break;
			//error("Read failed"); //
		}

		// Move pointer forward in struct by size of received data
		ptr += received;

		// Keep track of how much data has been received
		size -= received;

		// Read only header
		if (headerOnly) {
			// Exit loop if CRLF CRLF has been received
			if (strstr(buf, CRLF CRLF)) {
				break;
			}
		}
	}
}

void sendMsg(int s, int size, char *ptr) {
	int sent = 0;
	while ((size > 0) && (sent = write(s, ptr, size))) {
		if (sent < 0) {
			break;
		}
		// Move pointer forward in struct by size of sent data
		ptr += sent;
		// Keep track of how much data has been sent
		size -= sent;
	}
}

void cleanOnError(int sock, char* &buffer) {
	if (sock > -1) {
		shutdown(sock, SHUT_RDWR);
		close(sock);
	}
	delete[] buffer;
}

void consume(int clientSocket) {
	while (!done) {
		int sock = clientSocket;
		if (!done && sock >= 0) {
			char* buffer = new char[MAX_MSG_SIZE];
			memset(buffer, '\0', MAX_MSG_SIZE);
			receiveMsg(sock, MAX_MSG_SIZE, buffer, true);
			
			char* savePtr;
			char* recvCMD;
			char* recvHost;
			char* recvVer;
			char* parsedHeaders;
			recvCMD = strtok_r(buffer, " ", &savePtr);
			recvHost = strtok_r(NULL, " ", &savePtr);
			recvVer = strtok_r(NULL, CRLF, &savePtr);
			parsedHeaders = strtok_r(NULL, CRLF, &savePtr);

			// Check
			if (!recvCMD || (strcmp(recvCMD, "GET") != 0) || !recvHost
					|| !recvVer) {
				sendMsg(sock, strlen(errMsg), (char*) errMsg);
				cleanOnError(sock, buffer);
				continue;
			}
			if (strcmp(recvVer, "HTTP/1.1") == 0 || strcmp(recvVer, "HTTPS/1.1") == 0 || strcmp(recvVer, "HTTPS/1.0") == 0) {
				cout << errMsg << endl;
				cleanOnError(sock, buffer);
				exit(-1);
			}

			
			string::size_type index;
			map<string, string> headerMap;
			while (parsedHeaders != NULL) {
				index = string(parsedHeaders).find(':', 0);
				if (index != string::npos) {
					pair<string, string> tempPair;
					string key = string(parsedHeaders).substr(0, index + 1);
					transform(key.begin(), key.end(), key.begin(), ::tolower);
					string value = string(parsedHeaders).substr(index + 1);
					tempPair = make_pair(key, value);
					headerMap.insert(tempPair);
				}
				parsedHeaders = strtok_r(NULL, CRLF, &savePtr);
			}
			map<string, string>::iterator it = headerMap.find(
					"connection:");
			if (it != headerMap.end()) {
				it->second = " close";
			} else {
				pair<string, string> tempPair;
				tempPair = make_pair("connection:", " close");
				headerMap.insert(tempPair);
			}
			
			string recvHostStr = string(recvHost);
			string http = "http://";
			index = recvHostStr.find(http);
			if (index != string::npos) {
				recvHostStr.erase(index, http.length());
			}

			string parsedRelPath;
			index = recvHostStr.find('/');
			if (index != string::npos) {
				parsedRelPath = recvHostStr.substr(index);
				recvHostStr.erase(index, parsedRelPath.length());
			}
			
			string beginning = "http://www.";
			if(string(recvHost).find(beginning) == string::npos){
				cout << errMsg << endl;
				exit(-1);
			}

			string parsedPort;
			index = recvHostStr.find(':');
			if (index != string::npos) {
				parsedPort = recvHostStr.substr(index + 1);
			} else {
				parsedPort = "80";
			}

			stringstream ss;
			ss << recvCMD << " " << parsedRelPath << " " << recvVer << CRLF;

			for (it = headerMap.begin(); it != headerMap.end(); ++it) {
				ss << it->first << it->second << CRLF;
			}

			it = headerMap.find("host:");
			if (it == headerMap.end()) {
				ss << "Host: " << recvHostStr << CRLF;
			}

			ss << CRLF;
			string fullRequest = ss.str();
			ss.str(string());
			ss.clear();

			struct sockaddr_in sa;
			bzero((char*) &sa, sizeof(sa));

			struct hostent *host = gethostbyname(recvHostStr.c_str());
			if (!host) {
				cleanOnError(sock, buffer);
				continue;
			}

			memcpy(&sa.sin_addr.s_addr, host->h_addr_list[0], host->h_length);

			int port = atoi(parsedPort.c_str());
			sa.sin_family = AF_INET;
			sa.sin_port = htons(port);

			int web_s;
			if ((web_s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
				cleanOnError(sock, buffer);
				continue;
			}

			if (connect(web_s, (struct sockaddr*) &sa, sizeof(sa)) < 0) {
				cleanOnError(sock, buffer);
				continue;
			}

			char* req = new char[fullRequest.length() + 1];
			memset(req, '\0', fullRequest.length() + 1);
			strcpy(req, fullRequest.c_str());

			sendMsg(web_s, strlen(req), req);
			delete[] req;

			char* resBuffer = new char[MAX_MSG_SIZE];
			memset(resBuffer, '\0', MAX_MSG_SIZE);
			receiveMsg(web_s, MAX_MSG_SIZE, resBuffer, false);

			shutdown(web_s, SHUT_RDWR);
			close(web_s);

			sendMsg(sock, MAX_MSG_SIZE, resBuffer);

			delete[] buffer;
			delete[] resBuffer;
			shutdown(sock, SHUT_RDWR);
			close(sock);
		}
	}
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		error("usage: ./MyProxy <port>\n");
	}

	struct sockaddr_in server;
	memset(&server, 0, sizeof(server));
	
	int port = atoi(argv[1]);
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(port);

	if ((server_s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		error("Failed to open socket");
	}

	if (bind(server_s, (struct sockaddr*) &server, sizeof(server)) < 0) {
		error("Failed to bind");
	}

	cout << "Listening:" << port << "..." << endl;
	if (listen(server_s, SOMAXCONN) < 0) {
		error("Failed to listen");
	}
	struct sockaddr_in client;
	memset(&client, 0, sizeof(client));
	socklen_t csize = sizeof(client);
	
	while (!done) {
		// Accept client connections
		int client_s = accept(server_s, (struct sockaddr*) &client, &csize);

		// Prevent error from showing if program is exiting
		// Not a reason to terminate proxy
		if (client_s < 0 && !done) {
			shutdown(client_s, SHUT_RDWR);
			close(client_s);
			continue;
		}
		consume(client_s);
	}
	return 0;
}
