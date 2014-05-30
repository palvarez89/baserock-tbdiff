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
#include <stddef.h>
#include <string.h>
#include <inttypes.h>

#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

#include "tbdiff-stat.h"

static struct tbd_stat*
tbd_stat_from_path(const char *name,
                   const char *path)
{
	struct stat info;

	if(lstat(path, &info) != 0)
		return NULL;

	size_t nlen = strlen(name);
	struct tbd_stat *ret = (struct tbd_stat*)malloc(
	                          sizeof(struct tbd_stat) + (nlen + 1));
	if(ret == NULL)
		return NULL;

	ret->parent = NULL;
	ret->size = 0;
	ret->name   = (char*)((uintptr_t)ret +
                          sizeof(struct tbd_stat));
	memcpy(ret->name, name, (nlen + 1));	

	if(S_ISREG(info.st_mode)) {
		ret->type = TBD_STAT_TYPE_FILE;
		ret->size = info.st_size;
	} else if(S_ISDIR(info.st_mode)) {
		ret->type = TBD_STAT_TYPE_DIR;
		DIR *dp = opendir(path);

		if(dp == NULL) {
			free(ret);
			return NULL;
		}
		
		/* FIXME: Remove the need for directory size? */
		struct dirent *ds;
		for(ds = readdir(dp); ds != NULL; ds = readdir(dp)) {
			if((strcmp(ds->d_name, ".") == 0)
			    || (strcmp(ds->d_name, "..") == 0))
				continue;

			ret->size++;
		}
		closedir(dp);
	} else if(S_ISLNK(info.st_mode))
		ret->type = TBD_STAT_TYPE_SYMLINK;
	else if(S_ISCHR(info.st_mode))
		ret->type = TBD_STAT_TYPE_CHRDEV;
	else if(S_ISBLK(info.st_mode))
		ret->type = TBD_STAT_TYPE_BLKDEV;
	else if(S_ISFIFO(info.st_mode))
		ret->type = TBD_STAT_TYPE_FIFO;
	else if(S_ISSOCK(info.st_mode))
		ret->type = TBD_STAT_TYPE_SOCKET;
	else {
		free(ret);
		return NULL;
	}

	ret->rdev  = (uint32_t)info.st_rdev;
	ret->uid   = (uid_t)info.st_uid;
	ret->gid   = (gid_t)info.st_gid;
	ret->mode  = (mode_t)info.st_mode;
	ret->mtime = (time_t)info.st_mtime;
	return ret;
}

struct tbd_stat*
tbd_stat(const char *path)
{
	struct tbd_stat *ret = tbd_stat_from_path(path, path);
	return ret;
}

void
tbd_stat_free(struct tbd_stat *file)
{
	free(file);
}

void
tbd_stat_print(struct tbd_stat *file)
{
	(void)file;
}

struct tbd_stat*
tbd_stat_entry(struct tbd_stat *file, uint32_t entry)
{
	if((file == NULL)
	    || (file->type != TBD_STAT_TYPE_DIR)
	    || (entry >= file->size))
		return NULL;

	char *path = tbd_stat_path(file);
	DIR *dp = opendir(path);
	free (path);

	if(dp == NULL)
		return NULL;

	uintptr_t i;
	struct dirent *ds;
	for(i = 0; i <= entry; i++) {
		ds = readdir(dp);
		if(ds == NULL) {
			closedir(dp);
			return NULL;
		}

		if((strcmp(ds->d_name, ".") == 0) ||
		    (strcmp(ds->d_name, "..") == 0))
			i--;
	}
	char *name = strndup(ds->d_name,
	                     ds->d_reclen-offsetof(struct dirent, d_name));
	closedir (dp);

	char *spath = tbd_statubpath(file, name);
	if(spath == NULL) {
		free(name);
		return NULL;
    }

	struct tbd_stat *ret = tbd_stat_from_path(name, (const char*)spath);

	free(name);
	free(spath);

	if (ret == NULL)
		return NULL;

	ret->parent = file;
	return ret;
}

struct tbd_stat*
tbd_stat_entry_find(struct tbd_stat *file,
                    const char      *name)
{
	if((file == NULL)
	    || (file->type != TBD_STAT_TYPE_DIR))
		return NULL;

	char *path = tbd_stat_path (file);
	DIR *dp = opendir(path);
	free (path);

	if(dp == NULL)
		return NULL;

	struct dirent *ds;
	for(ds = readdir(dp); ds != NULL; ds = readdir(dp)) {
		if(strcmp(ds->d_name, name) == 0) {
			char *spath = tbd_statubpath(file, ds->d_name);

			if(spath == NULL) {
				closedir (dp);
				return NULL;
			}

			struct tbd_stat *ret = tbd_stat_from_path(ds->d_name,
			                                   (const char*)spath);
			free(spath);
			ret->parent = file;

			closedir (dp);
			return ret;
		}
	}

	closedir (dp);
	return NULL;
}

char*
tbd_statubpath(struct tbd_stat *file,
                 const char    *entry)
{
	if(file == NULL)
		return NULL;

	size_t elen = ((entry == NULL) ? 0 : (strlen(entry) + 1));
	size_t plen;

	struct tbd_stat *root;
	for(root = file, plen = 0;
	    root != NULL;
	    plen += (strlen(root->name) + 1),
	    root = (struct tbd_stat*)root->parent);

	plen += elen;

	char *path = (char*)malloc(plen);
	if(path == NULL)
		return NULL;
	char *ptr = &path[plen];

	if(entry != NULL) {
		ptr = (char*)((uintptr_t)ptr - elen);
		memcpy(ptr, entry, elen);
	}

	for(root = file; root != NULL; root = (struct tbd_stat*)root->parent) {
		size_t rlen = strlen(root->name) + 1;
		ptr = (char*)((uintptr_t)ptr - rlen);
		memcpy(ptr, root->name, rlen);
		if((file != root) || (entry != NULL))
			ptr[rlen - 1] = '/';
	}

	return path;
}

char*
tbd_stat_path(struct tbd_stat *file)
{
	return tbd_statubpath(file, NULL);
}

int
tbd_stat_open(struct tbd_stat *file, int flags)
{
	char *path = tbd_stat_path(file);
	if(path == NULL)
		return -1;
	int fd = open(path, flags);
	free(path);
	return fd;
}
