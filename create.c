#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <unistd.h>

#include "stat.h"
#include "otap.h"


int main(int argc, char** argv) {
	if(argc < 4) {
		fprintf(stderr, "Usage: %s OUTPUT SOURCE_DIR TARGET_DIR.\n", argv[0]);
		return EXIT_FAILURE;
	}
	
	size_t cwd_size = pathconf(".", _PC_PATH_MAX);
	char   cwd_buff[cwd_size];
	if(getcwd(cwd_buff, cwd_size) == NULL)
		return EXIT_FAILURE;

	otap_stat_t* tstat[2];
	
	tstat[0] = otap_stat(argv[2]);
	if(tstat[0] == NULL) {
		fprintf(stderr, "Error: Unable to stat '%s'.\n", argv[2]);
		return EXIT_FAILURE;
	}
        
	if (chdir(cwd_buff) != 0) {
                fprintf(stderr, "Error: Unable to return to '%s'.\n", cwd_buff);
                return EXIT_FAILURE;
  }
	
	tstat[1] = otap_stat(argv[3]);
	if(tstat[1] == NULL) {
		fprintf(stderr, "Error: Unable to stat '%s'.\n", argv[3]);
		return EXIT_FAILURE;
	}
	
	if (chdir(cwd_buff) != 0) {
                fprintf(stderr, "Error: Unable to return to '%s'.\n", cwd_buff);
                return EXIT_FAILURE;
  }
	
	FILE* fp = fopen(argv[1], "wb");
	if(fp == NULL) {
		fprintf(stderr, "ERROR: Unable to open patch for writing.\n");
		return EXIT_FAILURE;
	}
	
	int err;
	if((err = otap_create(fp, tstat[0], tstat[1])) != 0) {
		fclose(fp);
		remove("patch.otap");
		fprintf(stderr, "Error: Failed to create otap (err=%d).\n", err);
		return EXIT_FAILURE;
	}
	
	fclose(fp);
	otap_stat_free(tstat[0]);
	otap_stat_free(tstat[1]);
	
	return EXIT_SUCCESS;
}
