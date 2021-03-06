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
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

#include "tbdiff-common.h"
#include "tbdiff-io.h"
#include "tbdiff-private.h"
#include "tbdiff-xattrs.h"

#define PATH_BUFFER_LENGTH 4096

static int
tbd_create_write_cmd(int          stream,
                     tbd_cmd_type cmd)
{
	if(write(stream, &cmd, sizeof(tbd_cmd_type)) != sizeof(tbd_cmd_type))
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_WRITE_STREAM);
	return 0;
}

static int
tbd_create_write_string(int         stream,
                        const char *string)
{
	uint16_t slen = strlen(string);
	if((!tbd_write_uint16(slen, stream))
	    || (write(stream, string, slen) != slen))
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_WRITE_STREAM);
	return 0;
}

static int
tbd_create_write_block(int         stream,
                       void const *data,
                       size_t      size)
{
	if (write(stream, &size, sizeof(size)) != sizeof(size)) {
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_WRITE_STREAM);
	}

	if (write(stream, data, size) != size) {
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_WRITE_STREAM);
	}
	return TBD_ERROR_SUCCESS;
}

static int
tbd_create_write_mdata_mask(int      stream,
                            uint16_t mask)
{
	if(!tbd_write_uint16(mask, stream))
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_WRITE_STREAM);
	return 0;
}

static int
tbd_create_write_mtime(int    stream,
                       time_t mtime)
{
	if(!tbd_write_time(mtime, stream))
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_WRITE_STREAM);
	return 0;
}

static int
tbd_create_write_mode(int    stream,
                      mode_t mode)
{
	if(!tbd_write_mode(mode, stream))
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_WRITE_STREAM);
	return 0;
}

static int
tbd_create_write_gid(int   stream,
                     gid_t gid)
{
	if(!tbd_write_gid(gid, stream))
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_WRITE_STREAM);
	return 0;
}

static int
tbd_create_write_uid(int   stream,
                     uid_t uid)
{
	if(!tbd_write_uid(uid, stream))
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_WRITE_STREAM);
	return 0;
}

static int
tbd_create_write_dev(int      stream,
                     uint32_t dev)
{
	if(!tbd_write_uint32(dev, stream))
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_WRITE_STREAM);
	return 0;
}

static int
tbd_create_cmd_ident(int stream)
{
	int err;

	if((err = tbd_create_write_cmd(stream, TBD_CMD_IDENTIFY)) != 0)
		return err;
	if((err = tbd_create_write_string(stream, TB_DIFF_PROTOCOL_ID)) != 0)
		return err;
	return 0;
}

static int
tbd_create_cmd_update(int stream)
{
	return tbd_create_write_cmd(stream, TBD_CMD_UPDATE);
}

/* callback function to pass to tbx_xattrs_pairs
 * this will write the attribute name, then the data representing that block
 */
static int
tbd_create_cmd_write_xattr_pair(char const *name,
                                void const *data,
                                size_t      size,
                                void       *ud)
{
	int err;
	int stream = *((int *)ud);

	if ((err = tbd_create_write_string(stream, name)) !=
	    TBD_ERROR_SUCCESS)
		return err;

	if ((err = tbd_create_write_block(stream, data, size)) !=
	    TBD_ERROR_SUCCESS)
		return err;

	return TBD_ERROR_SUCCESS;
}

static int
tbd_create_cmd_write_xattrs(int stream, struct tbd_stat *f)
{
	int err = TBD_ERROR_SUCCESS;
	struct tbd_xattrs_names names;
	char *path = tbd_stat_path(f);
	if (path == NULL) {
		return TBD_ERROR(TBD_ERROR_OUT_OF_MEMORY);
	}

	switch (err = tbd_xattrs_names(path, &names)) {
	/* separated as ignore XATTR unspported may be added */
	case TBD_ERROR_XATTRS_NOT_SUPPORTED:
	default:
		goto cleanup_path;
	case TBD_ERROR_SUCCESS:
		break;
	}
	
	{ /* write the header */
		uint32_t count;
		/* if failed to count or there are no xattrs */
		if ((err = tbd_xattrs_names_count(&names, &count)) !=
		    TBD_ERROR_SUCCESS || count == 0) {
			goto cleanup_names;
		}

		if ((err = tbd_create_write_cmd(stream,
						 TBD_CMD_XATTRS_UPDATE)
		    ) != TBD_ERROR_SUCCESS) {
			goto cleanup_names;
		}

		if ((err = tbd_create_write_string(stream, f->name))!=
		    TBD_ERROR_SUCCESS) {
			goto cleanup_names;
		}

		if (!tbd_write_uint32(count, stream)) {
			err = TBD_ERROR(TBD_ERROR_UNABLE_TO_WRITE_STREAM);
			goto cleanup_names;
		}
	}

	/* write the name:data pairs */
	err = tbd_xattrs_pairs(&names,
	                        path,
	                        tbd_create_cmd_write_xattr_pair,
	                        &stream);

cleanup_names:
	tbd_xattrs_names_free(&names);
cleanup_path:
	free(path);
	return err;
}

