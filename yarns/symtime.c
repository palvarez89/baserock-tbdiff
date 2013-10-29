#define _BSD_SOURCE
#include <stdlib.h>
#include <sys/time.h>
#include <stdio.h>
#include <err.h>

/*
 * Busybox touch currently doesn't do any special symlink handling; this
 * program is used to change the modification times of symbolic links.
 */

int set_link_mtime(char *filepath, int mtime)
{
	struct timeval tv[2] = { {.tv_sec = mtime}, {.tv_sec = mtime} };

	if (lutimes(filepath, tv) == -1) {
		err(EXIT_FAILURE, NULL);
	}

	return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
	char *endptr;
	long mtime;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s FILE_PATH TIMESTAMP\n", argv[0]);
		return EXIT_FAILURE;
	}

	mtime = strtol(argv[2], &endptr, 10);

	if (*endptr != '\0') {
		fprintf(stderr, "TIMESTAMP must not include non-digits\n");
		return EXIT_FAILURE;
	}

	return set_link_mtime(argv[1], mtime);
}
