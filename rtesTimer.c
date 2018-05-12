//Gklaras Argyrios 7358 rtesTimer
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>


void do_nothing(){}


long *timestamp;
struct  timeval *tim;

void *get_timestamp(void *sample_number){
	gettimeofday(&tim[(long)sample_number],NULL);
	long sample=(long)sample_number;
	gettimeofday(&tim[sample],NULL);
	timestamp[sample]=tim[sample].tv_sec*1000000+tim[sample].tv_usec;	
	pthread_exit(NULL);
}

int main(int argc,char **argv){
	int sample_number,sample_time;
	long i;
	FILE *FL;
	if (argc!=3){
		printf("ERROR: Wrong number of inputs. Please enter  number of samples and  sampling range.");
		exit(1);
	}
	timestamp=malloc(sizeof(long)*sample_number);
	tim=malloc(sizeof(struct  timeval)*sample_number);
	sample_number=atoi(argv[1]);
	pthread_t tid[sample_number];
	sample_time=atoi(argv[2]);
	double period=(double)1000000*sample_time/sample_number; 
	int period_usec=(int)period;//Περίοδος δειγματοληψείας
	printf("%f %d\n",period,period_usec);
	//signal(SIGALRM,do_nothing);
	for (i=0;i<sample_number;i++){
		//ualarm(period_usec,0);
		pthread_create(&tid[i],NULL,get_timestamp,(void*) i);
		usleep(period_usec);
	}
	for(i=0;i<sample_number;i++){
		pthread_join(tid[i],NULL);
	}
	FL=fopen("timestamps.bin","w+");
	fwrite(&timestamp[0],sizeof(long),sample_number,FL);
	fclose(FL);
	pthread_exit(NULL);
}