static int
tbd_create_cmd_file_create(int              stream,
                           struct tbd_stat *f)
{
	int err;
	if((err = tbd_create_write_cmd(stream, TBD_CMD_FILE_CREATE)) != 0 ||
	   (err = tbd_create_write_string(stream, f->name))          != 0 ||
	   (err = tbd_create_write_mtime (stream, f->mtime))         != 0 ||
	   (err = tbd_create_write_mode  (stream, f->mode))          != 0 ||
	   (err = tbd_create_write_uid   (stream, f->uid))           != 0 ||
	   (err = tbd_create_write_gid   (stream, f->gid))           != 0)
		return err;

	uint32_t size = f->size;
	if(!tbd_write_uint32(size, stream))
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_WRITE_STREAM);

	int fd = tbd_stat_open(f, O_RDONLY);
	if(fd < 0)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_OPEN_FILE_FOR_READING);

	uint8_t buff[256];
	uintptr_t b = 256;
	for(b = 256; b == 256; ) {
		b = read(fd, buff, b);
		if(write(stream, buff, b) != b) {
			close(fd);
			return TBD_ERROR(TBD_ERROR_UNABLE_TO_WRITE_STREAM);
		}
	}
	close(fd);

	return tbd_create_cmd_write_xattrs(stream, f);
}

static uint16_t
tbd_metadata_mask(struct tbd_stat *a,
                  struct tbd_stat *b)
{
	uint16_t metadata_mask = TBD_METADATA_NONE;

	/* If nothing changes we issue no command */
	if(a->mtime != b->mtime)
		metadata_mask |= TBD_METADATA_MTIME;
	if(a->uid != b->uid)
		metadata_mask |= TBD_METADATA_UID;
	if(a->gid != b->gid)
		metadata_mask |= TBD_METADATA_GID;
	if(a->mode != b->mode)
		metadata_mask |= TBD_METADATA_MODE;

	return metadata_mask;
}

static int
tbd_create_cmd_file_metadata_update(int              stream,
                                    struct tbd_stat *a,
                                    struct tbd_stat *b)
{
	int err;
	uint16_t metadata_mask = tbd_metadata_mask(a, b);

	if(metadata_mask == TBD_METADATA_NONE)
		return 0;
	/* TODO: Optimize protocol by only sending useful metadata */
	if((err = tbd_create_write_cmd(stream, TBD_CMD_FILE_METADATA_UPDATE)) != 0 ||
	   (err = tbd_create_write_mdata_mask(stream, metadata_mask))         != 0 ||
	   (err = tbd_create_write_mtime     (stream, b->mtime))              != 0 ||
	   (err = tbd_create_write_uid       (stream, b->uid))                != 0 ||
	   (err = tbd_create_write_gid       (stream, b->gid))                != 0 ||
	   (err = tbd_create_write_mode      (stream, b->mode))               != 0)
		return err;

	return tbd_create_write_string(stream, b->name);
}

