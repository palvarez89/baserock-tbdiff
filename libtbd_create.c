/*
 *    Copyright(C) 2011 Codethink Ltd.
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

#include "tbdiff.h"
#include "tbdiff-private.h"


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#define PATH_BUFFER_LENGTH 4096

static int
tbd_create_fwrite_cmd(FILE    *stream,
                      uint8_t  cmd)
{
	if(fwrite(&cmd, 1, 1, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_WRITE_STREAM);
	return 0;
}

static int
tbd_create_fwrite_string(FILE       *stream,
                         const char *string)
{
	uint16_t slen = strlen(string);
	if((fwrite(&slen, 2, 1, stream) != 1)
	    || (fwrite(string, 1, slen, stream) != slen))
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_WRITE_STREAM);
	return 0;
}

static int
tbd_create_fwrite_mdata_mask(FILE    *stream,
                             uint16_t mask)
{
	if(fwrite(&mask, sizeof(uint16_t), 1, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_WRITE_STREAM);
	return 0;
}

static int
tbd_create_fwrite_mtime(FILE    *stream,
                        uint32_t  mtime)
{
	if(fwrite(&mtime, sizeof(uint32_t), 1, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_WRITE_STREAM);
	return 0;
}

static int
tbd_create_fwrite_mode(FILE     *stream,
                       uint32_t  mode)
{
	if(fwrite(&mode, sizeof(uint32_t), 1, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_WRITE_STREAM);
	return 0;
}

static int
tbd_create_fwrite_gid(FILE     *stream,
                      gid_t     gid)
{
	if(fwrite(&gid, sizeof(gid_t), 1, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_WRITE_STREAM);
	return 0;
}

static int
tbd_create_fwrite_uid(FILE     *stream,
                      uid_t     uid)
{
	if(fwrite(&uid, sizeof(uid_t), 1, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_WRITE_STREAM);
	return 0;
}

static int
tbd_create_fwrite_dev(FILE     *stream,
                      uint32_t  dev)
{
	if(fwrite(&dev, sizeof(uint32_t), 1, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_WRITE_STREAM);
	return 0;
}

static int
tbd_create_cmd_ident(FILE *stream)
{
	int err;

	if((err = tbd_create_fwrite_cmd(stream, TBD_CMD_IDENTIFY)) != 0)
		return err;
	if((err = tbd_create_fwrite_string(stream, TB_DIFF_PROTOCOL_ID)) != 0)
		return err;
	return 0;
}

static int
tbd_create_cmd_update(FILE *stream)
{
	return tbd_create_fwrite_cmd(stream, TBD_CMD_UPDATE);
}

static int
tbd_create_cmd_file_create(FILE       *stream,
                           tbd_stat_t *f)
{
	int err;
	if((err = tbd_create_fwrite_cmd(stream, TBD_CMD_FILE_CREATE)) != 0 ||
	   (err = tbd_create_fwrite_string(stream, f->name))  != 0 ||
	   (err = tbd_create_fwrite_mtime (stream, f->mtime)) != 0 ||
	   (err = tbd_create_fwrite_mode  (stream, f->mode))  != 0 ||
	   (err = tbd_create_fwrite_uid   (stream, f->uid))   != 0 ||
	   (err = tbd_create_fwrite_gid   (stream, f->gid))   != 0)
		return err;

	uint32_t size = f->size;
	if(fwrite(&size, sizeof(uint32_t), 1, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_WRITE_STREAM);

	FILE *fp = tbd_stat_fopen(f, "rb");
	if(fp == NULL)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_OPEN_FILE_FOR_READING);

	uint8_t buff[256];
	uintptr_t b = 256;
	for(b = 256; b == 256; ) {
		b = fread(buff, 1, b, fp);
		if(fwrite(buff, 1, b, stream) != b) {
			fclose(fp);
			return TBD_ERROR(TBD_ERROR_UNABLE_TO_WRITE_STREAM);
		}
	}
	fclose(fp);

	return 0;
}

static uint16_t
tbd_metadata_mask(tbd_stat_t *a,
                  tbd_stat_t *b)
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
tbd_create_cmd_file_metadata_update(FILE        *stream,
                                    tbd_stat_t *a,
                                    tbd_stat_t *b)
{
	int err;
	uint16_t metadata_mask = tbd_metadata_mask(a, b);

	if(metadata_mask == TBD_METADATA_NONE)
		return 0;
	/* TODO: Optimize protocol by only sending useful metadata */
	if((err = tbd_create_fwrite_cmd(stream, TBD_CMD_FILE_METADATA_UPDATE)) != 0 ||
	   (err = tbd_create_fwrite_mdata_mask (stream, metadata_mask))         != 0 ||
	   (err = tbd_create_fwrite_mtime      (stream, b->mtime))              != 0 ||
	   (err = tbd_create_fwrite_uid        (stream, b->uid))                != 0 ||
	   (err = tbd_create_fwrite_gid        (stream, b->gid))                != 0 ||
	   (err = tbd_create_fwrite_mode       (stream, b->mode))               != 0)
		return err;

	return tbd_create_fwrite_string(stream, b->name);
}

