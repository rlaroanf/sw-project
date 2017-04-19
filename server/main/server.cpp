#include <cstdio>
#include <iostream>
#include <fstream>
#include <ctime>
#include "socketLayer.h"
#include "md5check.h"

using namespace std;

#define STRING_LENGTH 512

const int BUFFER_SIZE = SocketLayer::BUFFER_SIZE;
 
int main(){
	int port;
	printf("PORT: "); cin >> port;

	SocketLayer SL;
	SOCKET server;
	if( SL.Create(&server) == INVALID_SOCKET ){
		puts("FAIL TO CREATE SOCKET");
	}
	cout<< "Success socket create at port " << port <<endl;
	
	// Prepare the sockaddr_in structure
	sockaddr_in socketInfo = SL.MakeAddress(port);
	int socketBlockSize = sizeof(socketInfo);

	if( SL.Bind(server, socketInfo) == SOCKET_ERROR ){
		perror("Bind Error");
		exit(EXIT_FAILURE);
	}
	puts("Bind done");

	while( true ){
		sockaddr_in clientSocketInfo;
        ZeroMemory( &clientSocketInfo, sizeof(sockaddr_in) ); 

		char buf[BUFFER_SIZE] = {};
        // try to receive some data, this is a blocking call
        if (SL.Receive(server, buf, BUFFER_SIZE, clientSocketInfo) == SOCKET_ERROR){
            printf("recvfrom() failed with error code : %d" , WSAGetLastError());
            exit(EXIT_FAILURE);
		}
		printf("[Receive fisrt: %s]\n", buf);

		time_t lastTickTime = clock();
		long long lastTickFileSize = 0LL;
		// print details of the client/peer and the data received
		printf("Received packet from %s:%d\n", inet_ntoa(clientSocketInfo.sin_addr), ntohs(clientSocketInfo.sin_port));
		bool endFileReceive = false;
		string hashResult("");
		if (strncmp(buf, "<send-file>", 11) == 0){
			char filename[STRING_LENGTH] = {};
			strncpy(filename, buf + 12, STRING_LENGTH - 12);
			printf("[%s]\n", filename);

			ofstream outFile(filename, ios::out | ios::binary);
			printf("File received: %s ...\n", filename);
			int rsize = 0, sendCount = 0;
			while ((rsize = SL.Receive(server, buf, BUFFER_SIZE, clientSocketInfo)) > 0){
				if (strncmp(buf, "</send-file>", 12) == 0){
					endFileReceive = true;
					break;
				}
				time_t now = clock(), elapseSecond = (now - lastTickTime) / CLOCKS_PER_SEC;
				if(elapseSecond >= 1){
					double kbps = (lastTickFileSize / 1024LL) / (elapseSecond + (double)1e-9);
					printf("%s]... kbps: %.3lf KB/s\n", filename, kbps);
					lastTickTime = now;
					lastTickFileSize = 0;
				}
				lastTickFileSize += rsize;
				outFile.write(buf, rsize);
				hashResult = md5( hashResult + buf );
			}
			outFile.close();
			printf("[%s] end!\n", filename);
		}

		if(endFileReceive){
			// now reply the client with the result
			printf("convert hash to packet form\n");
			sprintf(buf, "<file-hash:%s", hashResult.c_str());
			printf("my hash is %s\n", hashResult.c_str());
			if (SL.Send(server, buf, BUFFER_SIZE, clientSocketInfo) == SOCKET_ERROR) {
				printf("sendto() failed with error code : %d\n", WSAGetLastError());
				exit(EXIT_FAILURE);
			}
			puts("sended..");
		}
	}
	SL.Close(server);

	return 0; 
 
} 
