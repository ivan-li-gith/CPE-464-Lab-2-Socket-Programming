/******************************************************************************
* myClient.c
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

void sendToServer(int socketNum);
int readFromStdin(uint8_t * buffer);
void checkArgs(int argc, char * argv[]);
void clientControl(int socketNum);
void processStdin(int socketNum);
void processMsgFromServer(int socketNum);

int main(int argc, char * argv[])
{
	int socketNum = 0;         //socket descriptor
	checkArgs(argc, argv);

	/* set up the TCP Client socket  */
	socketNum = tcpClientSetup(argv[1], argv[2], DEBUG_FLAG);
	// sendToServer(socketNum);

	clientControl(socketNum);
	close(socketNum);
	return 0;
}

void sendToServer(int socketNum){
	// buffers for sending and receiving 
	uint8_t sendBuf[MAXBUF];   	  
	uint8_t recvBuffer[MAXBUF];   

	int sendLen = 0;        //amount of data to send
	int sent = 0;           //actual amount of data sent/* get the data and send it   */
	int msgReceived;

	while(1){
		sendLen = readFromStdin(sendBuf);
		printf("read: %s string len: %d (including null)\n", sendBuf, sendLen);

		sent = sendPDU(socketNum, sendBuf, sendLen);

		if(sent < 0){
			perror("send call");
			exit(-1);
		}

		printf("Amount of data sent is: %d\n", sent);

		msgReceived = recvPDU(socketNum, recvBuffer, MAXBUF);
		// printf("msgReceived = %d\n", msgReceived);

		if(msgReceived == 0){
			printf("Server has terminated\n");
			exit(-1);
		}

		if(msgReceived < 0){
			perror("Unable to receive from server");
			exit(-1);
		}

		printf("msgReceived = %d\n", msgReceived);

	}

	// close socket
	close(socketNum);
	exit(0);
}

void clientControl(int socketNum){
	// start the poll set
	setupPollSet();

	// add stdin and the socket to the poll set
	addToPollSet(STDIN_FILENO);
	addToPollSet(socketNum);

	while(1){
		// flush out stdout
		printf("Enter a message:\n");
		fflush(stdout);

		// call poll
		int socket = pollCall(POLL_WAIT_FOREVER);

		// if poll returns main server socket call addNewSocket else if poll returns client socket call processClient
		if(socket == STDIN_FILENO){
			processStdin(socketNum);
		}else{
			processMsgFromServer(socketNum);
		}
	}

	// close all sockets
	printf("Server shutting down\n");
    close(socketNum);
}

// send message to server
void processStdin(int socketNum){
	// buffer to store input
    uint8_t sendBuf[MAXBUF];
    int sendLen;

    // read user input
    sendLen = read(STDIN_FILENO, sendBuf, MAXBUF);

    // handle the new line -> replace new line with null to format string
    sendBuf[sendLen - 1] = '\0';

    // send message to server
    if(sendPDU(socketNum, sendBuf, sendLen) < 0) {
        perror("Error sending message");
		exit(-1);
    }else{
        printf("Message sent: %s\n", sendBuf);
    }
}

// takes that echoed message from the server and prints it out
void processMsgFromServer(int socketNum){	
	// buffer for the message
    uint8_t recvBuffer[MAXBUF];

    int msgReceived = recvPDU(socketNum, recvBuffer, MAXBUF);

    if(msgReceived == 0){
		// control C
        printf("Server terminated\n");
        exit(0);
    }else if(msgReceived < 0) {
        perror("Error receiving from server");
        exit(-1);
    }else{
		// print out the message
        printf("Received from server: %s\n", recvBuffer);
    }
}


int readFromStdin(uint8_t * buffer)
{
	char aChar = 0;
	int inputLen = 0;        
	
	// Important you don't input more characters than you have space 
	buffer[0] = '\0';
	printf("Enter data: ");
	while (inputLen < (MAXBUF - 1) && aChar != '\n')
	{
		aChar = getchar();
		if (aChar != '\n')
		{
			buffer[inputLen] = aChar;
			inputLen++;
		}
	}
	
	// Null terminate the string
	buffer[inputLen] = '\0';
	inputLen++;
	
	return inputLen;
}

void checkArgs(int argc, char * argv[])
{
	/* check command line arguments  */
	if (argc != 3)
	{
		printf("usage: %s host-name port-number \n", argv[0]);
		exit(1);
	}
}
