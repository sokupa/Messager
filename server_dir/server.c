#include <arpa/inet.h>
#include <netdb.h>#include <netinet/in.h>#include <string.h>#include <stdio.h>
#include <pthread.h>
#define PORT 31337

int parseARGS(char **args, char *line){	int tmp=0;	args[tmp] = strtok( line, ":" );	while ( (args[++tmp] = strtok(NULL, ":" ) ) != NULL );	return tmp - 1;}

int client(void *ptr){
	int  connectSOCKET;
	connectSOCKET = (int ) ptr;
	char recvBUFF[4096];	char *filename, *filesize;	FILE * recvFILE;	int received = 0;	char tempstr[4096];
	char *header[4096];

	while(1){		if( recv(connectSOCKET, recvBUFF, sizeof(recvBUFF), 0) ){			if(!strncmp(recvBUFF,"FBEGIN",6)) {				recvBUFF[strlen(recvBUFF) - 2] = 0;				parseARGS(header, recvBUFF);				filename = header[1];				filesize = header[2];
				printf("Filename: %s\n", filename);				printf("Filesize: %d Kb\n", atoi(filesize) / 1024);
		}
		recvBUFF[0] = 0;		recvFILE = fopen ( filename,"w" );		while(1){			if( recv(connectSOCKET, recvBUFF, 1, 0) != 0 ) {				fwrite (recvBUFF , sizeof(recvBUFF[0]) , 1 , recvFILE );				received++;				recvBUFF[0] = 0;				} else {
				printf("Done: %s [ %d of %s bytes]\n", filename, received , filesize);				return 0;				}			}			return 0;		} else {		printf("Client dropped connection\n");		}	return 0;	}
}

int main(){
	int socketINDEX = 0;
	int listenSOCKET, connectSOCKET[512];	socklen_t clientADDRESSLENGTH[512];	struct sockaddr_in clientADDRESS[512], serverADDRESS;
	pthread_t threads[512];	listenSOCKET = socket(AF_INET, SOCK_STREAM, 0);	if (listenSOCKET < 0) {		printf("Cannot create socket\n");		close(listenSOCKET);		return 1;	}	serverADDRESS.sin_family = AF_INET;	serverADDRESS.sin_addr.s_addr = htonl(INADDR_ANY);	serverADDRESS.sin_port = htons(PORT);	if (bind(listenSOCKET, (struct sockaddr *) &serverADDRESS, sizeof(serverADDRESS)) < 0) {		printf("Cannot bind socket\n");		close(listenSOCKET);		return 1;	}	listen(listenSOCKET, 5);	clientADDRESSLENGTH[socketINDEX] = sizeof(clientADDRESS[socketINDEX]);
	while(1){
		connectSOCKET[socketINDEX] = accept(listenSOCKET, (struct sockaddr *) &clientADDRESS[socketINDEX], &clientADDRESSLENGTH[socketINDEX]);		if (connectSOCKET[socketINDEX] < 0) {			printf("Cannot accept connection\n");			close(listenSOCKET);			return 1;		}

		pthread_create( &threads[socketINDEX], NULL, client, connectSOCKET[socketINDEX]);
		if(socketINDEX=512) {
			socketINDEX = 0;
		} else { 
		socketINDEX++;
		}
	}
	close(listenSOCKET);
}