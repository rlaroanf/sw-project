#pragma once

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <string>
#include <vector>
#include <fstream>
using namespace std;

#pragma comment(lib, "ws2_32.lib") // Winsock Library

typedef struct sockaddr_in sockaddr_in;

#define STRING_LENGTH 512
#define BUFFER_SIZE  4096

void ErrorHandling(char* message) {
	fprintf(stderr, "%s : Error Code : \n", message, WSAGetLastError());
	exit(EXIT_FAILURE);
}

int ServerSendToClient(bool isTCP, SOCKET& sock, char *buffer, int bufferSize, sockaddr_in& echoAddr, int addrSize) {
	bool success = true;
	int msgSize = 0;
	if (isTCP) {
	}
	else {
		msgSize = sendto(sock, buffer, bufferSize, 0, (sockaddr *)&echoAddr, addrSize);
		success &= msgSize > 0;
	}

	if (success == false)
		ErrorHandling("sendto() sent a different number of bytes than expected");
	return msgSize;
}

int ServerReceiveFromClient(bool isTCP, SOCKET& sock, char *buffer, int bufferSize, sockaddr_in& echoAddr, int *addrSize) {
	bool success = true;
	int msgSize = 0;
	if (isTCP) {
	}
	else {
		msgSize = recvfrom(sock, buffer, bufferSize, 0, (sockaddr *)&echoAddr, addrSize);
		success &= msgSize > 0;
	}

	if (success == false)
		ErrorHandling("sendto() sent a different number of bytes than expected");
	return msgSize;
}

int ClientSendToServer(bool isTCP, SOCKET& sock, char *buffer, int bufferSize, sockaddr_in& echoAddr, int addrSize) {
	bool success = true;
	int msgSize = 0;
	if (isTCP) {
	}
	else {
		msgSize = sendto(sock, buffer, bufferSize, 0, (sockaddr *)&echoAddr, addrSize);
		success &= msgSize > 0;
	}
	
	if( success == false )
		ErrorHandling("sendto() sent a different number of bytes than expected");
	return msgSize;
}

int ClientReceiveFromServer(bool isTCP, SOCKET& sock, char *buffer, int bufferSize, sockaddr_in& echoAddr, int *addrSize) {
	bool success = true;
	int msgSize = 0;
	if (isTCP) {
	}
	else {
		msgSize = recvfrom(sock, buffer, bufferSize, 0, (sockaddr *)&echoAddr, addrSize);
		success &= msgSize > 0;
	}

	if (success == false)
		ErrorHandling("sendto() sent a different number of bytes than expected");
	return msgSize;
}

int MakeMessage(char *buffer, char *message, int bufferSize = -1) {
	ZeroMemory(buffer, BUFFER_SIZE);
	int messageLength = bufferSize < 0 ? strlen(message) : bufferSize;
	strncpy(buffer, message, messageLength);
	return messageLength;
}

bool isBeginToSendFile(char* buffer, char* filename) {
	char token[] = "<send-file>";
	int tokenLength = strlen(token);
	strcpy(filename, buffer + tokenLength);
	filename[tokenLength] = 0;
	return strncmp(buffer, token, tokenLength) == 0;
}
bool isEndToSendFile(char* buffer) {
	return strncmp(buffer, "</send-file>", strlen("</send-file>")) == 0;
}

// return: fileSize
long long SendFileToServer(bool isTCP, char *filename, SOCKET& sock, sockaddr_in& echoAddr, int addrSize) {
	bool success = true;
	ifstream file(filename, ios::in | ios::binary);
	if (!file.is_open()) {
		fprintf(stderr, "File is not exists: \"%s\"\n", filename);
		return -1;
	}

	char buffer[BUFFER_SIZE] = {};
	sprintf(buffer, "<send-file>%s", filename);
	ClientSendToServer(isTCP, sock, buffer, strlen(buffer), echoAddr, addrSize);

	long long totalFileSize = file.tellg();
	file.seekg(0, ios::end);
	totalFileSize = file.tellg() - totalFileSize;
	file.seekg(0, ios::beg);
	int sendCount = 0;
	long long calculatedFileSize = 0LL;
	do {
		memset(buffer, 0, sizeof(buffer));
		file.read((char *)&buffer, BUFFER_SIZE);
		if (file.eof()) {
			file.clear();
			file.seekg(0, ios::end);
			long long realRestSize = file.tellg() - (long long)(sendCount*BUFFER_SIZE);
			// cout << realRestSize << '\n';

			file.clear();
			file.seekg(-realRestSize, ios::end);
			memset(buffer, 0, sizeof(buffer));
			file.read((char *)&buffer, realRestSize);
			ClientSendToServer(isTCP, sock, buffer, realRestSize, echoAddr, addrSize);
			calculatedFileSize += realRestSize;
			break;
		}
		else {
			Sleep(10);
			ClientSendToServer(isTCP, sock, buffer, BUFFER_SIZE, echoAddr, addrSize);
			++sendCount;
			calculatedFileSize += BUFFER_SIZE;
		}
	} while (file.tellg() >= 0);
	file.close();

	int msgLen = MakeMessage(buffer, "</send-file>");
	ClientSendToServer(isTCP, sock, buffer, msgLen, echoAddr, addrSize);

	if (success == false)
		ErrorHandling("sendto() sent a different number of bytes than expected");

	return calculatedFileSize;
}

long long SaveFileToServer(bool isTCP, char *buffer, SOCKET& sock, sockaddr_in& echoAddr, int* addrSize) {
	char filename[STRING_LENGTH] = {};
	if (isBeginToSendFile(buffer, filename)) {
		printf("전송되는 파일명: %s\n", filename);
		while (isEndToSendFile(buffer) == false) {
			puts(buffer);
			ZeroMemory(buffer, BUFFER_SIZE);
			/* Block until receive message from a client */
			int recvBlockSize = ServerReceiveFromClient(isTCP, sock, buffer, BUFFER_SIZE, echoAddr, addrSize);
		}
		printf("end with [%s]\n", buffer);
	} else {
		// this is not file sending packet
		return -1;
	}
}