static int
tbd_create_cmd_file_delta(FILE        *stream,
                          tbd_stat_t  *a,
                          tbd_stat_t  *b)
{
	FILE *fpa = tbd_stat_fopen(a, "rb");
	if(fpa == NULL)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_OPEN_FILE_FOR_READING);
	FILE *fpb = tbd_stat_fopen(b, "rb");
	if(fpb == NULL) {
		fclose(fpa);
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_OPEN_FILE_FOR_READING);
	}

	// Calculate start.
	uintptr_t blks[2] = { 256, 256 };
	uint8_t   buff[2][256];

	uintptr_t o;
	for(o = 0; (blks[1] == blks[0]) && (blks[0] != 0); o += blks[1]) {
		blks[0] = fread(buff[0], 1, blks[0], fpa);
		blks[1] = fread(buff[1], 1, blks[0], fpb);
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

	if((fseek(fpa, 0, SEEK_END) != 0) || (fseek(fpb, 0, SEEK_END) != 0)) {
		fclose(fpa);
		fclose(fpb);
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_SEEK_THROUGH_STREAM);
	}

	// Find length.
	long flena = ftell(fpa);
	long flenb = ftell(fpb);

	if((flena < 0) || (flenb < 0)) {
		fclose(fpa);
		fclose(fpb);
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_DETECT_STREAM_POSITION);
	}

	// Find end.
	blks[0] = 256;
	blks[1] = 256;
	for(o = 0; true; o += blks[1]) {
		blks[0] = ((flena - o) < 256     ? (flena - o) : 256    );
		blks[1] = ((flenb - o) < blks[0] ? (flenb - o) : blks[0]);
		if((blks[0] == 0) || (blks[1] == 0))
			break;

		if((fseek(fpa, flena - (o + blks[0]), SEEK_SET) != 0)
		    || (fseek(fpb, flenb - (o + blks[1]), SEEK_SET) != 0)) {
			fclose(fpa);
			fclose(fpb);
			return TBD_ERROR(TBD_ERROR_UNABLE_TO_SEEK_THROUGH_STREAM);
		}

		if((fread(buff[0], 1, blks[0], fpa) != blks[0])
		    || (fread(buff[1], 1, blks[1], fpb) != blks[1])) {
			fclose(fpa);
			fclose(fpb);
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
	fclose(fpa);

	// Ensure that the start and end don't overlap for the new file.
	if((flenb - o) < start)
		o = (flenb - start);

	uint32_t end = (flena - o);
	if(end < start)
		end = start;

	uint32_t size = flenb - ((flena - end) + start); //(flenb - (o + start));

	if((end == start) && (size == 0)) {
		tbd_create_cmd_file_metadata_update(stream, a, b);
		fclose(fpb);
		return 0;
	}

	uint16_t metadata_mask = tbd_metadata_mask(a, b);

	/* TODO: Optimize protocol by only sending useful metadata */
	int err;
	if(((err = tbd_create_fwrite_cmd(stream, TBD_CMD_FILE_DELTA))  != 0) ||
	    ((err = tbd_create_fwrite_string(stream, b->name))           != 0) ||
	    ((err = tbd_create_fwrite_mdata_mask(stream, metadata_mask)) != 0) ||
	    ((err = tbd_create_fwrite_mtime (stream, b->mtime))          != 0) ||
	    ((err = tbd_create_fwrite_uid   (stream, b->uid))            != 0) ||
	    ((err = tbd_create_fwrite_gid   (stream, b->gid))            != 0) ||
	    ((err = tbd_create_fwrite_mode  (stream, b->mode))           != 0)) {
		fclose(fpb);
		return err;
	}
	if((fwrite(&start, sizeof(uint32_t), 1, stream) != 1) ||
	    (fwrite(&end,   sizeof(uint32_t), 1, stream) != 1)   ||
	    (fwrite(&size,  sizeof(uint32_t), 1, stream) != 1)) {
		fclose(fpb);
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_WRITE_STREAM);
	}
	if(fseek(fpb, start, SEEK_SET) != 0) {
		fclose(fpb);
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_SEEK_THROUGH_STREAM);
	}

	for(o = 0; o < size; o += 256) {
		uintptr_t csize = ((size - o) > 256 ? 256 : (size - o));
		if(fread(buff[0], 1, csize, fpb) != csize) {
			fclose(fpb);
			return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
		}
		if(fwrite(buff[0], 1, csize, stream) != csize) {
			fclose(fpb);
			return TBD_ERROR(TBD_ERROR_UNABLE_TO_WRITE_STREAM);
		}
	}

	fclose(fpb);
	return 0;
}

