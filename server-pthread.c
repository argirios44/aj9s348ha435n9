/* A simple server in the internet domain using TCP
   The port number is passed as an argument 
   
   Rensselaer Polytechnic Institute (RPI)
*/

#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

void *connection(void* socket);
void error(char *msg)
{
  perror(msg);
  exit(1);
}

int main(int argc, char *argv[])
{
  int sockfd, newsockfd, portno, clilen;
  char buffer[256];
  struct sockaddr_in serv_addr, cli_addr;
  int n;
pthread_t thread_id;
  if (argc < 2) {
    fprintf(stderr,"ERROR, no port provided\n");
    exit(1);
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
  listen(sockfd,5);
  clilen = sizeof(cli_addr);
  while(newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)){
	if (pthread_create(&thread_id,NULL,connection,(void*)&newsockfd)!=0){
		printf("ERROR creating thread");
		return 1;	
	}

	}
  if (newsockfd < 0) error("ERROR on accept");
  return 0; 
}


void *connection(void *socket){
	char buffer[256];
	int n;
	int socket_in=*(int*)socket;
	bzero(buffer,256);
	n = read(socket_in,buffer,255);
 	if (n < 0) error("ERROR reading from socket");
  	printf("Here is the message: %s\n",buffer);
  	n = write(socket_in,"I got your message",18);
  	if (n < 0) error("ERROR writing to socket");
  	return 0; 
}
