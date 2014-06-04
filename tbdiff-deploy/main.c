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
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <tbdiff/tbdiff.h>

static void
print_usage(const char *name)
{
	printf("Usage: %s <patch-stream>\n", name);
}

int
main(int    argc,
     char **argv)
{
	if(argc < 2) {
		fprintf(stderr, "Error: No patch stream specified.\n");
		print_usage(argv[0]);
		return EXIT_FAILURE;
	}

	int patch = open(argv[1], O_RDONLY);
	if(patch < 0) {
		fprintf(stderr,
                "Error: Can't open patch stream for reading (err=%d).\n",
                errno);
		return EXIT_FAILURE;
	}

	int err;
	if((err = tbd_apply(patch)) != 0) {
		close(patch);
		fprintf(stderr, "Error: Error applying patch stream (err=%d).\n", err);
		return EXIT_FAILURE;
	}

	close(patch);
	return EXIT_SUCCESS;
}
