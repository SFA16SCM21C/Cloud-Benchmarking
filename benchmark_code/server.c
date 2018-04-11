#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define MAX_MSG_LEN 1000000 // 1 MB
#define MULTIPLIER 10

struct sockaddr_in address;
int addrlen = sizeof(address);
char sendString[MAX_MSG_LEN];
double *currentTimeElapsed;

typedef struct{
        int serverFd;
	int threadIndx;
}OperationInfo;


void createString(){
	int indx;
	memset(sendString, '\0', sizeof(sendString));
	for(indx = 0; indx < MAX_MSG_LEN-1; indx++){
		sendString[indx] = 'a';	
	}
}

int check_numeric(char *string){
        int indx = 0;

        for(indx = 0; indx < strlen(string); indx++){
                if(!isdigit(string[indx])){
                        return 0;
                }
        }

        return 1;
}


int send_message(char *msg, int communicationSocket){
	if(send(communicationSocket, msg, strlen(msg) , 0 ) >= 0){
		return 1;
	}

	return 0;
}

char *trim(char *str){
        int indx = 0;

        for(indx = strlen(str)-1; indx >= 0; indx--){
                if(str[indx] != ' ' && str[indx] != '\t' && str[indx] != '\r' && str[indx] != '\n'){
                        break;
                }
        }

        str[indx+1] = '\0';

        return str;
}


int createConnection(int serverPort){
	int serverFd;
        int opt = 1;

	// Creating socket file descriptor
        if ((serverFd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
        {
                perror("socket failed");
                exit(EXIT_FAILURE);
        }

        // Forcefully attaching socket to the port
        if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
        {
                perror("setsockopt");
                exit(EXIT_FAILURE);
        }
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons( serverPort );

        // Forcefully attaching socket to the port
        if (bind(serverFd, (struct sockaddr *)&address, sizeof(address))<0)
        {
                perror("bind failed");
                exit(EXIT_FAILURE);
        }

	return serverFd;

}


int recieve_message(char *buffer, int length, int communicationSocket){
	if(read(communicationSocket, buffer, length) < 0){
		return 0;
        }

	return 1;
}

void *server_thread(void *args){
	int serverFd;

	int newSocket;
	char buffer[MAX_MSG_LEN];
	int indx;
	int threadindx ;
	struct timeval startTime, endTime;

	OperationInfo *operation = (OperationInfo *)args;
	serverFd = operation->serverFd;
	threadindx = operation->threadIndx;


	if (listen(serverFd, 3) < 0)
        {
                perror("listen");
                exit(EXIT_FAILURE);
        }
	

	
	if ((newSocket = accept(serverFd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
        {
                perror("accept");
                exit(EXIT_FAILURE);
        }

	
	gettimeofday(&startTime, NULL);
	for(indx = 0; indx < MULTIPLIER; indx++){
		send_message(sendString, newSocket);
		memset(buffer, '\0', sizeof(buffer));
		recieve_message(buffer, sizeof(buffer), newSocket);
	}
	gettimeofday(&endTime, NULL);
        currentTimeElapsed[threadindx] = (((double)(endTime.tv_sec*1000000 - startTime.tv_sec * 1000000 + endTime.tv_usec - startTime.tv_usec))/1000000);
	return NULL;
}

int invoke_server(int serverPort, int threadCount)
{
	pthread_t tid[threadCount];
	int indx;
	int serverFd;
	double  mbTransfered, totalTimeElapsed = 0;	
	OperationInfo sendData[threadCount];
	serverFd = createConnection(serverPort);

	for(indx = 0; indx < threadCount; indx++){
		sendData[indx].serverFd = serverFd;
                sendData[indx].threadIndx = indx;
		pthread_create(&tid[indx], NULL, server_thread, (void *)(sendData + indx));
	}

	for(indx = 0; indx < threadCount; indx++){
		pthread_join(tid[indx], NULL);
		totalTimeElapsed += currentTimeElapsed[indx];
	}

	mbTransfered = (MAX_MSG_LEN*MULTIPLIER*threadCount*2)/(1000000.0); //divide by to for turn around time

	printf("\n\t\t %lf MB in time:%lf", mbTransfered, totalTimeElapsed);
        printf("\n\t\t Throughput %lf MBps\n", mbTransfered/totalTimeElapsed);
        fflush(stdout);

	return 0;
}

void usage(){
        printf("Proper usage as follows:\n");
        printf("./server port_no thread_count\n");
        printf("port_no: Available port between 1024 and 65535\n");
	printf("thread_count: Number of threads\n");
        fflush(stdout);
        return;
}

int main(int argc, char const *argv[]){
	int serverPort = 12345;
        char strPortNo[8];
	int threadCount;
	createString();

	if(argc != 3){
		usage();
		return 0;
	}

	threadCount = atoi(argv[2]);

       
                if(strlen(argv[1]) > 5){
                        printf("Invalid Port Number");
                        exit(EXIT_FAILURE);
                }
         
                strncpy(strPortNo, argv[1], sizeof(strPortNo)-1);

                if(!check_numeric(strPortNo)){
                        printf("Non Numeric Port Number not allowed");
                        exit(EXIT_FAILURE);
                }

                serverPort = atoi(strPortNo);

                if(serverPort < 1024 || serverPort > 65535){
                        printf("Invalid Port Number");
                        exit(EXIT_FAILURE);
                }

	currentTimeElapsed = (double *)calloc(threadCount, sizeof(double));	
	printf("Performing with %d thread\n", threadCount);
	fflush(stdout);
	invoke_server(serverPort, threadCount);

	return 0;	
}