static int
tbd_create_cmd_file_delta(int              stream,
                          struct tbd_stat *a,
                          struct tbd_stat *b)
{
	int fda = tbd_stat_open(a, O_RDONLY);
	if(fda < 0)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_OPEN_FILE_FOR_READING);
	int fdb = tbd_stat_open(b, O_RDONLY);
	if(fdb < 0) {
		close(fda);
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_OPEN_FILE_FOR_READING);
	}

	/* Calculate start. */
	uintptr_t blks[2] = { 256, 256 };
	uint8_t   buff[2][256];

	uintptr_t o;
	for(o = 0; (blks[1] == blks[0]) && (blks[0] != 0); o += blks[1]) {
		blks[0] = read(fda, buff[0], blks[0]);
		blks[1] = read(fdb, buff[1], blks[0]);
		if((blks[0] == 0) || (blks[1] == 0))
			break;

		uintptr_t i;
		for(i = 0; i < blks[1]; i++) {
			if(buff[0][i] != buff[1][i]) {
				o += i;
				break;
			}
		}
		if(i < blks[1])
			break;
	}
	uint32_t start = o;

	/* Find length. */
	off_t flena = lseek(fda, 0, SEEK_END);
	off_t flenb = lseek(fdb, 0, SEEK_END);

	if((flena < 0) || (flenb < 0)) {
		close(fda);
		close(fdb);
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_SEEK_THROUGH_STREAM);
	}

	/* Find end. */
	blks[0] = 256;
	blks[1] = 256;
	for(o = 0; true; o += blks[1]) {
		blks[0] = ((flena - o) < 256     ? (flena - o) : 256    );
		blks[1] = ((flenb - o) < blks[0] ? (flenb - o) : blks[0]);
		if((blks[0] == 0) || (blks[1] == 0))
			break;

		if((lseek(fda, flena - (o + blks[0]), SEEK_SET) < 0)
		    || (lseek(fdb, flenb - (o + blks[1]), SEEK_SET) < 0)) {
			close(fda);
			close(fdb);
			return TBD_ERROR(TBD_ERROR_UNABLE_TO_SEEK_THROUGH_STREAM);
		}

		if((read(fda, buff[0], blks[0]) != blks[0])
		    || (read(fdb, buff[1], blks[1]) != blks[1])) {
			close(fda);
			close(fdb);
			return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
		}

		uintptr_t i, ja, jb;
		for(i = 0, ja = (blks[0] - 1), jb = (blks[1] - 1); i < blks[1]; i++, ja--, jb--) {
			if(buff[0][ja] != buff[1][jb]) {
				o += i;
				break;
			}
		}
		if(i < blks[1])
			break;
	}
	close(fda);

	/* Ensure that the start and end don't overlap for the new file. */
	if((flenb - o) < start)
		o = (flenb - start);

	uint32_t end = (flena - o);
	if(end < start)
		end = start;

	uint32_t size = flenb - ((flena - end) + start); /* (flenb - (o + start)); */

	/* Data is identical, only alter metadata */
	if((end == start) && (size == 0)) {
		tbd_create_cmd_file_metadata_update(stream, a, b);
		close(fdb);
		return tbd_create_cmd_write_xattrs(stream, b);
	}

	uint16_t metadata_mask = tbd_metadata_mask(a, b);

	/* TODO: Optimize protocol by only sending useful metadata */
	int err;
	if(((err = tbd_create_write_cmd(stream, TBD_CMD_FILE_DELTA))    != 0) ||
	    ((err = tbd_create_write_string(stream, b->name))           != 0) ||
	    ((err = tbd_create_write_mdata_mask(stream, metadata_mask)) != 0) ||
	    ((err = tbd_create_write_mtime (stream, b->mtime))          != 0) ||
	    ((err = tbd_create_write_uid   (stream, b->uid))            != 0) ||
	    ((err = tbd_create_write_gid   (stream, b->gid))            != 0) ||
	    ((err = tbd_create_write_mode  (stream, b->mode))           != 0)) {
		close(fdb);
		return err;
	}

	if((!tbd_write_uint32(start, stream)) ||
	   (!tbd_write_uint32(end  , stream)) ||
	   (!tbd_write_uint32(size , stream))) {
		close(fdb);
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_WRITE_STREAM);
	}
	if(lseek(fdb, start, SEEK_SET) < 0) {
		close(fdb);
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_SEEK_THROUGH_STREAM);
	}

	for(o = 0; o < size; o += 256) {
		uintptr_t csize = ((size - o) > 256 ? 256 : (size - o));
		if(read(fdb, buff[0], csize) != csize) {
			close(fdb);
			return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
		}
		if(write(stream, buff[0], csize) != csize) {
			close(fdb);
			return TBD_ERROR(TBD_ERROR_UNABLE_TO_WRITE_STREAM);
		}
	}

	close(fdb);
	return tbd_create_cmd_write_xattrs(stream, b);
}

