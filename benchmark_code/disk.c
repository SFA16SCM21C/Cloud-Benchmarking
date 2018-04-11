#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <memory.h>
#include <string.h>

#define FILE_SIZE 10000000000
#define LINE_SIZE 100000
#define ACCESS_SIZE 1000000000 // Will try to read only 1 GB of file by all thread
// Array acting as source of operations

// structure to send information to threads
typedef struct{
	long int startSeq;
	long int blockSize;
	long int totalTimes; 
	char *dest;
}OperationInfo;

char *getArray(int size){
        char *write_array = NULL;
        int counter = 0;
        write_array = (char *)calloc(size+10, sizeof(char));;

	if(write_array == NULL){
		perror("malloc:");
	}

        for(counter = 0; counter < size-1; counter++){
                write_array[counter] = 'a';
        }
	write_array[size-1] = '\0';

        return write_array;
}


//allocate memory to source array
void createFile(){
	long int indx = 0;
	int i = 0;
	char source[LINE_SIZE]; 
	int totalLines = FILE_SIZE/LINE_SIZE;
	FILE *fp = fopen("test.txt", "w+");

	memset(source, '\0', sizeof(source));

	for(i = 0; i < LINE_SIZE-1; i++){
		source[i] = 'a'+indx;
	}

	if(fp == NULL){
		printf("Unable to open file\n");
		fflush(stdout);
		exit(1);
	}

	printf("Creating test file\n");
	fflush(stdout);

	for(indx = 0; indx < totalLines; indx++){
		fprintf(fp, "%s\n", source);
	}
	fclose(fp);

	printf("Test file created\n");
	fflush(stdout);
}

// Thread to read sequencially
void *sequential_read(void *args){
	// read arguments
	FILE *fp = fopen("test.txt", "r+");
	OperationInfo *operation = (OperationInfo *)args; 
	int count = operation->totalTimes;
	long int blockSize = operation->blockSize;
	int startLine = operation->startSeq;
	char *readBuffer = operation->dest;;

	fseek(fp, startLine, SEEK_SET);
	for(; count>0; count--){
		fread(readBuffer, sizeof(char), blockSize, fp);
	}	
	fclose(fp);
	return NULL;
}

// Thread to write sequencially
void *sequential_write(void *args){
	// read arguments
	FILE *fp = fopen("test.txt", "a+");
	OperationInfo *operation = (OperationInfo *)args;
        int count = operation->totalTimes;
        long int blockSize = operation->blockSize;
	int startLine = operation->startSeq;
        char *writeBuffer = operation->dest;

	fseek(fp, startLine, SEEK_SET);
        for(; count>0; count--){
		fwrite(writeBuffer, sizeof(char), blockSize, fp);
        }
	fclose(fp);
	return NULL;
}

// Thread to read randomly
void *random_read(void *args){
	// read arguments
	srand(time(0));
	FILE *fp = fopen("test.txt", "r+");
	int loc;
	srand(time(0));
	OperationInfo *operation = (OperationInfo *)args;
        int count = operation->totalTimes;
        long int blockSize = operation->blockSize;
	int startLine = operation->startSeq;
	char *readBuffer = operation->dest;;
	int randLimit = count;

        for(; count>0; count--){
		loc = startLine + (rand() % randLimit);	
		fseek(fp, loc, SEEK_SET);
		fread(readBuffer, sizeof(char), blockSize, fp);
        }
	fclose(fp);
        return NULL;
}

// Thread to write randomly
void *random_write(void *args){
	// read arguments
	srand(time(0));
        FILE *fp = fopen("test.txt", "a+");
	int loc;
	OperationInfo *operation = (OperationInfo *)args;
        int count = operation->totalTimes;
	long int blockSize = operation->blockSize;
        int startLine = operation->startSeq;
	char *writeBuffer = operation->dest;
        int randLimit = count;

        for(; count>0; count--){
		loc = startLine + (rand() % randLimit);  
                fseek(fp, loc, SEEK_SET);
                fwrite(writeBuffer, sizeof(char), blockSize, fp);
        }
	fclose(fp);

        return NULL;
}

