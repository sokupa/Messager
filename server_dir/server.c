#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>


int parseARGS(char **args, char *line){

int client(void *ptr){
	int  connectSOCKET;
	connectSOCKET = (int ) ptr;
	char recvBUFF[4096];
	char *header[4096];

	while(1){
				printf("Filename: %s\n", filename);
		}
		recvBUFF[0] = 0;
				printf("Done: %s [ %d of %s bytes]\n", filename, received , filesize);
}


	int socketINDEX = 0;
	int listenSOCKET, connectSOCKET[512];
	pthread_t threads[512];
	while(1){
		connectSOCKET[socketINDEX] = accept(listenSOCKET, (struct sockaddr *) &clientADDRESS[socketINDEX], &clientADDRESSLENGTH[socketINDEX]);

		pthread_create( &threads[socketINDEX], NULL, client, connectSOCKET[socketINDEX]);
		if(socketINDEX=512) {
			socketINDEX = 0;
		} else { 
		socketINDEX++;
		}
	}
	close(listenSOCKET);
}