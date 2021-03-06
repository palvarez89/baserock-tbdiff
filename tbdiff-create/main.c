/*
 *    Copyright (C) 2011-2014 Codethink Ltd.
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
#include <errno.h>

#include <fcntl.h>
#include <unistd.h>

#include <tbdiff/tbdiff.h>


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

	struct tbd_stat *tstat[2];

	tstat[0] = tbd_stat(argv[2]);
	if(tstat[0] == NULL) {
		fprintf(stderr, "Error: Unable to stat '%s'.\n", argv[2]);
		return EXIT_FAILURE;
	}

	if(chdir(cwd_buff) != 0) {
		fprintf(stderr, "Error: Unable to return to '%s'.\n", cwd_buff);
		return EXIT_FAILURE;
	}

	tstat[1] = tbd_stat(argv[3]);
	if(tstat[1] == NULL) {
		fprintf(stderr, "Error: Unable to stat '%s'.\n", argv[3]);
		return EXIT_FAILURE;
	}

	if (chdir(cwd_buff) != 0) {
		fprintf(stderr, "Error: Unable to return to '%s'.\n", cwd_buff);
		return EXIT_FAILURE;
	}

	int fd = open(argv[1],
	              O_WRONLY | O_CREAT | O_TRUNC,
	              S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if(fd < 0) {
		fprintf(stderr, "Error(%d): Unable to open patch for writing.\n", errno);
		return EXIT_FAILURE;
	}

	int err;
	if((err = tbd_create(fd, tstat[0], tstat[1])) != 0) {
		close(fd);
		tbd_stat_free(tstat[0]);
		tbd_stat_free(tstat[1]);

		remove(argv[1]);
		fprintf(stderr, "Error: Failed to create tbdiff image (err=%d).\n", err);
		switch (err) {
		case TBD_ERROR_UNABLE_TO_CREATE_SOCKET_FILE:
			fprintf(stderr, "%s directory contains Unix Sockets. "
			        "%s cannot create sockets, please ensure no "
			        "programs are using sockets in directory.\n",
			        argv[3], argv[0]);
			break;
		default:
			break;
		}
		return EXIT_FAILURE;
	}

	close(fd);
	tbd_stat_free(tstat[0]);
	tbd_stat_free(tstat[1]);

	return EXIT_SUCCESS;
}