static int
tbd_create_cmd_dir_create(int              stream,
                          struct tbd_stat *d)
{
	int err;

	if((err = tbd_create_write_cmd(stream, TBD_CMD_DIR_CREATE)) != 0 ||
	   (err = tbd_create_write_string(stream, d->name))         != 0 ||
	   (err = tbd_create_write_mtime(stream, d->mtime))         != 0 ||
	   (err = tbd_create_write_uid(stream, d->uid))             != 0 ||
	   (err = tbd_create_write_gid(stream, d->gid))             != 0)
		return err;

	return tbd_create_write_mode(stream, d->mode);
}

static int
tbd_create_cmd_dir_enter(int         stream,
                         const char *name)
{
	int err;
	if((err = tbd_create_write_cmd(stream, TBD_CMD_DIR_ENTER)) != 0)
		return err;
	return tbd_create_write_string(stream, name);
}

static int
tbd_create_cmd_dir_leave(int              stream,
                         struct tbd_stat *dir)
{
	int err;
	if ((err = tbd_create_write_cmd(stream, TBD_CMD_DIR_LEAVE)) !=
	    TBD_ERROR_SUCCESS) {
		return err;
	}

	return tbd_create_write_mtime(stream, dir->mtime);
}

static int
tbd_create_cmd_entity_delete(int         stream,
                             const char *name)
{
	int err;
	if((err = tbd_create_write_cmd(stream, TBD_CMD_ENTITY_DELETE)) != 0)
		return err;
	return tbd_create_write_string(stream, name);
}

static int
tbd_create_cmd_dir_delta(int               stream,
                         struct tbd_stat  *a,
                         struct tbd_stat  *b)
{
	int err;
	uint16_t metadata_mask = tbd_metadata_mask(a, b);

	if(metadata_mask == TBD_METADATA_NONE)
		return 0;

	if((err = tbd_create_write_cmd(stream, TBD_CMD_DIR_DELTA))     != 0 ||
	   (err = tbd_create_write_mdata_mask (stream, metadata_mask)) != 0 ||
	   (err = tbd_create_write_mtime      (stream, b->mtime))      != 0 ||
	   (err = tbd_create_write_uid        (stream, b->uid))        != 0 ||
	   (err = tbd_create_write_gid        (stream, b->gid))        != 0 ||
	   (err = tbd_create_write_mode       (stream, b->mode))       != 0)
		return err;

	return tbd_create_write_string(stream, b->name);
}

static int
tbd_create_cmd_symlink_create(int              stream,
                              struct tbd_stat *symlink)
{
	int err;
	char path[PATH_BUFFER_LENGTH];

	char *slpath = tbd_stat_path(symlink);
	ssize_t len = readlink(slpath, path, sizeof(path)-1);
	free(slpath);
	if(len < 0)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_SYMLINK);
	path[len] = '\0';

	if((err = tbd_create_write_cmd(stream, TBD_CMD_SYMLINK_CREATE)) != 0 ||
	   (err = tbd_create_write_mtime (stream, symlink->mtime))      != 0 ||
	   (err = tbd_create_write_uid   (stream, symlink->uid))        != 0 ||
	   (err = tbd_create_write_gid   (stream, symlink->gid))        != 0 ||
	   (err = tbd_create_write_string(stream, symlink->name))       != 0)
		return err;

	return tbd_create_write_string(stream, path);
}

static int
tbd_create_cmd_symlink_delta(int              stream,
                             struct tbd_stat *a,
                             struct tbd_stat *b)
{
	int err;
	char path_a[PATH_BUFFER_LENGTH];
	char path_b[PATH_BUFFER_LENGTH];

	char *spath_a = tbd_stat_path(a);
	char *spath_b = tbd_stat_path(b);

	ssize_t len_a = readlink(spath_a, path_a, sizeof(path_a)-1);
	ssize_t len_b = readlink(spath_b, path_b, sizeof(path_b)-1);

	free(spath_a);
	free(spath_b);

	if(len_a < 0 || len_b < 0)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_SYMLINK);

	path_a[len_a] = path_b[len_b] = '\0';

	int pathcmp = strcmp(path_a, path_b);
	printf ("readlink %s %s - %d\n", path_a, path_b, pathcmp);

	/* If both symlinks are equal, we quit */
	if ((b->mtime == a->mtime) && (b->gid == a->gid) && (pathcmp == 0))
		return 0;

	/* TODO: If only mtime changes, use a mtime update cmd */
	err = tbd_create_cmd_entity_delete(stream, a->name);
	if(err != 0)
		return err;

	return tbd_create_cmd_symlink_create(stream, b);
}

