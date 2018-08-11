//Simple message server, allowing 20 clients connected at the same time and 50 in queue//
#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/sysinfo.h>

struct data{
	char SENDER_ADDR[20];
	char RCVER_ADDR[20];
	char MESSAGE[256];
};
struct timeval tim;
long timestamp[2];
 struct data *MEM;
 int size=0;
 struct sysinfo info;
pthread_mutex_t mutex,lock;
pthread_cond_t cond;
int num_threads= 0;
void *connection(void* socket);
void del_entry_and_realloc(int entry_number);
void error(char *msg)
{
  perror(msg);
  exit(1);
}

int main(int argc, char *argv[])
{
	int sockfd, newsockfd, portno, clilen;
	char buffer[300];
	struct sockaddr_in serv_addr, cli_addr;
	int n,i;
	MEM=(struct data*)malloc(sizeof(struct data));
	pthread_t thread_id;
	if (argc < 2) {
		fprintf(stderr,"ERROR, no port provided\n");
		exit(1);
	}
	if (pthread_mutex_init(&mutex,NULL)!=0){
		error("ERROR initializing mutex.");
		return 1;
	}
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
    error("ERROR opening socket");
	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	if (bind(sockfd, (struct sockaddr *) &serv_addr,
           sizeof(serv_addr)) < 0) 
    error("ERROR on binding");
	listen(sockfd,50);//50 socket queue available
	clilen = sizeof(cli_addr);
	while(1){
		if (num_threads>=20){
			pthread_mutex_lock(&lock);
			pthread_cond_wait(&cond,&lock);
			pthread_mutex_unlock(&lock);
		}
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (pthread_create(&thread_id,NULL,connection,(void*)&newsockfd)!=0){
			printf("ERROR creating thread");
			return 1;	
		}
		else{
			pthread_mutex_lock(&lock);
			num_threads++;
			pthread_mutex_unlock(&lock);
		}
	}
	if (newsockfd < 0) error("ERROR on accept");
	return 0; 
}

//********FUNCTIONS***********//
void *connection(void *socket){
	char buffer[303],*token;
	char *dilim="##";
	int n,curr,i,tosend;
	int socket_in=*(int*)socket;
	bzero(buffer,303);
	n = read(socket_in,buffer,303);
	gettimeofday(&tim,NULL);
	timestamp[0]=tim.tv_sec*1000000+tim.tv_usec;	
 	if (n < 0) error("ERROR reading from socket");
	//realloc MEMORY//+mutex
	pthread_mutex_lock(&mutex);
	size++;
	curr=size-1;
	MEM=(struct data*)realloc(MEM,(size)* sizeof(struct data));
	pthread_mutex_unlock(&mutex);
	//split and save//
	token=strtok(buffer,dilim);
	strcpy(MEM[curr].SENDER_ADDR,token);
	token=strtok(NULL,dilim);
	strcpy(MEM[curr].RCVER_ADDR,token);

	//check if receive only
	char current_rcv[20];
	strcpy(current_rcv,MEM[curr].RCVER_ADDR);
	if(strcmp(current_rcv,"recv")==0){
		strcpy(current_rcv,MEM[curr].SENDER_ADDR);
		pthread_mutex_lock(&mutex);
		size--;
		MEM=(struct data*)realloc(MEM,(size)*sizeof(struct data));
		pthread_mutex_unlock(&mutex);
	}
	else{
		strcpy(current_rcv,MEM[curr].SENDER_ADDR);
		token=strtok(NULL,dilim);
		strcpy(MEM[curr].MESSAGE,token);
	}
	//search for receive
	pthread_mutex_lock(&mutex);
	bzero(buffer,303);
	int found=0;
	i=0;
	while(i<=size){
		if (strcmp(current_rcv,MEM[i].RCVER_ADDR)==0&&i!=size&&strlen(buffer)<250){
			
			strcat(buffer,"Message from:: ");
			strcat(buffer,MEM[i].SENDER_ADDR);
			strcat(buffer," :: ");
			strcat(buffer,MEM[i].MESSAGE);
			strcat(buffer,"\n");
			del_entry_and_realloc(i);
			found=1;
			i--;
		}
		else if((i==size-1&&found==0)||(size==0&&found==0)) {
			strcat(buffer,"Done. You have no new messages.");
		}
		i++;
	}
	pthread_mutex_unlock(&mutex);
	/*for (i=0;i<size;i++){
		printf("%s %s %s\n",MEM[i].SENDER_ADDR,MEM[i].RCVER_ADDR,MEM[i].MESSAGE);
	}*/
  	printf(" num threads=%d\n",num_threads);
	printf("\nsize=%d\n",size);
	gettimeofday(&tim,NULL);
	timestamp[1]=tim.tv_sec*1000000+tim.tv_usec;	
	n = write(socket_in,buffer,strlen(buffer));
  	if (n < 0) error("ERROR writing to socket");
	pthread_mutex_lock(&lock);
	num_threads--;
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&lock);
	//printf("\ntime1=%ld  time2=%ld uptime=%ld\n",timestamp[0],timestamp[1],info.uptime);
  	return 0;
}


void del_entry_and_realloc(int entry_number){
	int i;
	for (i=entry_number;i<size-1;i++){
		strcpy(MEM[i].SENDER_ADDR,MEM[i+1].SENDER_ADDR);
		strcpy(MEM[i].RCVER_ADDR,MEM[i+1].RCVER_ADDR);
		strcpy(MEM[i].MESSAGE,MEM[i+1].MESSAGE);
	}
	size--;
	MEM=(struct data*)realloc(MEM,(size)*sizeof(struct data));
}
