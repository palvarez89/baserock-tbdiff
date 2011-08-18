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

	otap_stat_t* tstat[2];
	
	tstat[0] = otap_stat(argv[1]);
	if(tstat[0] == NULL) {
		fprintf(stderr, "Error: Unable to stat '%s'.\n", argv[1]);
		return EXIT_FAILURE;
	}
	chdir(cwd_buff);
	
	tstat[1] = otap_stat(argv[2]);
	if(tstat[1] == NULL) {
		fprintf(stderr, "Error: Unable to stat '%s'.\n", argv[2]);
		return EXIT_FAILURE;
	}
	chdir(cwd_buff);
	
	FILE* fp = fopen("patch.otap", "wb");
	if(fp == NULL) {
		fprintf(stderr, "Error: Unable to open patch for writing.\n");
		return EXIT_FAILURE;
	}
	
	if(!otap_create(fp, tstat[0], tstat[1])) {
		fclose(fp);
		remove("patch.otap");
		fprintf(stderr, "Error: Failed to create otap.\n");
		return EXIT_FAILURE;
	}
	
	fclose(fp);
	otap_stat_free(tstat[0]);
	otap_stat_free(tstat[1]);
	
	return EXIT_SUCCESS;
}