static int
tbd_create_cmd_dir_create(FILE        *stream,
                          tbd_stat_t  *d)
{
	int err;

	if((err = tbd_create_fwrite_cmd(stream, TBD_CMD_DIR_CREATE)) != 0 ||
	   (err = tbd_create_fwrite_string(stream, d->name)) != 0 ||
	   (err = tbd_create_fwrite_mtime(stream, d->mtime)) != 0 ||
	   (err = tbd_create_fwrite_uid(stream, d->uid))     != 0 ||
	   (err = tbd_create_fwrite_gid(stream, d->gid))     != 0)
		return err;

	return tbd_create_fwrite_mode(stream, d->mode);
}

static int
tbd_create_cmd_dir_enter(FILE       *stream,
                         const char *name)
{
	int err;
	if((err = tbd_create_fwrite_cmd(stream, TBD_CMD_DIR_ENTER)) != 0)
		return err;
	return tbd_create_fwrite_string(stream, name);
}

static int
tbd_create_cmd_dir_leave(FILE      *stream,
                         uintptr_t  count)
{
	if(count == 0)
		return 0;
	int err;
	if((err = tbd_create_fwrite_cmd(stream, TBD_CMD_DIR_LEAVE)) != 0)
		return err;

	uint8_t token;
	if(count > 256) {
		token = 255;
		for(; count > 256; count -= 256) {
			if(fwrite(&token, sizeof (uint8_t), 1, stream) != 1)
				return TBD_ERROR(TBD_ERROR_UNABLE_TO_WRITE_STREAM);
		}
	}

	token = (count - 1);
	if(fwrite(&token, 1, 1, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_WRITE_STREAM);
	return 0;
}

static int
tbd_create_cmd_entity_delete(FILE       *stream,
                             const char *name)
{
	int err;
	if((err = tbd_create_fwrite_cmd(stream, TBD_CMD_ENTITY_DELETE)) != 0)
		return err;
	return tbd_create_fwrite_string(stream, name);
}

static int
tbd_create_cmd_dir_delta(FILE        *stream,
                         tbd_stat_t  *a,
                         tbd_stat_t  *b)
{
	int err;
	uint16_t metadata_mask = tbd_metadata_mask(a, b);

	if(metadata_mask == TBD_METADATA_NONE)
		return 0;

	if((err = tbd_create_fwrite_cmd(stream, TBD_CMD_DIR_DELTA))    != 0 ||
	   (err = tbd_create_fwrite_mdata_mask (stream, metadata_mask)) != 0 ||
	   (err = tbd_create_fwrite_mtime      (stream, b->mtime))      != 0 ||
	   (err = tbd_create_fwrite_uid        (stream, b->uid))        != 0 ||
	   (err = tbd_create_fwrite_gid        (stream, b->gid))        != 0 ||
	   (err = tbd_create_fwrite_mode       (stream, b->mode))       != 0)
		return err;

	return tbd_create_fwrite_string(stream, b->name);
}

static int
tbd_create_cmd_symlink_create(FILE        *stream,
                              tbd_stat_t  *symlink)
{
	int err;
	char path[PATH_BUFFER_LENGTH];

	char *slpath = tbd_stat_path(symlink);
	ssize_t len = readlink(slpath, path, sizeof(path)-1);
	free(slpath);
	if(len < 0)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_SYMLINK);
	path[len] = '\0';

	if((err = tbd_create_fwrite_cmd(stream, TBD_CMD_SYMLINK_CREATE)) != 0 ||
	   (err = tbd_create_fwrite_mtime (stream, symlink->mtime)) != 0 ||
	   (err = tbd_create_fwrite_uid   (stream, symlink->uid)) != 0 ||
	   (err = tbd_create_fwrite_gid   (stream, symlink->gid)) != 0 ||
	   (err = tbd_create_fwrite_string(stream, symlink->name)) != 0)
		return err;

	return tbd_create_fwrite_string(stream, path);
}

