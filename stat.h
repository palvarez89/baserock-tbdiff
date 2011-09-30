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

#ifndef __otap_stat_h__
#define __otap_stat_h__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

typedef enum {
	OTAP_STAT_TYPE_FILE    = 'f',
	OTAP_STAT_TYPE_DIR     = 'd',
	OTAP_STAT_TYPE_SYMLINK = 'l',
	OTAP_STAT_TYPE_CHRDEV  = 'c',
	OTAP_STAT_TYPE_BLKDEV  = 'b',
	OTAP_STAT_TYPE_FIFO    = 'p',
	OTAP_STAT_TYPE_SOCKET  = 's'
} otap_stat_type_e;

typedef struct {
	void*            parent;
	char*            name;
	otap_stat_type_e type;
	uint32_t         mtime;
	uint32_t         size; // Count for directory.
	uint32_t         uid;
	uint32_t         gid;
	uint32_t         mode;
	uint32_t         rdev;
} otap_stat_t;


extern otap_stat_t* otap_stat(const char* path);
extern void         otap_stat_free(otap_stat_t* file);
extern void         otap_stat_print(otap_stat_t* file);
extern otap_stat_t* otap_stat_entry(otap_stat_t* file, uint32_t entry);
extern otap_stat_t* otap_stat_entry_find(otap_stat_t* file, const char* name);
extern char*        otap_stat_subpath(otap_stat_t* file, const char* entry);
extern char*        otap_stat_path(otap_stat_t* file);
extern int          otap_stat_open(otap_stat_t* file, int flags);
extern FILE*        otap_stat_fopen(otap_stat_t* file, const char* mode);

#endif