/** 
	Calculate throughput
	blockSize: Size of block to read/write in one go
	loopTime: Number of time each operation is to be performed 
	latency: 1 if calculating latency else 0

**/
int throughput(long int blockSize, int loopTime, int latency, int threadCount){
	// Allocate variables
	pthread_t tidSeqRead[threadCount], tidSeqWrite[threadCount], tidRandRead[threadCount], tidRandWrite[threadCount];
	int indx = 0, counter = 0;
	OperationInfo sendData[threadCount];
	char *destination[threadCount*5];
	struct timeval startTime, endTime;
	double currentTimeElapsed = 0; // time elapsed in one operation
	double  totalTimeElapsed = 0; // total time elapsed
	double currentByteCopied = 0; // Byte used in current operation
	double totalByteCopied = 0; // Byte used in all operation
	double mbCopied = 0;

	// Create destination array where the data is read or written
	for(indx = 0; indx < threadCount*4; indx++){
		destination[indx] = getArray(blockSize);
	}

	currentByteCopied = 0;
	gettimeofday(&startTime, NULL);
	for(counter = 0; counter < loopTime; counter++){
		for(indx = 0; indx < threadCount; indx++){
			sendData[indx].startSeq = indx;
			sendData[indx].blockSize = blockSize;

			// If not latency then calculate the time data is copied
			if(!latency){
				sendData[indx].totalTimes = ACCESS_SIZE/(blockSize*threadCount);
			}else{
				sendData[indx].totalTimes = 1;
			}

			sendData[indx].dest = destination[indx];

			currentByteCopied += sendData[indx].totalTimes * blockSize; 
			pthread_create(&tidSeqRead[indx], NULL, sequential_read, (void *)(sendData + indx));
		}

		for(indx = 0; indx < threadCount; indx++){
			pthread_join(tidSeqRead[indx], NULL);
		}
	}
	gettimeofday(&endTime, NULL);
	
	for(indx = 0; indx < threadCount; indx++){
		free(destination[indx]);
	}


	totalByteCopied += currentByteCopied;

	// calculate time
	currentTimeElapsed = ((double)(endTime.tv_sec*1000000 - startTime.tv_sec * 1000000 + endTime.tv_usec - startTime.tv_usec))/1000000;
	totalTimeElapsed += currentTimeElapsed; 

	mbCopied = currentByteCopied/1000000.0;
	if(!latency){
		printf("Sequential read:\n\t\t %lf MB in time:%lf", mbCopied, currentTimeElapsed);
		printf("\n\t\t Throughput %lf MBps", mbCopied/currentTimeElapsed);
		fflush(stdout);
	}

	currentByteCopied = 0;
	gettimeofday(&startTime, NULL);
	for(counter = 0; counter < loopTime; counter++){
		for(indx = 0; indx < threadCount; indx++){
			sendData[indx].startSeq = indx;
			sendData[indx].blockSize = blockSize;
		
			// If not latency then calculate the time data is copied
			if(!latency){
				sendData[indx].totalTimes = ACCESS_SIZE/(blockSize*threadCount);
			}else{
				sendData[indx].totalTimes = 1;
			}
			sendData[indx].dest = destination[indx+threadCount];

			currentByteCopied += sendData[indx].totalTimes * blockSize;
			pthread_create(&tidSeqWrite[indx], NULL, sequential_write, (void *)(sendData + indx));
		}

		for(indx = 0; indx < threadCount; indx++){
			pthread_join(tidSeqWrite[indx], NULL);
		}
	}
	gettimeofday(&endTime, NULL);

	for(indx = 0; indx < threadCount; indx++){
                free(destination[indx+threadCount]);
        }
	totalByteCopied += currentByteCopied;

	// calculate time
	currentTimeElapsed = ((double)(endTime.tv_sec*1000000 - startTime.tv_sec * 1000000 + endTime.tv_usec - startTime.tv_usec))/1000000;
	totalTimeElapsed += currentTimeElapsed;

	mbCopied = currentByteCopied/1000000.0;
	if(!latency){
		printf("\nSequential write:\n\t\t %lf MB in time:%lf", mbCopied, currentTimeElapsed);
		printf("\n\t\t Throughput %lf MBps", mbCopied/currentTimeElapsed);
		fflush(stdout);
	}

	currentByteCopied = 0;
	gettimeofday(&startTime, NULL);
	for(counter = 0; counter < loopTime; counter++){
		for(indx = 0; indx < threadCount; indx++){
			sendData[indx].startSeq = indx;
			sendData[indx].blockSize = blockSize;
		
			// If not latency then calculate the time data is copied
			if(!latency){
				sendData[indx].totalTimes = ACCESS_SIZE/(blockSize*threadCount);
			}else{
				sendData[indx].totalTimes = 1;
			}
			sendData[indx].dest = destination[indx+(threadCount*2)];

			currentByteCopied += sendData[indx].totalTimes * blockSize;
			pthread_create(&tidRandWrite[indx], NULL, random_read, (void *)(sendData + indx));
		}

		for(indx = 0; indx < threadCount; indx++){
			pthread_join(tidRandWrite[indx], NULL);
		}
	}
	gettimeofday(&endTime, NULL);
	
	for(indx = 0; indx < threadCount; indx++){
                free(destination[indx+(2*threadCount)]);
        }

	totalByteCopied += currentByteCopied;

	// calculate time
	currentTimeElapsed = ((double)(endTime.tv_sec*1000000 - startTime.tv_sec * 1000000 + endTime.tv_usec - startTime.tv_usec))/1000000;
	totalTimeElapsed += currentTimeElapsed;

	mbCopied = currentByteCopied/1000000.0;
	if(!latency){
		printf("\nRandom Read:\n\t\t %lf MB in time:%lf", mbCopied, currentTimeElapsed);
		printf("\n\t\t Throughput %lf MBps", mbCopied/currentTimeElapsed);
		fflush(stdout);
	}

	currentByteCopied = 0;
	gettimeofday(&startTime, NULL);
	for(counter = 0; counter < loopTime; counter++){
		for(indx = 0; indx < threadCount; indx++){
			sendData[indx].startSeq = indx;
			sendData[indx].blockSize = blockSize;
		
			// If not latency then calculate the time data is copied
			if(!latency){
				sendData[indx].totalTimes = ACCESS_SIZE/(blockSize*threadCount);
			}else{
				sendData[indx].totalTimes = 1;
			}
			sendData[indx].dest = destination[indx+threadCount*3];

			currentByteCopied += sendData[indx].totalTimes * blockSize;
			pthread_create(&tidRandRead[indx], NULL, random_write, (void *)(sendData + indx));
		}

		for(indx = 0; indx < threadCount; indx++){
			pthread_join(tidRandRead[indx], NULL);
		}
	}
	gettimeofday(&endTime, NULL);
	
	for(indx = 0; indx < threadCount; indx++){
                free(destination[indx+(3*threadCount)]);
        }


	totalByteCopied += currentByteCopied;

	// calculate time
	currentTimeElapsed = ((double)(endTime.tv_sec*1000000 - startTime.tv_sec * 1000000 + endTime.tv_usec - startTime.tv_usec))/1000000;
	totalTimeElapsed += currentTimeElapsed;

	mbCopied = currentByteCopied/1000000.0;
	if(!latency){
		printf("\nRandom write:\n\t\t %lf MB in time:%lf", mbCopied, currentTimeElapsed);
		printf("\n\t\t Throughput %lf MBps", mbCopied/currentTimeElapsed);
		fflush(stdout);
	}

	mbCopied = totalByteCopied/1000000.0;
	if(!latency){
		printf("\nAverage:\n\t\t %lf MB in time:%lf", mbCopied, totalTimeElapsed);
		printf("\n\t\t Throughput %lf MBps", mbCopied/totalTimeElapsed);
		fflush(stdout);
	}else{
		printf("\n\nLatency: %lf\n", totalTimeElapsed);
		fflush(stdout);
	}

	return 0;
}

