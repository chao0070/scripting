/**
 *
 *This is done following beej's tutorials
 */
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>
#define PORT "3490"

#define BACKLOG 10

// For handling the zombie processes created by the server to process client
// requests
void sigchild_handler(int s)
{
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

// printf("Signal handler called\n");
	errno = saved_errno;
}

void register_signal_handler(struct sigaction *sa) {
	sa->sa_handler = sigchild_handler;
	sigemptyset(&sa->sa_mask);
	sa->sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}
}

void
*get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int get_sockfd_server() {
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int yes = 1;
	int rv;
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1; 
	}

	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
		    p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
		    sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}
		
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server:bind");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo);

	if (p == NULL) {
		fprintf(stderr, "Server failed to bind\n");
		exit(1);
	}

	return sockfd;
}


int main()
{
	int sockfd, new_fd;
	struct sockaddr_storage their_addr;
	socklen_t sin_size;
	struct sigaction sa;
	int yes = 1;
	char s[INET6_ADDRSTRLEN];

	sockfd = get_sockfd_server();

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	register_signal_handler(&sa);

	printf("server: waiting for connections...\n");

	while (1) {
		sin_size = sizeof(their_addr);
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr,
		    &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
		    get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
		printf("server: got connection from %s\n", s);

		if (!fork()) {
			close(sockfd);
			if (send(new_fd, "Hello, world!", 13, 0) == -1)
				perror("send");
			close(new_fd);
			exit(0);
		}
		close(new_fd);
	}

}


