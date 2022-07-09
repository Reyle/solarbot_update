///* Creates a datagram server.  The port
//   number is passed as an argument.  This
//   server runs forever */
//
//#include <sys/types.h>
//#include <stdlib.h>
//#include <unistd.h>
//#include <sys/socket.h>
//#include <netinet/in.h>
//#include <string.h>
//#include <netdb.h>
//#include <stdio.h>
//
//void error(const char* msg)
//{
//	perror(msg);
//	exit(0);
//}
//
//int main(int argc, char* argv[])
//{
//	int sock, length, n;
//	socklen_t fromlen;
//	struct sockaddr_in server;
//	struct sockaddr_in from;
//	char buf[1024];
//
//	if (argc < 2) {
//		fprintf(stderr, "ERROR, no port provided\n");
//		exit(0);
//	}
//
//	sock = socket(AF_INET, SOCK_DGRAM, 0);
//	if (sock < 0) 
//		error("Opening socket");
//
//	length = sizeof(server);
//	bzero(&server, length);
//	server.sin_family = AF_INET;
//	server.sin_addr.s_addr = INADDR_ANY;
//	server.sin_port = htons(atoi(argv[1]));
//
//	if (bind(sock, (struct sockaddr*) & server, length) < 0)
//		error("binding");
//
//	fromlen = sizeof(struct sockaddr_in);
//	while (1) {
//		n = recvfrom(sock, buf, 1024, 0, (struct sockaddr*) & from, &fromlen);
//		if (n < 0) 
//			error("recvfrom");
//
//		write(1, "Received a datagram: ", 21);
//		write(1, buf, n);
//
//		//n = sendto(sock, "Got your message\n", 17, 0, (struct sockaddr*) & from, fromlen);
//		//if (n < 0) 
//		//	error("sendto");
//
//		while (1)
//		{
//			n = sendto(sock, "Got your message\n", 17, 0, (struct sockaddr*) & from, fromlen);
//			if (n < 0)
//				error("sendto");
//			usleep(250000);
//		}
//	}
//	return 0;
//}


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#include "SocketException.h"

void error(const char* msg)
{
	perror(msg);
	exit(0);
}

int main_bk2(int argc, char* argv[])
{
	int sockfd, portno, n;
	struct sockaddr_in serv_addr;
	struct hostent* server;

	char buffer[256];
	if (argc < 3) {
		fprintf(stderr, "usage %s hostname port\n", argv[0]);
		exit(0);
	}
	portno = atoi(argv[2]);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		error("ERROR opening socket");

	server = gethostbyname(argv[1]);
	if (server == NULL) {
		fprintf(stderr, "ERROR, no such host\n");
		exit(0);
	}
	bzero((char*)& serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char*)server->h_addr, (char*)& serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);
	if (connect(sockfd, (struct sockaddr*) & serv_addr, sizeof(serv_addr)) < 0)
		error("ERROR connecting");
	printf("Please enter the message: ");
	bzero(buffer, 256);
	fgets(buffer, 255, stdin);
	
	try
	{
		while (1)
		{
			n = write(sockfd, buffer, strlen(buffer));
			if (n < 0)
				error("ERROR writing to socket");

			//bzero(buffer, 256);
			//n = read(sockfd, buffer, 255);
			//if (n < 0)
			//	error("ERROR reading from socket");
			//else
			//	printf("%s\n", buffer);

			usleep(250000);
		}
	}
	catch(SocketException&){}

	printf("\nHasta luego\n");
	close(sockfd);
	return 0;
}
