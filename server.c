#include <arpa/inet.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>

#define BUF_SIZE 500

int main(int argc, char *argv[])
{
	struct addrinfo hints;
	struct addrinfo *result;
	int portindex;
	int sfd;
	int s;
	struct sockaddr_storage peer_addr;
	socklen_t peer_addr_len;
	ssize_t nread;
	char buf[BUF_SIZE];
	int af;
	char host[NI_MAXHOST], service[NI_MAXSERV];

	if (!(argc == 2 || (argc == 3 &&
						(strcmp(argv[1], "-4") == 0 || strcmp(argv[1], "-6") == 0))))
	{
		fprintf(stderr, "Usage: %s [ -4 | -6 ] port\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	if (argc == 2)
	{
		portindex = 1;
	}
	else
	{
		portindex = 2;
	}
	/* Use IPv4 by default (or if -4 is used).  If IPv6 is specified,
	 * then use that instead. */
	if (argc == 2 || strcmp(argv[1], "-4") == 0)
	{
		af = AF_INET;
	}
	else
	{
		af = AF_INET6;
	}

	memset(&hints, 0, sizeof(struct addrinfo));

	/*
	 * As a server, we want to exercise control over which protocol (IPv4
	 * or IPv6) is being used, so we specify AF_INET or AF_INET6 explicitly
	 * in hints, depending on what is passed on on the command line.
	 */
	hints.ai_family = af;			 /* Choose IPv4 or IPv6 */
	hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
	hints.ai_flags = AI_PASSIVE;	 /* Wildcard IP address - i.e., listen
					   on *all* IPv4 or *all* IPv6
					   addresses */
	hints.ai_protocol = 0;			 /* Any protocol */
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	/*
	 * getaddrinfo() returns a list of address structures. However, because
	 * we have only specified a single address family (AF_INET or AF_INET6) and
	 * have specified the wildcard IP address, we just grab the first item
	 * in the list; there is no need to loop.
	 */
	if ((s = getaddrinfo(NULL, argv[portindex], &hints, &result)) < 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	if ((sfd = socket(af, SOCK_STREAM, 0)) < 0)
	{
		perror("Error creating socket");
		exit(EXIT_FAILURE);
	}

	if (bind(sfd, result->ai_addr, result->ai_addrlen) < 0)
	{
		perror("Could not bind");
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(result); /* No longer needed */

	printf("Waiting for data on port %s...\n", argv[portindex]);

	int ret = listen(sfd, 100); //prior to the "for" loop (i.e., "for (;;)"), use the listen() function on the TCP server socket (you can use a backlog value of 100)

	peer_addr_len = sizeof(struct sockaddr_storage);
	int fd = accept(sfd, (struct sockaddr *)&peer_addr, &peer_addr_len);
	/* Read datagrams and echo them back to sender */
	for (;;)
	{
		peer_addr_len = sizeof(struct sockaddr_storage);
		printf("recvfrom() call\n");
		nread = recv(fd, buf, BUF_SIZE, 0);

		if (nread == 0) //break out of the loop if recv() returns 0 bytes.  This is an indicator that the server has closed its end of the connection and is effectively EOF.
			break;

		sleep(2); //Modify server.c such that it sleeps for 2 seconds immediately after calling recvfrom() on the socket. (between 4 and 5)

		if (nread == -1)
			continue; /* Ignore failed request */

		s = getnameinfo((struct sockaddr *)&peer_addr,
						peer_addr_len, host, NI_MAXHOST,
						service, NI_MAXSERV, NI_NUMERICSERV);

		if (s == 0)
			printf("Received %zd bytes from %s:%s\n",
				   nread, host, service);
		else
			fprintf(stderr, "getnameinfo: %s\n", gai_strerror(s));

		if (send(fd, buf, nread, 0) != nread)
			perror("Error sending response");
	}
}
