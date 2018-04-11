#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <memory.h>
#include <string.h>

#define ARRAY_SIZE 1000000000

// Array acting as source of operations
char *source;

// structure to send information to threads
typedef struct{
	long int startSeq;
	long int blockSize;
	long int totalTimes; 
	char *dest;
}OperationInfo;


//allocate memory to source array
void createArray(){
	long int totalBlock = ARRAY_SIZE/sizeof(char);
	int indx = 0;
	source = (char *)calloc(totalBlock, sizeof(char));

	// initalise array
	for(indx = 0; indx < totalBlock; indx++){
		source[indx] = 'a';
	}
	source[indx] = '\0';	

	printf("Memory of %d Bytes allocated\n", strlen(source)/sizeof(char));
	fflush(stdout);
}

// Thread to read sequencially
void *sequential_read(void *args){
	// read arguments
	OperationInfo *operation = (OperationInfo *)args; 
	int count = operation->totalTimes;
	char *src = source + operation->startSeq;
	char *dest = operation->dest;
	long int blockSize = operation->blockSize;

	for(; count>0; count--){
		memcpy(dest, src, blockSize);	
		// relocate src pointer
		src = src + blockSize;
	}	

	return NULL;
}

// Thread to write sequencially
void *sequential_write(void *args){
	// read arguments
	OperationInfo *operation = (OperationInfo *)args;
        int count = operation->totalTimes;
        char *src = source + operation->startSeq;
        char *dest = operation->dest;
        long int blockSize = operation->blockSize;

        for(; count>0; count--){
		strncpy(dest, src, blockSize);
		// relocate src pointer
		src = src + blockSize;
        }

	return NULL;
}

// Thread to read randomly
void *random_read(void *args){
	// read arguments
	int loc;
	srand(time(0));
	OperationInfo *operation = (OperationInfo *)args;
        int count = operation->totalTimes;
        char *src = source;
        char *dest = operation->dest;
        long int blockSize = operation->blockSize;

        for(; count>0; count--){
		loc = rand() % (ARRAY_SIZE - blockSize - 1);
		// relocate src pointer to the location chosen randomly
		src = source + loc;
                memcpy(dest, src, blockSize);
        }

        return NULL;

}

// Thread to write randomly
void *random_write(void *args){
	// read arguments
	int loc;
	srand(time(0));
	OperationInfo *operation = (OperationInfo *)args;
        int count = operation->totalTimes;
        char *src = source + operation->startSeq;
        char *dest = operation->dest;
        long int blockSize = operation->blockSize;

        for(; count>0; count--){
		loc = rand() % (ARRAY_SIZE - blockSize - 1);
		// relocate src pointer to the location chosen randomly
                src = source + loc;
                strncpy(dest, src, blockSize);
        }

        return NULL;
}


//check if block size entered is valid and return equivalent value in BYTE
int valid_blocksize(char *blockSize){
	if(strcmp(blockSize, "8K") == 0){
		return 8000;
	}else if(strcmp(blockSize, "8M") == 0){
                return 8000000;
        }else if(strcmp(blockSize, "80M") == 0){
		return 80000000;
	}
	return 0;
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
	for(indx = 0; indx < threadCount*5; indx++){
		destination[indx] = (char *)calloc(blockSize, sizeof(char));
	}

	currentByteCopied = 0;
	gettimeofday(&startTime, NULL);
	for(counter = 0; counter < loopTime; counter++){
		for(indx = 0; indx < threadCount; indx++){
			sendData[indx].startSeq = indx;
			sendData[indx].blockSize = blockSize;

			// If not latency then calculate the time data is copied
			if(!latency){
				sendData[indx].totalTimes = ARRAY_SIZE/(blockSize*threadCount);
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
				sendData[indx].totalTimes = ARRAY_SIZE/(blockSize*threadCount);
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
				sendData[indx].totalTimes = ARRAY_SIZE/(blockSize*threadCount);
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
				sendData[indx].totalTimes = ARRAY_SIZE/(blockSize*threadCount);
			}else{
				sendData[indx].totalTimes = 1;
			}
			sendData[indx].dest = destination[indx+threadCount];

			currentByteCopied += sendData[indx].totalTimes * blockSize;
			pthread_create(&tidRandRead[indx], NULL, random_write, (void *)(sendData + indx));
		}

		for(indx = 0; indx < threadCount; indx++){
			pthread_join(tidRandRead[indx], NULL);
		}
	}
	gettimeofday(&endTime, NULL);
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

	for(indx = 0; indx < threadCount*5; indx++){
                free(destination[indx]);
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

void memory_benchmark(long int blockSize, int loop){
	printf("Performing with 1 threead\n");
	throughput(blockSize, loop, 0, 1);
        throughput(8, loop, 1, 1);

	printf("\n\n\nPerforming with 2 threead\n");
        throughput(blockSize, loop, 0, 2);
        throughput(8, loop, 1, 2);

	printf("\n\n\nPerforming with 4 threead\n");
        throughput(blockSize, loop, 0, 4);
        throughput(8, loop, 1, 4);

	printf("\n\n\nPerforming with 8 threead\n");
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

	/**blockSize = valid_blocksize(argv[1]); 

	if(!blockSize){
		usage();
		return 0;
	}**/

	loop = atoi(argv[1]);

	createArray();
	blockSize = 8000;
	printf("\n\n\n\nPerforming with 8KB blocksize\n\n");
	memory_benchmark(blockSize, loop);

	blockSize = 8000000;
        printf("\n\n\n\nPerforming with 8MB blocksize\n\n");
        memory_benchmark(blockSize, loop);


	blockSize = 80000000;
        printf("\n\n\n\nPerforming with 80MB blocksize\n\n");
        memory_benchmark(blockSize, loop);



	free(source);

	return 0;
}