static int
tbd_create_cmd_symlink_delta(FILE        *stream,
                             tbd_stat_t  *a,
                             tbd_stat_t  *b)
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
tbd_create_cmd_special_create(FILE        *stream,
                              tbd_stat_t  *nod)
{
	int err;

	if((err = tbd_create_fwrite_cmd(stream, TBD_CMD_SPECIAL_CREATE)) != 0 ||
	   (err = tbd_create_fwrite_string(stream, nod->name))            != 0 ||
	   (err = tbd_create_fwrite_mtime (stream, nod->mtime))           != 0 ||
	   (err = tbd_create_fwrite_mode  (stream, nod->mode))             != 0 ||
	   (err = tbd_create_fwrite_uid   (stream, nod->uid))               != 0 ||
	   (err = tbd_create_fwrite_gid   (stream, nod->gid))               != 0)
		return err;
	return tbd_create_fwrite_dev(stream, nod->rdev);
}

static int
tbd_create_cmd_special_delta(FILE        *stream,
                             tbd_stat_t  *a,
                             tbd_stat_t  *b)
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
tbd_create_cmd_socket_create(FILE       *stream,
                             tbd_stat_t *nod)
{
	(void)stream;
	(void)nod;
	return TBD_ERROR(TBD_ERROR_UNABLE_TO_CREATE_SOCKET_FILE);
}

static int
tbd_create_cmd_socket_delta(FILE       *stream,
                            tbd_stat_t *a,
                            tbd_stat_t *b)
{
	(void)stream;
	(void)a;
	(void)b;
	return TBD_ERROR(TBD_ERROR_UNABLE_TO_CREATE_SOCKET_FILE);
}

