#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>

// Sequence of operatin
typedef struct{
	double min;
	double max;
}Sequence;

void *iops_thread(void *args){
	double min, max, indx;
	Sequence *seq = (Sequence *)args;
	min = seq->min;
	max = seq->max;
	
		for(indx = min; indx < max; indx++)
		{
			indx*indx;		
			indx+indx;
			indx-34;
			indx/2;
		}
	
	return NULL;
}

void *flops_thread(void *args){
        double min, max;
	double i;
        Sequence *seq = (Sequence *)args;
        min = seq->min;
        max = seq->max;

		for(i = min; i < max; i = i + 1.0)
		{
			i*i;
			i+i;
			i-34;
			i/2.0;
		}

        return NULL;
}

void usage(){
        printf("Proper usage as follows:\n");
        printf("./memory total_operation statistical_mode\n");
        printf("total_operation: No of times to repeat operation. Must be multiple of thread count\n");
	printf("statistical_mode: 0 - Normal Mode, 1: Create statistical data\n");
        fflush(stdout);
        return;
}

int perform_benchmark(double totalOperation, int threadCount, int isStatistical, int turn){
	struct timeval startTime, endTime;
	pthread_t tid[threadCount];
	int indx = 0;
	double operationPerThread = 0;
	Sequence sendData[threadCount];
	double currentTimeElapsed;

	operationPerThread = floor(totalOperation/threadCount);

	if(!isStatistical || (isStatistical && turn == 0)){
		// create thread
		gettimeofday(&startTime, NULL);
		for(indx = 0; indx < threadCount; indx++)
		{	

			sendData[indx].min = indx*operationPerThread + 1;
			sendData[indx].max = sendData[indx].min + operationPerThread - 1;   	

			pthread_create(&tid[indx], NULL, iops_thread, (void *)(sendData + indx));
		}

		// wait till all thread finish
		for(indx = 0; indx < threadCount; indx++)
		{
			pthread_join(tid[indx], NULL);
		}
		gettimeofday(&endTime, NULL);
		currentTimeElapsed = ((double)(endTime.tv_sec*1000000 - startTime.tv_sec * 1000000 + endTime.tv_usec - startTime.tv_usec))/1000000;

		if(!isStatistical){
			printf("\nIOPS perfomance:\n\t\tTotal Time Elapsed: %lf\n", currentTimeElapsed);
			printf("\t\tTotal operations: %lf\n", 4*totalOperation);
			printf("\t\tIOPS: %lf Giga IOPS", (4.0*totalOperation)/(currentTimeElapsed * 100000000));
		}else{
			printf("%lf,%lf,%lf\n", 4*totalOperation, currentTimeElapsed, (4.0*totalOperation)/(currentTimeElapsed * 100000000));
		}
	}

	if(!isStatistical || (isStatistical && turn == 1)){
		gettimeofday(&startTime, NULL);
		for(indx = 0; indx < threadCount; indx++)
		{

			sendData[indx].min = indx*operationPerThread + 1;
			sendData[indx].max = sendData[indx].min + operationPerThread - 1;

			pthread_create(&tid[indx], NULL, flops_thread, (void *)(sendData + indx));
		}

		// wait till all thread finish
		for(indx = 0; indx < threadCount; indx++)
		{
			pthread_join(tid[indx], NULL);
		}
		gettimeofday(&endTime, NULL);
		currentTimeElapsed = ((double)(endTime.tv_sec*1000000 - startTime.tv_sec * 1000000 + endTime.tv_usec - startTime.tv_usec))/1000000;

		if(!isStatistical){
			printf("\nFLOPS perfomance:\n\t\tTotal Time Elapsed: %lf\n", currentTimeElapsed);
			printf("\t\tTotal operations: %f\n", 4*totalOperation);
			printf("\t\tFLOPS: %lf Giga FLOPS\n", (4.0*totalOperation)/(currentTimeElapsed*1000000000));
		}else{
			printf("%f,%lf,%lf\n", 4*totalOperation, currentTimeElapsed, (4.0*totalOperation)/(currentTimeElapsed * 100000000));
		}
	}
	return 0;
}

int main(int argc, char *argv[]){
        double totalOperation = 0.0, helper = 0.0;
	int isStatistical = 0;

        if(argc < 3){
                usage();
                return 0;
        }

	isStatistical = atoi(argv[2]);
	
	if(isStatistical != 0 && isStatistical != 1){
		printf("Invalid statistical value: %d\n", isStatistical);
		usage();
                return 0;
	}

        totalOperation = atof(argv[1]);

	if(!isStatistical){
		printf("Total operation:%f\n", totalOperation);

		printf("Performing With 1 Thread\n");		
		perform_benchmark(totalOperation, 1, 0, 0);

		printf("\n\n\nPerforming With 2 Thread\n");           
		perform_benchmark(totalOperation, 2, 0, 0);

		printf("\n\n\nPerforming With 4 Thread\n");
		perform_benchmark(totalOperation, 4, 0, 0);

		printf("\n\n\nPerforming With 8 Thread\n");
		perform_benchmark(totalOperation, 8, 0, 0);
	}else{
		printf("Generating IOPS data\n");
		printf("Total Operation, Duration, Giga IOPS\n");
		totalOperation = 2000000000;
		helper = totalOperation;
		while(totalOperation < helper*600){
			totalOperation += helper;
			perform_benchmark(totalOperation, 8, 1, 0);
		}

		printf("Generating FLOPS data\n");
                printf("Total Operation, Duration, Giga FLOPS\n");
                totalOperation = 2000000000;
                helper = totalOperation;
                while(totalOperation < helper*600){
                        totalOperation += helper;
                        perform_benchmark(totalOperation, 8, 1, 1);
                }
		
	}
	return 0;
}

