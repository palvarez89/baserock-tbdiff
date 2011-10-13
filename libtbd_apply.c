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
#include <inttypes.h>
#include <time.h>

#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>
#include <unistd.h>
#include <utime.h>

char*
tbd_apply_fread_string(FILE *stream)
{
	uint16_t dlen;
	if(fread(&dlen, sizeof(uint16_t), 1, stream) != 1)
		return NULL;
	char dname[dlen + 1];
	if(fread(dname, 1, dlen, stream) != dlen)
		return NULL;
	dname[dlen] = '\0';

	return strdup(dname);
}

static int
tbd_apply_identify(FILE *stream)
{
	uint8_t cmd;
	if(fread(&cmd, 1, 1, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
	if(cmd != TBD_CMD_IDENTIFY)
		return TBD_ERROR(TBD_ERROR_INVALID_PARAMETER);
	uint16_t nlen;
	if(fread(&nlen, 2, 1, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
	if(strlen(TB_DIFF_PROTOCOL_ID) != nlen)
		return TBD_ERROR(TBD_ERROR_INVALID_PARAMETER);
	char nstr[nlen];
	if(fread(nstr, 1, nlen, stream) != nlen)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
	if(strncmp(nstr, TB_DIFF_PROTOCOL_ID, nlen) != 0)
		return TBD_ERROR(TBD_ERROR_INVALID_PARAMETER);
	return 0;
}

static int
tbd_apply_cmd_dir_create(FILE *stream)
{
	uint16_t dlen;
	if(fread(&dlen, sizeof(uint16_t), 1, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
	char dname[dlen + 1];
	if(fread(dname, 1, dlen, stream) != dlen)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
	dname[dlen] = '\0';
	fprintf(stderr, "cmd_dir_create %s\n", dname);
	if(strchr(dname, '/') != NULL)
		return TBD_ERROR(TBD_ERROR_INVALID_PARAMETER);

	uint32_t mtime;
	if(fread(&mtime, sizeof(uint32_t), 1, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);

	uint32_t uid;
	if(fread(&uid, sizeof(uint32_t), 1, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);

	uint32_t gid;
	if(fread(&gid, sizeof(uint32_t), 1, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);

	uint32_t mode;
	if(fread(&mode, sizeof(uint32_t), 1, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);

	if(mkdir(dname, (mode_t)mode) != 0)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_CREATE_DIR);

	// Apply metadata.
	struct utimbuf timebuff = { time(NULL), mtime };
	utime(dname, &timebuff); // Don't care if it succeeds right now.

	chown(dname, (uid_t)uid, (gid_t)gid);
	chmod (dname, mode);

	return 0;
}

static int
tbd_apply_cmd_dir_enter(FILE      *stream,
                        uintptr_t *depth)
{
	uint16_t dlen;
	if(fread(&dlen, 2, 1, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
	char dname[dlen + 1];
	if(fread(dname, 1, dlen, stream) != dlen)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
	dname[dlen] = '\0';
	fprintf(stderr, "cmd_dir_enter %s\n", dname);
	if((strchr(dname, '/') != NULL) || (strcmp(dname, "..") == 0))
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_CHANGE_DIR);
	if(depth != NULL)
		(*depth)++;

	if(chdir(dname) != 0)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_CHANGE_DIR);
	return 0;
}

static int
tbd_apply_cmd_dir_leave(FILE      *stream,
                        uintptr_t *depth)
{
	uint8_t count;
	if(fread(&count, 1, 1, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
	uintptr_t rcount = count + 1;
	fprintf(stderr, "cmd_dir_leave %"PRIuPTR"\n", rcount);

	if((depth != NULL) && (*depth < rcount))
		return TBD_ERROR(TBD_ERROR_INVALID_PARAMETER);

	uintptr_t i;
	for(i = 0; i < rcount; i++) {
		if(chdir("..") != 0)
			return TBD_ERROR(TBD_ERROR_UNABLE_TO_CHANGE_DIR);
	}

	if(depth != NULL)
		*depth -= rcount;
	return 0;
}

static int
tbd_apply_cmd_file_create(FILE *stream)
{
	uint16_t flen;
	if(fread(&flen, 2, 1, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
	char fname[flen + 1];
	if(fread(fname, 1, flen, stream) != flen)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
	fname[flen] = '\0';
	if((strchr(fname, '/') != NULL) || (strcmp(fname, "..") == 0))
		return TBD_ERROR(TBD_ERROR_INVALID_PARAMETER);

	uint32_t mtime;
	uint32_t mode;
	uint32_t uid;
	uint32_t gid;
	uint32_t fsize;

	if(fread(&mtime, sizeof(uint32_t), 1, stream) != 1 ||
	    fread(&mode,  sizeof(uint32_t), 1, stream) != 1 ||
	    fread(&uid, sizeof(uint32_t), 1, stream)   != 1 ||
	    fread(&gid, sizeof(uint32_t), 1, stream)   != 1 ||
	    fread(&fsize, 4, 1, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);

	fprintf(stderr, "cmd_file_create %s:%"PRId32"\n", fname, fsize);

	FILE *fp = fopen(fname, "rb");
	if(fp != NULL) {
		fclose(fp);
		return TBD_ERROR(TBD_ERROR_FILE_ALREADY_EXISTS);
	}

	fp = fopen(fname, "wb");
	if(fp == NULL)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_OPEN_FILE_FOR_WRITING);

	uintptr_t block = 256;
	uint8_t fbuff[block];
	for(; fsize != 0; fsize -= block) {
		if(fsize < block)
			block = fsize;
		if(fread(fbuff, 1, block, stream) != block)
			return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
		if(fwrite(fbuff, 1, block, fp) != block)
			return TBD_ERROR(TBD_ERROR_UNABLE_TO_WRITE_STREAM);
	}
	fclose(fp);

	// Apply metadata.
	struct utimbuf timebuff = { time(NULL), mtime };

	// Don't care if it succeeds right now.
	utime(fname, &timebuff);
	/* Chown ALWAYS have to be done before chmod */
	chown(fname, (uid_t)uid, (gid_t)gid);
	chmod(fname, mode);

	return 0;
}

static int
tbd_apply_cmd_file_delta(FILE *stream)
{
	uint16_t mdata_mask;
	uint32_t mtime;
	uint32_t uid;
	uint32_t gid;
	uint32_t mode;
	uint16_t flen;
	if(fread(&flen, 2, 1, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
	char fname[flen + 1];
	if(fread(fname, 1, flen, stream) != flen)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
	fname[flen] = '\0';

	fprintf(stderr, "cmd_file_delta %s\n", fname);

	if((strchr(fname, '/') != NULL) ||
	    (strcmp(fname, "..") == 0))
		return TBD_ERROR(TBD_ERROR_INVALID_PARAMETER);

	/* Reading metadata */
	if(fread(&mdata_mask, sizeof(uint16_t), 1, stream) != 1 ||
	    fread(&mtime,      sizeof(uint32_t), 1, stream) != 1 ||
	    fread(&uid,        sizeof(uint32_t), 1, stream) != 1 ||
	    fread(&gid,        sizeof(uint32_t), 1, stream) != 1 ||
	    fread(&mode,       sizeof(uint32_t), 1, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);

	FILE *op = fopen(fname, "rb");
	if(op == NULL)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_OPEN_FILE_FOR_READING);
	if(remove(fname) != 0) {
		fclose(op);
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_REMOVE_FILE);
	}
	FILE *np = fopen(fname, "wb");
	if(np == NULL) {
		fclose(op);
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_OPEN_FILE_FOR_WRITING);
	}

	uint32_t dstart, dend;
	if(fread(&dstart, 4, 1, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
	if(fread(&dend, 4, 1, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);

	uintptr_t block;
	uint8_t fbuff[256];
	for(block = 256; dstart != 0; dstart -= block) {
		if(dstart < block)
			block = dstart;
		if(fread(fbuff, 1, block, op) != block)
			return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
		if(fwrite(fbuff, 1, block, np) != block)
			return TBD_ERROR(TBD_ERROR_UNABLE_TO_WRITE_STREAM);
	}

	uint32_t fsize;
	if(fread(&fsize, 4, 1, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);

	for(block = 256; fsize != 0; fsize -= block) {
		if(fsize < block)
			block = fsize;
		if(fread(fbuff, 1, block, stream) != block)
			return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
		if(fwrite(fbuff, 1, block, np) != block)
			return TBD_ERROR(TBD_ERROR_UNABLE_TO_WRITE_STREAM);
	}

	if(fseek(op, dend, SEEK_SET) != 0) {
		fclose(np);
		fclose(op);
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_SEEK_THROUGH_STREAM);
	}

	for(block = 256; block != 0;) {
		block = fread(fbuff, 1, block, op);
		if(fwrite(fbuff, 1, block, np) != block)
			return TBD_ERROR(TBD_ERROR_UNABLE_TO_WRITE_STREAM);
	}

	fclose(np);
	fclose(op);

	// Apply metadata.
	if(mdata_mask & TBD_METADATA_MTIME) {
		struct utimbuf timebuff = { time(NULL), mtime };
		utime(fname, &timebuff); // Don't care if it succeeds right now.
	}
	if(mdata_mask & TBD_METADATA_UID ||
	    mdata_mask & TBD_METADATA_GID)
		chown(fname, (uid_t)uid, (gid_t)gid);
	if(mdata_mask | TBD_METADATA_MODE)
		chmod(fname, mode);

	return 0;
}

static int tbd_apply_cmd_entity_delete_for_name(const char*);
static int tbd_apply_cmd_dir_delete(const char *name)
{
	int err = TBD_ERROR_SUCCESS;
	DIR *dp;
	struct dirent *entry;
	if ((dp = opendir(name)) == NULL) {
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_REMOVE_FILE);
	}

	if (chdir(name) != 0) {
		closedir(dp);
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_CHANGE_DIR);
	}

	while ((entry = readdir(dp)) != NULL) {
		if ((strcmp(entry->d_name, ".") == 0) ||
		    (strcmp(entry->d_name, "..") == 0)) {
			continue;
		}
		if ((err = tbd_apply_cmd_entity_delete_for_name(entry->d_name))
		    != TBD_ERROR_SUCCESS) {
			goto cleanup;
		}
	}

	if (chdir("..") != 0) {
		err = TBD_ERROR(TBD_ERROR_UNABLE_TO_CHANGE_DIR);
		goto cleanup;
	}
	if (rmdir(name) != 0) {
		err = TBD_ERROR(TBD_ERROR_UNABLE_TO_REMOVE_FILE);
	}
cleanup:
	closedir(dp);
	return err;
}

static int
tbd_apply_cmd_entity_delete_for_name(const char *name)
{
	struct stat info;
	if (lstat(name, &info) != 0) {
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_STAT_FILE);
	}

	if (S_ISDIR(info.st_mode)) {
		return tbd_apply_cmd_dir_delete(name);
	}

	if (unlink(name) != 0) {
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_REMOVE_FILE);
	}
	
	return TBD_ERROR_SUCCESS;
}

static int
tbd_apply_cmd_entity_delete(FILE *stream)
{
	uint16_t elen;
	if(fread(&elen, 2, 1, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
	char ename[elen + 1];
	if(fread(ename, 1, elen, stream) != elen)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
	ename[elen] = '\0';

	fprintf(stderr, "cmd_entity_delete %s\n", ename);

	if((strchr(ename, '/') != NULL) || (strcmp(ename, "..") == 0))
		return TBD_ERROR(TBD_ERROR_INVALID_PARAMETER);
	return tbd_apply_cmd_entity_delete_for_name(ename);
}

static int
tbd_apply_cmd_symlink_create(FILE *stream)
{
	uint16_t len;
	uint32_t mtime;
	uint32_t uid;
	uint32_t gid;

	if(fread(&mtime, sizeof(uint32_t), 1, stream) != 1 ||
	    fread(&uid,   sizeof(uint32_t), 1, stream) != 1 ||
	    fread(&gid,   sizeof(uint32_t), 1, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);

	/* Reading link file name */
	if(fread(&len, sizeof(uint16_t), 1, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);

	char linkname[len + 1];
	linkname[len] = '\0';
	if(fread(linkname, sizeof(char), len, stream) != len)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);

	/* Reading target path */
	if(fread(&len, sizeof(uint16_t), 1, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
	char linkpath[len+1];
	linkpath[len] = '\0';

	if(fread(linkpath, sizeof(char), len, stream) != len)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);

	fprintf(stderr, "cmd_symlink_create %s -> %s\n", linkname, linkpath);

	if(symlink(linkpath, linkname))
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_CREATE_SYMLINK);

	struct timeval tv[2];
	gettimeofday(&tv[0], NULL);
	tv[1].tv_sec = (long) mtime;
	tv[1].tv_usec = 0;

	lutimes(linkname, tv); // Don't care if it succeeds right now.
	lchown(linkname, (uid_t)uid, (uid_t)gid);

	return TBD_ERROR_SUCCESS;
}

static int
tbd_apply_cmd_special_create(FILE *stream)
{
	char *name = tbd_apply_fread_string(stream);
	uint32_t mtime;
	uint32_t mode;
	uint32_t uid;
	uint32_t gid;
	uint32_t dev;

	if(name == NULL ||
	    fread(&mtime, sizeof(uint32_t), 1, stream) != 1 ||
	    fread(&mode, sizeof(uint32_t), 1, stream)  != 1 ||
	    fread(&uid, sizeof(uint32_t), 1, stream)   != 1 ||
	    fread(&gid, sizeof(uint32_t), 1, stream)   != 1 ||
	    fread(&dev, sizeof(uint32_t), 1, stream)   != 1) {
		free(name);
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
	}

	fprintf(stderr, "cmd_special_create %s\n", name);

	if(mknod(name, mode, (dev_t)dev) != 0) {
		free(name);
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_CREATE_SPECIAL_FILE);
	}

	struct utimbuf timebuff = { time(NULL), mtime };
	utime(name, &timebuff); // Don't care if it succeeds right now.

	chown(name, (uid_t)uid, (gid_t)gid);
	chmod(name, mode);

	free(name);
	return TBD_ERROR_SUCCESS;
}

static int
tbd_apply_cmd_dir_delta(FILE *stream)
{
	uint16_t metadata_mask;
	uint32_t mtime;
	uint32_t uid;
	uint32_t gid;
	uint32_t mode;

	if(fread(&metadata_mask, sizeof(uint16_t), 1, stream) != 1 ||
	    fread(&mtime, sizeof(uint32_t), 1, stream)         != 1 ||
	    fread(&uid, sizeof(uint32_t), 1, stream)           != 1 ||
	    fread(&gid, sizeof(uint32_t), 1, stream)           != 1 ||
	    fread(&mode, sizeof(uint32_t), 1, stream)          != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);

	char *dname = tbd_apply_fread_string(stream);
	if(dname == NULL)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);

	fprintf(stderr, "cmd_dir_delta %s\n", dname);

	if(metadata_mask & TBD_METADATA_MTIME) {
		struct utimbuf timebuff = { time(NULL), mtime };
		utime(dname, &timebuff); // Don't care if it succeeds right now.
	}
	if(metadata_mask & TBD_METADATA_UID || metadata_mask & TBD_METADATA_GID)
		chown(dname, (uid_t)uid, (gid_t)gid);
	if(metadata_mask | TBD_METADATA_MODE)
		chmod(dname, mode);

	free(dname);
	return TBD_ERROR_SUCCESS;
}

static int
tbd_apply_cmd_file_mdata_update(FILE *stream)
{
	uint16_t metadata_mask;
	uint32_t mtime;
	uint32_t uid;
	uint32_t gid;
	uint32_t mode;

	if(fread(&metadata_mask, sizeof(uint16_t), 1, stream) != 1 ||
	    fread(&mtime, sizeof(uint32_t), 1, stream)         != 1 ||
	    fread(&uid, sizeof(uint32_t), 1, stream)           != 1 ||
	    fread(&gid, sizeof(uint32_t), 1, stream)           != 1 ||
	    fread(&mode, sizeof(uint32_t), 1, stream)          != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);

	char *dname = tbd_apply_fread_string(stream);
	if(dname == NULL)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);

	fprintf(stderr, "cmd_metadata_update %s\n", dname);

	if(metadata_mask & TBD_METADATA_MTIME) {
		struct utimbuf timebuff = { time(NULL), mtime };
		utime(dname, &timebuff); // Don't care if it succeeds right now.
	}
	if(metadata_mask & TBD_METADATA_UID || metadata_mask & TBD_METADATA_GID)
		chown(dname, (uid_t)uid, (gid_t)gid);
	if(metadata_mask | TBD_METADATA_MODE)
		chmod(dname, mode);

	free(dname);
	return TBD_ERROR_SUCCESS;
}

int
tbd_apply(FILE *stream)
{
	if(stream == NULL)
		return TBD_ERROR(TBD_ERROR_NULL_POINTER);

	int err;
	if((err = tbd_apply_identify(stream)) != 0)
		return err;

	uintptr_t depth = 0;
	bool flush = false;
	while(!flush) {
		uint8_t cmd;
		if(fread(&cmd, 1, 1, stream) != 1)
			return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
		switch(cmd) {
		case TBD_CMD_DIR_CREATE:
			if((err = tbd_apply_cmd_dir_create(stream)) != 0)
				return err;
			break;
		case TBD_CMD_DIR_ENTER:
			if((err = tbd_apply_cmd_dir_enter(stream, &depth)) != 0)
				return err;
			break;
		case TBD_CMD_DIR_LEAVE:
			if((err = tbd_apply_cmd_dir_leave(stream, &depth)) != 0)
				return err;
			break;
		case TBD_CMD_FILE_CREATE:
			if((err = tbd_apply_cmd_file_create(stream)) != 0)
				return err;
			break;
		case TBD_CMD_FILE_DELTA:
			if((err = tbd_apply_cmd_file_delta(stream)) != 0)
				return err;
			break;
		case TBD_CMD_SYMLINK_CREATE:
			if((err = tbd_apply_cmd_symlink_create(stream)) != 0)
				return err;
			break;
		case TBD_CMD_SPECIAL_CREATE:
			if((err = tbd_apply_cmd_special_create(stream)) != 0)
				return err;
			break;
		case TBD_CMD_DIR_DELTA:
			if((err = tbd_apply_cmd_dir_delta(stream)) != 0)
				return err;
			break;
		case TBD_CMD_FILE_METADATA_UPDATE:
			if((err = tbd_apply_cmd_file_mdata_update(stream)) != 0)
				return err;
			break;
		case TBD_CMD_ENTITY_MOVE:
		case TBD_CMD_ENTITY_COPY:
			return TBD_ERROR(TBD_ERROR_FEATURE_NOT_IMPLEMENTED); // TODO - Implement.
		case TBD_CMD_ENTITY_DELETE:
			if((err = tbd_apply_cmd_entity_delete(stream)) != 0)
				return err;
			break;
		case TBD_CMD_UPDATE:
			flush = true;
			break;
		default:
			fprintf(stderr, "Error: Invalid command 0x%02"PRIx8".\n", cmd);
			return TBD_ERROR(TBD_ERROR_INVALID_PARAMETER);
		}
	}

	return 0;
}
