#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>

/*
 * There is currently no command within Baserock to create named Unix sockets;
 * this program is used to compensate for that.
 */

int main(int argc, char *argv[])
{
	int sfd;
	struct sockaddr_un sock;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s PATH\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (strlen(argv[1]) >= sizeof(sock.sun_path)) {
		fprintf(stderr, "%s: file name too long\n", argv[0]);
		return EXIT_FAILURE;
	}


	if ((sfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		return EXIT_FAILURE;
	}

	sock.sun_family = AF_UNIX;
	strcpy(sock.sun_path, argv[1]);
	if (bind(sfd, (struct sockaddr*)&sock, sizeof(sock)) == -1) {
		perror("bind");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