static int
tbd_create_cmd_special_create(int              stream,
                              struct tbd_stat *nod)
{
	int err;

	if((err = tbd_create_write_cmd(stream, TBD_CMD_SPECIAL_CREATE)) != 0 ||
	   (err = tbd_create_write_string(stream, nod->name))           != 0 ||
	   (err = tbd_create_write_mtime (stream, nod->mtime))          != 0 ||
	   (err = tbd_create_write_mode  (stream, nod->mode))           != 0 ||
	   (err = tbd_create_write_uid   (stream, nod->uid))            != 0 ||
	   (err = tbd_create_write_gid   (stream, nod->gid))            != 0)
		return err;
	return tbd_create_write_dev(stream, nod->rdev);
}

static int
tbd_create_cmd_special_delta(int              stream,
                             struct tbd_stat *a,
                             struct tbd_stat *b)
{
	uint16_t metadata_mask = tbd_metadata_mask(a, b);
	if(a->rdev != b->rdev)
		metadata_mask |= TBD_METADATA_RDEV;

	/* If nothing changes we issue no command */
	if(metadata_mask == TBD_METADATA_NONE)
		return 0;

	int err;
	if((err = tbd_create_cmd_entity_delete(stream, a->name)) != 0)
		return err;

	return tbd_create_cmd_special_create(stream, b);
}

static int
tbd_create_cmd_socket_create(int              stream,
                             struct tbd_stat *nod)
{
	(void)stream;
	(void)nod;
	return TBD_ERROR(TBD_ERROR_UNABLE_TO_CREATE_SOCKET_FILE);
}

static int
tbd_create_cmd_socket_delta(int              stream,
                            struct tbd_stat *a,
                            struct tbd_stat *b)
{
	(void)stream;
	(void)a;
	(void)b;
	return TBD_ERROR(TBD_ERROR_UNABLE_TO_CREATE_SOCKET_FILE);
}

static int
tbd_create_dir(int              stream,
               struct tbd_stat *d)
{
	int err;
	if(((err =tbd_create_cmd_dir_create(stream, d))       != 0) ||
	   ((err = tbd_create_cmd_dir_enter(stream, d->name)) != 0))
		return err;

	uintptr_t i;
	for(i = 0; i < d->size; i++) {
		struct tbd_stat *f = tbd_stat_entry(d, i);

		if(f == NULL)
			return TBD_ERROR(TBD_ERROR_UNABLE_TO_STAT_FILE);

		switch(f->type) {
		case TBD_STAT_TYPE_FILE:
			err = tbd_create_cmd_file_create(stream, f);
			break;
		case TBD_STAT_TYPE_DIR:
			err = tbd_create_dir(stream, f);
			break;
		case TBD_STAT_TYPE_SYMLINK:
			err = tbd_create_cmd_symlink_create(stream, f);
			break;
		case TBD_STAT_TYPE_BLKDEV:
		case TBD_STAT_TYPE_CHRDEV:
		case TBD_STAT_TYPE_FIFO:
		case TBD_STAT_TYPE_SOCKET:
			err = tbd_create_cmd_special_create(stream, f);
			break;
		default:
			tbd_stat_free(f);
			return TBD_ERROR(TBD_ERROR_FEATURE_NOT_IMPLEMENTED);
			break;
		}
		tbd_stat_free(f);
		if(err != 0)
			return err;
	}

	return tbd_create_cmd_dir_leave(stream, d);
}

