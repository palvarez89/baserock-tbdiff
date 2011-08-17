#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <unistd.h>

#include "stat.h"
#include "otap.h"


int main(int argc, char** argv) {
	if(argc < 3) {
		fprintf(stderr, "Error: Need to designate 2 trees to difference.\n");
		return EXIT_FAILURE;
	}
	
	size_t cwd_size = pathconf(".", _PC_PATH_MAX);
	char   cwd_buff[cwd_size];
	if(getcwd(cwd_buff, cwd_size) == NULL)
		return EXIT_FAILURE;

	dir_stat_t* dstat[2];
	
	dstat[0] = dir_stat(NULL, argv[1]);
	if(dstat[0] == NULL) {
		fprintf(stderr, "Error: Unable to stat directory '%s'.\n", argv[1]);
		return EXIT_FAILURE;
	}
	
	dstat[1] = dir_stat(NULL, argv[2]);
	if(dstat[1] == NULL) {
		fprintf(stderr, "Error: Unable to stat directory '%s'.\n", argv[2]);
		return EXIT_FAILURE;
	}
	
	//dir_stat_print(0, dstat[0]);
	
	FILE* fp = fopen("patch.otap", "wb");
	if(fp == NULL) {
		fprintf(stderr, "Error: Unable to open patch for writing.\n");
		return EXIT_FAILURE;
	}
	
	if(!otap_create(fp, dstat[0], dstat[1])) {
		fclose(fp);
		remove("patch.otap");
		fprintf(stderr, "Error: Failed to create otap.\n");
		return EXIT_FAILURE;
	}
	
	fclose(fp);
	
	return EXIT_SUCCESS;
}
