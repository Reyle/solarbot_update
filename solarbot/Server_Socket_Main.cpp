#include <iostream>
#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include <arpa/inet.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include <sys/types.h>
#include <sys/wait.h>


char client_message[2000];
char buffer[1024];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void* socketThread_k2(void* arg)
{
	//int newSocket = *((int*)arg);
	//recv(newSocket, client_message, 2000, 0);
	//// Send message to the client socket 
	//pthread_mutex_lock(&lock);
	//char* message = (char*)malloc(sizeof(client_message) + 20);
	//strcpy(message, "Hello Client : ");
	//strcat(message, client_message);
	//strcat(message, "\n");
	//strcpy(buffer, message);
	//free(message);
	//pthread_mutex_unlock(&lock);
	//sleep(1);
	//send(newSocket, buffer, 13, 0);
	//printf("Exit socketThread \n");
	//close(newSocket);
	//pthread_exit(NULL);

	int newSocket = *((int*)arg);
	char* buffer = "Merda OK?\n";
	while (true)
	{
		try
		{
			send(newSocket, buffer, 10, 0);
		}
		catch (...) {}
		sleep(1);
	}
	printf("Exit socketThread \n");
	close(newSocket);
	pthread_exit(NULL);
}

int main_k2() {
	int serverSocket, newSocket;
	struct sockaddr_in serverAddr;
	struct sockaddr_storage serverStorage;
	socklen_t addr_size;
	int client_index = 0;
	//pid_t pid[50];
	//Create the socket. 
	serverSocket = socket(PF_INET, SOCK_STREAM, 0);
	// Configure settings of the server address struct
	// Address family = Internet 
	serverAddr.sin_family = AF_INET;
	//Set port number, using htons function to use proper byte order 
	serverAddr.sin_port = htons(2010);
	//Set IP address to localhost 
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	//Set all bits of the padding field to 0 
	memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);
	//Bind the address struct to the socket 
	bind(serverSocket, (struct sockaddr*) & serverAddr, sizeof(serverAddr));
	//Listen on the socket, with 5 max connection requests queued 
	if (listen(serverSocket, 5) == 0)
		printf("Listening\n");
	else
		printf("Error\n");

	pthread_t tid[60];
	int i = 0;
	while (1)
	{
		if (client_index < 5)
		{
			/*---- Accept call creates a new socket for the incoming connection ----*/
			addr_size = sizeof serverStorage;
			newSocket = accept(serverSocket, (struct sockaddr*) & serverStorage, &addr_size);

			//for each client request creates a thread and assign the client request to it to process
		   //so the main thread can entertain next request
			if (pthread_create(&tid[client_index], NULL, socketThread_k2, &newSocket) != 0)
				printf("Failed to create thread\n");

			client_index++;
		}

		//if (i >= 5)
		//{
		//	i = 0;
		//	while (i < 5)
		//	{
		//		pthread_join(tid[i++], NULL);
		//	}
		//	i = 0;
		//}
	}
	return 0;
}