static int
tbd_create_impl(int              stream,
                struct tbd_stat *a,
                struct tbd_stat *b,
                bool        top)
{
	if((a == NULL) && (b == NULL))
		return TBD_ERROR(TBD_ERROR_NULL_POINTER);

	int err;
	if(((b == NULL) || ((a != NULL) && (a->type != b->type)))) {
		TBD_DEBUGF("file delete %s\n", a->name);
		if((err = tbd_create_cmd_entity_delete(stream, a->name)) != 0)
			return err;
	}

	if((a == NULL) || ((b != NULL) && (a->type != b->type))) {
		switch(b->type) {
		case TBD_STAT_TYPE_FILE:
			TBD_DEBUGF("file new %s\n", b->name);
			return tbd_create_cmd_file_create(stream, b);
		case TBD_STAT_TYPE_DIR:
			TBD_DEBUGF("dir new %s\n", b->name);
			return tbd_create_dir(stream, b);
		case TBD_STAT_TYPE_SYMLINK:
			TBD_DEBUGF("symlink new %s\n", b->name);
			return tbd_create_cmd_symlink_create(stream, b);
		case TBD_STAT_TYPE_CHRDEV:
		case TBD_STAT_TYPE_BLKDEV:
		case TBD_STAT_TYPE_FIFO:
			TBD_DEBUGF("special new %s\n", b->name);
			return tbd_create_cmd_special_create(stream, b);
		case TBD_STAT_TYPE_SOCKET:
			TBD_DEBUGF("socket new %s\n", b->name);
			return tbd_create_cmd_socket_create(stream, b);
		default:
			return TBD_ERROR(TBD_ERROR_FEATURE_NOT_IMPLEMENTED);
		}
	}

	switch(b->type) {
	case TBD_STAT_TYPE_FILE:
		TBD_DEBUGF("file delta %s\n", a->name);
		return tbd_create_cmd_file_delta(stream, a, b);
	case TBD_STAT_TYPE_SYMLINK:
		TBD_DEBUGF("symlink delta %s\n", a->name);
		return tbd_create_cmd_symlink_delta(stream, a, b);
	case TBD_STAT_TYPE_CHRDEV:
	case TBD_STAT_TYPE_BLKDEV:
	case TBD_STAT_TYPE_FIFO:
		TBD_DEBUGF("special delta %s\n", a->name);
		return tbd_create_cmd_special_delta(stream, a, b);
	case TBD_STAT_TYPE_SOCKET:
		TBD_DEBUGF("socket delta %s\n", a->name);
		return tbd_create_cmd_socket_delta(stream, a, b);
	case TBD_STAT_TYPE_DIR:
		if(!top) {
			TBD_DEBUGF("dir delta %s\n", a->name);
			if ((err = tbd_create_cmd_dir_delta(stream, a, b)) !=
			    TBD_ERROR_SUCCESS) {
				return err;
			}
		}
		break;
	default:
		break;
	}

	if(!top && ((err = tbd_create_cmd_dir_enter(stream, b->name)) != 0))
		return err;

	/* Handle changes/additions. */
	uintptr_t i;
	for(i = 0; i < b->size; i++) {
		struct tbd_stat *_b = tbd_stat_entry(b, i);
		if(_b == NULL)
			return TBD_ERROR(TBD_ERROR_UNABLE_TO_STAT_FILE);
		struct tbd_stat *_a = tbd_stat_entry_find(a, _b->name);
		err = tbd_create_impl(stream, _a, _b, false);
		tbd_stat_free(_a);
		tbd_stat_free(_b);
		if(err != 0)
			return err;
	}

	/* Handle deletions. */
	for(i = 0; i < a->size; i++) {
		err = 0;

		struct tbd_stat *_a = tbd_stat_entry(a, i);
		if(_a == NULL)
			return TBD_ERROR(TBD_ERROR_UNABLE_TO_STAT_FILE);
		struct tbd_stat *_b = tbd_stat_entry_find(b, _a->name);

		if (_b == NULL)
			err = tbd_create_cmd_entity_delete(stream, _a->name);

		tbd_stat_free(_b);
		tbd_stat_free(_a);

		if(err != 0)
			return err;
	}

	if(!top && ((err = tbd_create_cmd_dir_leave(stream, b)) != 
	            TBD_ERROR_SUCCESS)) {
		return err;
	}
	return TBD_ERROR_SUCCESS;
}

int
tbd_create(int              stream,
           struct tbd_stat *a,
           struct tbd_stat *b)
{
	int err;
	if((stream < 0) || (a == NULL) || (b == NULL))
		return TBD_ERROR(TBD_ERROR_NULL_POINTER);

	if((err = tbd_create_cmd_ident(stream))        != 0 ||
	   (err = tbd_create_impl(stream, a, b, true)) != 0)
		return err;

	return tbd_create_cmd_update(stream);
}
