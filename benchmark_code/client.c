#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <pthread.h>
#define MAX_MSG_LEN 1000000 // 1 MB
#define MULTIPLER 10

struct sockaddr_in serv_addr;
int serverPort;
char serverIP[100];

int checkIsNumeric(char *string){
        int indx = 0;

        for(indx = 0; indx < strlen(string); indx++){
                if(!isdigit(string[indx])){
                        return 0;
                }
        }

        return 1;
}


int make_connection(){
	int sock = 0;

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("\n Socket creation error \n");
		return -1;
	}

	memset(&serv_addr, '0', sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(serverPort);

	// Convert IPv4 and IPv6 addresses from text to binary form
	if(inet_pton(AF_INET, serverIP, &serv_addr.sin_addr)<=0)
	{
		printf("\nInvalid address/ Address not supported \n");
		return -1;
	}

	return sock;
}

int send_message(char *msg, int connectionSocket){
	
        if(send(connectionSocket, msg, strlen(msg) , 0 ) >= 0){
                return 1;
        }

        return 0;
}

void *client_thread(void *args){
	char buffer[MAX_MSG_LEN];
	int connectionSocket;
	int indx = 0;

	connectionSocket = make_connection();

        if (connect(connectionSocket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
                printf("\nConnection Failed \n");
                return NULL;
        }

	for(indx = 0; indx < MULTIPLER; indx++){
		memset(buffer, '\0', sizeof(buffer));
		read(connectionSocket , buffer, sizeof(buffer));
		send_message(buffer, connectionSocket);
	}

	return NULL;
}

int invoke_client(int threadCount){
	int indx;
	pthread_t tid[threadCount];


	for(indx = 0; indx < threadCount; indx++){
                pthread_create(&tid[indx], NULL, client_thread, NULL);
        }

        for(indx = 0; indx < threadCount; indx++){
                pthread_join(tid[indx], NULL);
        }

	return 0;
}

void usage(){
        printf("Proper usage as follows:\n");
        printf("./client ip port_no thread_count\n");
	printf("ip: IP address on which server is running\n");
        printf("port_no: Port number on which server is listening\n");
        printf("thread_count: Number of threads that are used on Server\n");
        fflush(stdout);
        return;
}


int main(int argc, char const *argv[])
{
	char strPortNo[8];
	int threadCount;

	if(argc != 4){
		usage();
		exit(EXIT_FAILURE);
	} else {
		if(strlen(argv[2]) > 5){
			usage();
			exit(EXIT_FAILURE);
		}

		strncpy(strPortNo, argv[2], sizeof(strPortNo)-1);

		if(!checkIsNumeric(strPortNo)){
			usage();
			exit(EXIT_FAILURE);
		}

		serverPort = atoi(strPortNo);

		if(serverPort < 1024 || serverPort > 65535){
			usage();
			exit(EXIT_FAILURE);
		}

		strncpy(serverIP, argv[1], sizeof(serverIP)-1);

	}
	
	threadCount = atoi(argv[3]);
	invoke_client(threadCount);

	return 0;
}

