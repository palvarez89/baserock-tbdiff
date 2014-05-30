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

#if !defined (TBDIFF_INSIDE_TBDIFF_H) && !defined (TBDIFF_COMPILATION)
#error "Only <tbdiff/tbdiff.h> may be included directly. This file might disappear or change contents."
#endif

#ifndef __TBDIFF_STAT_H__
#define __TBDIFF_STAT_H__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

typedef enum {
	TBD_STAT_TYPE_FILE    = 'f',
	TBD_STAT_TYPE_DIR     = 'd',
	TBD_STAT_TYPE_SYMLINK = 'l',
	TBD_STAT_TYPE_CHRDEV  = 'c',
	TBD_STAT_TYPE_BLKDEV  = 'b',
	TBD_STAT_TYPE_FIFO    = 'p',
	TBD_STAT_TYPE_SOCKET  = 's'
} tbd_stat_type_e;

typedef struct tbd_stat_s tbd_stat_t;

struct tbd_stat_s {
	tbd_stat_t*      parent;
	char*            name;
	tbd_stat_type_e  type;
	time_t           mtime;
	uint32_t         size; /* Count for directory. */
	uid_t            uid;
	gid_t            gid;
	mode_t           mode;
	uint32_t         rdev;
};

tbd_stat_t*  tbd_stat(const char *path);
void         tbd_stat_free(tbd_stat_t *file);
void         tbd_stat_print(tbd_stat_t *file);
tbd_stat_t*  tbd_stat_entry(tbd_stat_t *file, uint32_t entry);
tbd_stat_t*  tbd_stat_entry_find(tbd_stat_t *file, const char *name);
char*        tbd_stat_subpath(tbd_stat_t *file, const char *entry);
char*        tbd_stat_path(tbd_stat_t *file);
int          tbd_stat_open(tbd_stat_t *file, int flags);
FILE*        tbd_stat_fopen(tbd_stat_t *file, const char *mode);

#endif /* !__TBDIFF_STAT_H__ */
