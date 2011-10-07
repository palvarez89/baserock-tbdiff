#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define MAX_CLIENTS 1
#ifdef DEBUG
#define DEBUGPRINT(fmt, ...) fprintf(stderr, "DEBUG: %s %s %d: " fmt, \
                                     __FILE__, __func__, __LINE__, __VA_ARGS__)
#else
#define DEBUGPRINT(...) (void)0
#endif

int main(int argc, char *argv[]){
	struct sockaddr_un sock = {
		.sun_family = AF_UNIX
	};
	int sfd;
	if (argc < 1){
		fprintf(stderr, "Usage: %s PATH\n", argv[0]);
		return 1;
	}
	strncpy(sock.sun_path, argv[1], sizeof(sock) - offsetof(struct sockaddr_un, sun_path));
	DEBUGPRINT("%s", "Constructed socket address\n");
	if ((sfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
		perror("socket");
		return 2;
	}
	DEBUGPRINT("Created socket fd=%d\n", sfd);
	if (bind(sfd, (struct sockaddr*)&sock, sizeof(sock)) == -1){
		perror("bind");
		return 3;
	}
	if (listen(sfd, MAX_CLIENTS) == -1){
		perror("listen");
		return 4;
	}
	DEBUGPRINT("Listening to %d clients\n", MAX_CLIENTS);
	{
		struct sockaddr_un client_address;
		socklen_t client_size = sizeof(client_address);
		int cfd;
		while ((cfd = accept(sfd, (struct sockaddr*)&client_address,
		                     &client_size)) != -1) {
			char buf[BUFSIZ];
			ssize_t rdcount = -1;
			DEBUGPRINT("Listening to client fd=%d\n", cfd);
			while ((rdcount = read(cfd, buf, sizeof(buf))) > 0) {
				DEBUGPRINT("Read %zi bytes from client, "
				           "message was:\n%.*s", rdcount,
				           rdcount, buf);
				write(STDOUT_FILENO, buf, rdcount);
			}
			assert(rdcount == 0 || rdcount == -1);
			if (rdcount == -1) {
				perror("read");
				return 5;
			}
			DEBUGPRINT("Finished listening to fd=%d\n", cfd);
			close(cfd);
		}
	}

	close(sfd);
	unlink(argv[1]);
	return 0;
}