void usage(){
	printf("Proper usage as follows:\n");
	printf("./memory loop_time\n");
	printf("loop_time: No of times to repeat operation. Control the duration for which code executes\n");
	fflush(stdout);	
	return;
}

void disk_benchmark(long int blockSize, int loop){
	printf("With 1 thread\n");
	throughput(blockSize, loop, 0, 1);
        throughput(8, loop, 1, 1);

	printf("\n\n\nWith 2 thread\n");
        throughput(blockSize, loop, 0, 2);
        throughput(8, loop, 1, 2);

	printf("\n\n\nWith 4 thread\n");
        throughput(blockSize, loop, 0, 4);
        throughput(8, loop, 1, 4);

	printf("\n\n\nWith 8 thread\n");
        throughput(blockSize, loop, 0, 8);
        throughput(8, loop, 1, 8);
}

int main(int argc, char *argv[]){
	long int blockSize;
	int loop = 0;
	
	if(argc != 2){
		usage();
		return 0;
	}

	loop = atoi(argv[1]);

	createFile();

	blockSize = 8;
	printf("\n\n\n\nPerforming with 8B blocksize\n\n");
	disk_benchmark(blockSize, loop);

	blockSize = 8000;
	printf("\n\n\n\nPerforming with 8KB blocksize\n\n");
	disk_benchmark(blockSize, loop);

	blockSize = 8000000;
        printf("\n\n\n\nPerforming with 8MB blocksize\n\n");
        disk_benchmark(blockSize, loop);


	blockSize = 80000000;
        printf("\n\n\n\nPerforming with 80MB blocksize\n\n");
        disk_benchmark(blockSize, loop);

	return 0;
}



