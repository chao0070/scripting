#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>

int main(void)
{
	int fd[2], nbytes;
	pid_t childpid;
	char string[] = "Hello, world!\n";
	char readbuffer[80];

	pipe(fd);

	if ((childpid = fork()) == -1) {
		perror("fork");
		exit(1);
	}

	if (childpid == 0) {
		// Its a child process
		close(fd[0]);

		write(fd[1], string, (strlen(string) + 1));
		exit(0);
	} else {
		close(fd[1]);

		nbytes = read(fd[0], readbuffer, sizeof(readbuffer));
		printf("Received string: %s\n", readbuffer);
	}
	return (0);
}