static int
tbd_create_dir(FILE        *stream,
               tbd_stat_t  *d)
{
	int err;
	if(((err =tbd_create_cmd_dir_create(stream, d))       != 0) ||
	   ((err = tbd_create_cmd_dir_enter(stream, d->name)) != 0))
		return err;

	uintptr_t i;
	for(i = 0; i < d->size; i++) {
		tbd_stat_t *f = tbd_stat_entry(d, i);

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

	return tbd_create_cmd_dir_leave(stream, 1);
}

static int
tbd_create_impl(FILE        *stream,
                tbd_stat_t  *a,
                tbd_stat_t  *b,
                bool         top)
{
	if((a == NULL) && (b == NULL))
		return TBD_ERROR(TBD_ERROR_NULL_POINTER);

	int err;
	if(((b == NULL) || ((a != NULL) && (a->type != b->type)))) {
		fprintf(stderr, "file delete %s\n", a->name);
		if((err = tbd_create_cmd_entity_delete(stream, a->name)) != 0)
			return err;
	}

	if((a == NULL) || ((b != NULL) && (a->type != b->type))) {
		switch(b->type) {
		case TBD_STAT_TYPE_FILE:
			fprintf(stderr, "file new %s\n", b->name);
			return tbd_create_cmd_file_create(stream, b);
		case TBD_STAT_TYPE_DIR:
			fprintf(stderr, "dir new %s\n", b->name);
			return tbd_create_dir(stream, b);
		case TBD_STAT_TYPE_SYMLINK:
			fprintf(stderr, "symlink new %s\n", b->name);
			return tbd_create_cmd_symlink_create(stream, b);
		case TBD_STAT_TYPE_CHRDEV:
		case TBD_STAT_TYPE_BLKDEV:
		case TBD_STAT_TYPE_FIFO:
			fprintf(stderr, "special new %s\n", b->name);
			return tbd_create_cmd_special_create(stream, b);
		case TBD_STAT_TYPE_SOCKET:
			fprintf(stderr, "socket new %s\n", b->name);
			return tbd_create_cmd_socket_create(stream, b);
		default:
			return TBD_ERROR(TBD_ERROR_FEATURE_NOT_IMPLEMENTED);
		}
	}

	switch(b->type) {
	case TBD_STAT_TYPE_FILE:
		fprintf(stderr, "file delta %s\n", a->name);
		return tbd_create_cmd_file_delta(stream, a, b);
	case TBD_STAT_TYPE_SYMLINK:
		fprintf(stderr, "symlink delta %s\n", a->name);
		return tbd_create_cmd_symlink_delta(stream, a, b);
	case TBD_STAT_TYPE_CHRDEV:
	case TBD_STAT_TYPE_BLKDEV:
	case TBD_STAT_TYPE_FIFO:
		fprintf(stderr, "special delta %s\n", a->name);
		return tbd_create_cmd_special_delta(stream, a, b);
	case TBD_STAT_TYPE_SOCKET:
		fprintf(stderr, "socket delta %s\n", a->name);
		return tbd_create_cmd_socket_delta(stream, a, b);
	case TBD_STAT_TYPE_DIR:
		if(!top) {
			fprintf(stderr, "dir delta %s\n", a->name);
			tbd_create_cmd_dir_delta(stream, a, b);
		}
		break;
	default:
		break;
	}

	if(!top && ((err = tbd_create_cmd_dir_enter(stream, b->name)) != 0))
		return err;

	// Handle changes/additions.
	uintptr_t i;
	for(i = 0; i < b->size; i++) {
		tbd_stat_t *_b = tbd_stat_entry(b, i);
		if(_b == NULL)
			return TBD_ERROR(TBD_ERROR_UNABLE_TO_STAT_FILE);
		tbd_stat_t *_a = tbd_stat_entry_find(a, _b->name);
		err = tbd_create_impl(stream, _a, _b, false);
		tbd_stat_free(_a);
		tbd_stat_free(_b);
		if(err != 0)
			return err;
	}

	// Handle deletions.
	for(i = 0; i < a->size; i++) {
		err = 0;
		
		tbd_stat_t *_a = tbd_stat_entry(a, i);
		if(_a == NULL)
			return TBD_ERROR(TBD_ERROR_UNABLE_TO_STAT_FILE);
		tbd_stat_t *_b = tbd_stat_entry_find(b, _a->name);

		if (_b == NULL)
			err = tbd_create_cmd_entity_delete(stream, _a->name);

		tbd_stat_free(_b);
		tbd_stat_free(_a);

		if(err != 0)
			return err;
	}

	if(!top && ((err = tbd_create_cmd_dir_leave(stream, 1)) != 0))
		return err;
	return 0;
}

int
tbd_create(FILE        *stream,
            tbd_stat_t *a,
            tbd_stat_t *b)
{
	int err;
	if((stream == NULL) || (a == NULL) || (b == NULL))
		return TBD_ERROR(TBD_ERROR_NULL_POINTER);

	if((err = tbd_create_cmd_ident(stream))        != 0 ||
	   (err = tbd_create_impl(stream, a, b, true)) != 0)
		return err;

	return tbd_create_cmd_update(stream);
}
