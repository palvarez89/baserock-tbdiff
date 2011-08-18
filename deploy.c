#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <unistd.h>

#include "otap.h"

#include <errno.h>

int main(int argc, char** argv) {
	if(argc < 2) {
		fprintf(stderr, "Error: No patch stream specified.\n");
		return EXIT_FAILURE;
	}

	FILE* patch = fopen(argv[1], "rb");
	if(patch == NULL) {
		fprintf(stderr, "Error: Can't open patch stream for reading.\n");
		return EXIT_FAILURE;
	}
	
	/*size_t cwd_size = pathconf(".", _PC_PATH_MAX);
	char   cwd_buff[cwd_size];
	if(getcwd(cwd_buff, cwd_size) == NULL) {
		fprintf(stderr, "Error: Couldn't resolve the current working directory.\n");
		return EXIT_FAILURE;
	}
	
	if(chroot(cwd_buff) != 0) {
		fclose(patch);
		fprintf(stderr, "Error: Couldn't chroot into the current working directory.\n");
		return EXIT_FAILURE;
	}*/
		
	int err;
	if((err = otap_apply(patch)) != 0) {
		fclose(patch);
		fprintf(stderr, "Error: Error applying patch stream (err=%d).\n", err);
		return EXIT_FAILURE;
	}
	
	fclose(patch);
	return EXIT_SUCCESS;	
}
