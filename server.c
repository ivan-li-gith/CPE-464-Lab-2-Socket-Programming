/******************************************************************************
* myServer.c
* 
* Writen by Prof. Smith, updated Jan 2023
* Use at your own risk.  
*
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdint.h>

#include "networks.h"
#include "safeUtil.h"
#include "new.h"
#include "pollLib.h"

#define MAXBUF 1024
#define DEBUG_FLAG 1

void recvFromClient(int clientSocket);
int checkArgs(int argc, char *argv[]);
void addNewSocket(int mainServerSocket);
void processClient(int clientSocket);
void serverControl(int mainServerSocket);

// to handle closing the server with multiple clients
volatile int activeClients = 0;

int main(int argc, char *argv[])
{
	int mainServerSocket = 0;   //socket descriptor for the server socket
	// int clientSocket = 0;   //socket descriptor for the client socket
	int portNumber = 0;
	
	portNumber = checkArgs(argc, argv);
	
	//create the server socket
	mainServerSocket = tcpServerSetup(portNumber);

	serverControl(mainServerSocket);


	// while(1){
	// 	// wait for client to connect
	// 	clientSocket = tcpAccept(mainServerSocket, DEBUG_FLAG);

	// 	recvFromClient(clientSocket);
		
	// 	/* close the sockets */
	// 	close(clientSocket);

	// }

	close(mainServerSocket);


	
	return 0;
	
}

void recvFromClient(int clientSocket)
{
	uint8_t dataBuffer[MAXBUF];
	int messageLen = 0;
	
	//now get the data from the client_socket
	if ((messageLen = recvPDU(clientSocket, dataBuffer, MAXBUF)) < 0)
	{
		perror("recv call");
		exit(-1);
	}

	if (messageLen > 0)
	{
		printf("Message received, length: %d Data: %s\n", messageLen, dataBuffer);
	}
	else
	{
		printf("Connection closed by other side\n");
	}
}

int checkArgs(int argc, char *argv[])
{
	// Checks args and returns port number
	int portNumber = 0;

	if (argc > 2)
	{
		fprintf(stderr, "Usage %s [optional port number]\n", argv[0]);
		exit(-1);
	}
	
	if (argc == 2)
	{
		portNumber = atoi(argv[1]);
	}
	
	return portNumber;
}


void serverControl(int mainServerSocket){
	// start the poll set
	setupPollSet();

	// add main server to poll set
	addToPollSet(mainServerSocket);

	while(1){
		// call poll
		int socket = pollCall(POLL_WAIT_FOREVER);

		// if poll returns main server socket call addNewSocket else if poll returns client socket call processClient
		if(socket == mainServerSocket){
			addNewSocket(mainServerSocket);
		}else{
			processClient(socket);
		}
	}

	// close all sockets
	printf("Server shutting down\n");
    close(mainServerSocket);
}

void addNewSocket(int mainServerSocket){
	// process new connection 
	// accepts socket
	int newClientSocket = tcpAccept(mainServerSocket, DEBUG_FLAG);

	// error handling
	if(newClientSocket < 0){
		perror("Cant accept new client");
		return;
	}

	// add to poll set
	addToPollSet(newClientSocket);
	activeClients++;
}

void processClient(int clientSocket) {
    uint8_t buffer[MAXBUF];
    int bytesReceived;


	bytesReceived = recvPDU(clientSocket, buffer, MAXBUF);

	// when the client hits Control C remove the socket from the poll set
	if (bytesReceived == 0) {
		printf("Client on socket %d disconnected\n", clientSocket);
		removeFromPollSet(clientSocket);
		close(clientSocket);
		activeClients--;

		if (activeClients == 0) {
            exit(0); 
        }

		return;
	}

	if (bytesReceived < 0) {
		perror("Could not receive PDU");
		removeFromPollSet(clientSocket);
		close(clientSocket);
        activeClients--;

		if (activeClients == 0) {
            exit(0); 
        }

		return;
	}

	printf("Message received on socket %d, length: %d Data: %s\n", clientSocket, bytesReceived, buffer);

	// if i remove this line i can no longer continuosly repeat the message
	// echo message back to the client
	sendPDU(clientSocket, buffer, bytesReceived);

}
