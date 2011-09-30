/*
 *    Copyright (C) 2011 Codethink Ltd.
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License Version 2 as
 *    published by the Free Software Foundation.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License along
 *    with this program; if not, write to the Free Software Foundation, Inc.,
 *    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <unistd.h>

#include "libtbd_stat.h"
#include "tbdiff.h"


int
main(int    argc,
     char **argv)
{
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

	if(chdir(cwd_buff) != 0) {
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
		remove(argv[1]);
		fprintf(stderr, "Error: Failed to create otap (err=%d).\n", err);
		return EXIT_FAILURE;
	}

	fclose(fp);
	otap_stat_free(tstat[0]);
	otap_stat_free(tstat[1]);

	return EXIT_SUCCESS;
}
