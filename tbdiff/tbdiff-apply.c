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

#include "config.h"

#if HAVE_ATTR_XATTR_H
#include <attr/xattr.h>
#else
#include <sys/xattr.h>
#endif

#include <tbdiff/tbdiff-common.h>
#include <tbdiff/tbdiff-io.h>
#include <tbdiff/tbdiff-private.h>
#include <tbdiff/tbdiff-xattrs.h>

char*
tbd_apply_read_string(FILE *stream)
{
	uint16_t dlen;
	if(tbd_read_uint16(&dlen, stream) != 1)
		return NULL;
	char dname[dlen + 1];
	if(fread(dname, 1, dlen, stream) != dlen)
		return NULL;
	dname[dlen] = '\0';

	return strdup(dname);
}

/* reads a block of data into memory
 * using the address in *data which is assumed to be able to contain *size
 * if it needs more than *size bytes to store the data, *data is reallocated
 * providing initial values of *data = NULL and *size = 0 will force it to
 * allocate the required memory itself
 * do not supply a statically or dynamically allocated buffer unless:
 *  - you can guarantee it is not smaller than the data
 *  - or realloc doesn't free old memory (though this will be a memory leak)
 *  - or your allocator does nothing when asked to free non-allocated memory
 */
int
tbd_apply_read_block(FILE *stream, void **data, size_t *size)
{
	{
		size_t _size;
		if (fread(&_size, 1, sizeof(_size), stream) != sizeof(_size) ) {
			return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
		}
		if (_size > *size) {
			void *allocres = realloc(*data, _size);
			if (allocres == NULL) {
				return TBD_ERROR(TBD_ERROR_OUT_OF_MEMORY);
			}
			*data = allocres;
			*size = _size;
		}
	}

	if (fread(*data, 1, *size, stream) != *size) {
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
	}
	return TBD_ERROR_SUCCESS;
}

static int
tbd_apply_identify(FILE *stream)
{
	tbd_cmd_type cmd;
	if(fread(&cmd, sizeof(tbd_cmd_type), 1, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
	if(cmd != TBD_CMD_IDENTIFY)
		return TBD_ERROR(TBD_ERROR_INVALID_PARAMETER);
	uint16_t nlen;
	if(tbd_read_uint16(&nlen, stream) != 1)
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
	if(tbd_read_uint16(&dlen, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
	char dname[dlen + 1];
	if(fread(dname, 1, dlen, stream) != dlen)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
	dname[dlen] = '\0';
	TBD_DEBUGF("cmd_dir_create %s\n", dname);
	if(strchr(dname, '/') != NULL)
		return TBD_ERROR(TBD_ERROR_INVALID_PARAMETER);

	time_t mtime;
	if(tbd_read_time(&mtime, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);

	uid_t uid;
	if(tbd_read_uid(&uid, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);

	gid_t gid;
	if(tbd_read_gid(&gid, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);

	mode_t mode;
	if(tbd_read_mode(&mode, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);

	if(mkdir(dname, (mode_t)mode) != 0)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_CREATE_DIR);

	/* Apply metadata. */
	struct utimbuf timebuff = { time(NULL), mtime };
	utime(dname, &timebuff); /* Don't care if it succeeds right now. */

	if (chown(dname, (uid_t)uid, (gid_t)gid) < 0)
		TBD_WARN("Failed to change ownership of directory");
	chmod (dname, mode);

	return 0;
}

static int
tbd_apply_cmd_dir_enter(FILE      *stream,
                        uintptr_t *depth)
{
	uint16_t dlen;
	if(tbd_read_uint16(&dlen, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
	char dname[dlen + 1];
	if(fread(dname, 1, dlen, stream) != dlen)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
	dname[dlen] = '\0';
	TBD_DEBUGF("cmd_dir_enter %s\n", dname);
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
	int err = TBD_ERROR_SUCCESS;
	struct utimbuf time;

	if (tbd_read_time(&(time.modtime), stream) != 1) {
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
	}
	time.actime = time.modtime;/* not sure what the best atime to use is */

	TBD_DEBUG("cmd_dir_leave\n");

	/* test for leaving shallowest depth */
	if ((depth != NULL) && (*depth < 1)) {
		return TBD_ERROR(TBD_ERROR_INVALID_PARAMETER);
	}

	if (utime(".", &time) == -1) {
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_CHANGE_DIR);
	}

	if (chdir("..") != 0) {
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_CHANGE_DIR);
	}

	if (depth != NULL) {
		(*depth)--;
	}

	return err;
}

static int
tbd_apply_cmd_file_create(FILE *stream)
{
	uint16_t flen;
	if(tbd_read_uint16(&flen, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
	char fname[flen + 1];
	if(fread(fname, 1, flen, stream) != flen)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
	fname[flen] = '\0';
	if((strchr(fname, '/') != NULL) || (strcmp(fname, "..") == 0))
		return TBD_ERROR(TBD_ERROR_INVALID_PARAMETER);

	time_t mtime;
	uint32_t mode;
	uid_t uid;
	gid_t gid;
	uint32_t fsize;

	if(tbd_read_time  (&mtime, stream) != 1 ||
	   tbd_read_uint32(&mode , stream) != 1 ||
	   tbd_read_uid   (&uid  , stream) != 1 ||
	   tbd_read_gid   (&gid  , stream) != 1 ||
	   tbd_read_uint32(&fsize, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);

	TBD_DEBUGF("cmd_file_create %s:%"PRId32"\n", fname, fsize);

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
		if(fread(fbuff, 1, block, stream) != block) {
			fclose(fp);
			return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
		}
		if(fwrite(fbuff, 1, block, fp) != block) {
			fclose(fp);
			return TBD_ERROR(TBD_ERROR_UNABLE_TO_WRITE_STREAM);
		}
	}
	fclose(fp);

	/* Apply metadata. */
	struct utimbuf timebuff = { time(NULL), mtime };

	/* Don't care if it succeeds right now. */
	utime(fname, &timebuff);
	/* Chown ALWAYS have to be done before chmod */
	if (chown(fname, (uid_t)uid, (gid_t)gid) < 0)
		TBD_WARN("Failed to change ownership of file");
	chmod(fname, mode);

	return 0;
}

static int
tbd_apply_cmd_file_delta(FILE *stream)
{
	uint16_t mdata_mask;
	time_t mtime;
	uid_t uid;
	gid_t gid;
	mode_t mode;
	uint16_t flen;
	int error;

	if(tbd_read_uint16(&flen, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
	char fname[flen + 1];
	if(fread(fname, 1, flen, stream) != flen)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
	fname[flen] = '\0';

	TBD_DEBUGF("cmd_file_delta %s\n", fname);

	if((strchr(fname, '/') != NULL) ||
	    (strcmp(fname, "..") == 0))
		return TBD_ERROR(TBD_ERROR_INVALID_PARAMETER);

	/* Reading metadata */
	if(tbd_read_uint16(&mdata_mask, stream) != 1 ||
	   tbd_read_time  (&mtime     , stream) != 1 ||
	   tbd_read_uid   (&uid       , stream) != 1 ||
	   tbd_read_gid   (&gid       , stream) != 1 ||
	   tbd_read_uint32(&mode      , stream) != 1)
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
	if(tbd_read_uint32(&dstart, stream) != 1) {
		error = TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
		goto tbd_apply_cmd_file_delta_error;
    }
	if(tbd_read_uint32(&dend, stream) != 1) {
		error = TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
		goto tbd_apply_cmd_file_delta_error;
    }

	uintptr_t block;
	uint8_t fbuff[256];
	for(block = 256; dstart != 0; dstart -= block) {
		if(dstart < block)
			block = dstart;
		if(fread(fbuff, 1, block, op) != block) {
			error = TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
			goto tbd_apply_cmd_file_delta_error;
		}
		if(fwrite(fbuff, 1, block, np) != block) {
			error = TBD_ERROR(TBD_ERROR_UNABLE_TO_WRITE_STREAM);
			goto tbd_apply_cmd_file_delta_error;
		}
	}

	uint32_t fsize;
	if(tbd_read_uint32(&fsize, stream) != 1) {
		error = TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
		goto tbd_apply_cmd_file_delta_error;
	}

	for(block = 256; fsize != 0; fsize -= block) {
		if(fsize < block)
			block = fsize;
		if(fread(fbuff, 1, block, stream) != block) {
			error = TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
			goto tbd_apply_cmd_file_delta_error;
		}
		if(fwrite(fbuff, 1, block, np) != block) {
			error = TBD_ERROR(TBD_ERROR_UNABLE_TO_WRITE_STREAM);
			goto tbd_apply_cmd_file_delta_error;
		}
	}

	if(fseek(op, dend, SEEK_SET) != 0) {
		error = TBD_ERROR(TBD_ERROR_UNABLE_TO_SEEK_THROUGH_STREAM);
		goto tbd_apply_cmd_file_delta_error;
	}

	for(block = 256; block != 0;) {
		block = fread(fbuff, 1, block, op);
		if(fwrite(fbuff, 1, block, np) != block) {
			error = TBD_ERROR(TBD_ERROR_UNABLE_TO_WRITE_STREAM);
			goto tbd_apply_cmd_file_delta_error;
		}
	}

	fclose(np);
	fclose(op);

	/* Apply metadata. */
	/* file was removed so old permissions were lost
	 * all permissions need to be reapplied, all were sent in this protocol
	 * if only changed sent will have to save mdata from file before it is
	 * removed, then change that data based on the mask
	 * it will still all have to be reapplied
	 */
	{
		struct utimbuf timebuff; 
		timebuff.modtime = mtime;
		if (time(&(timebuff.actime)) == (time_t)-1) {
			return TBD_ERROR(TBD_ERROR_FAILURE);
		}
		if (utime(fname, &timebuff) == -1) {
			return TBD_ERROR(TBD_ERROR_FAILURE);
		}
		if (chown(fname, (uid_t)uid, (gid_t)gid) == -1) {
			return TBD_ERROR(TBD_ERROR_FAILURE);
		}
		if (chmod(fname, mode) == -1) {
			return TBD_ERROR(TBD_ERROR_FAILURE);
		}
	}

	return 0;

tbd_apply_cmd_file_delta_error:
	fclose(np);
	fclose(op);

	return error;
}

static int
tbd_apply_cmd_entity_delete_for_name(const char*);

static int
tbd_apply_cmd_dir_delete(const char *name)
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
		if ((strcmp(entry->d_name, "." ) == 0) ||
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
	if(tbd_read_uint16(&elen, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
	char ename[elen + 1];
	if(fread(ename, 1, elen, stream) != elen)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
	ename[elen] = '\0';

	TBD_DEBUGF("cmd_entity_delete %s\n", ename);

	if((strchr(ename, '/') != NULL) || (strcmp(ename, "..") == 0))
		return TBD_ERROR(TBD_ERROR_INVALID_PARAMETER);
	return tbd_apply_cmd_entity_delete_for_name(ename);
}

static int
tbd_apply_cmd_symlink_create(FILE *stream)
{
	uint16_t len;
	time_t mtime;
	uid_t uid;
	gid_t gid;

	if(tbd_read_time(&mtime, stream) != 1 ||
	    tbd_read_uid(&uid  , stream) != 1 ||
	    tbd_read_gid(&gid  , stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);

	/* Reading link file name */
	if(tbd_read_uint16(&len, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);

	char linkname[len + 1];
	linkname[len] = '\0';
	if(fread(linkname, sizeof(char), len, stream) != len)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);

	/* Reading target path */
	if(tbd_read_uint16(&len, stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
	char linkpath[len+1];
	linkpath[len] = '\0';

	if(fread(linkpath, sizeof(char), len, stream) != len)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);

	TBD_DEBUGF("cmd_symlink_create %s -> %s\n", linkname, linkpath);

	if(symlink(linkpath, linkname))
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_CREATE_SYMLINK);

	struct timeval tv[2];
	gettimeofday(&tv[0], NULL);
	tv[1].tv_sec = (long) mtime;
	tv[1].tv_usec = 0;

	lutimes(linkname, tv); /* Don't care if it succeeds right now. */
	if (lchown(linkname, (uid_t)uid, (uid_t)gid) < 0)
		TBD_WARN("Failed to change ownership of symlink");

	return TBD_ERROR_SUCCESS;
}

static int
tbd_apply_cmd_special_create(FILE *stream)
{
	char *name = tbd_apply_read_string(stream);
	time_t mtime;
	mode_t mode;
	uid_t uid;
	gid_t gid;
	uint32_t dev;

	if(name == NULL ||
	    tbd_read_time  (&mtime, stream) != 1 ||
	    tbd_read_mode  (&mode , stream) != 1 ||
	    tbd_read_uid   (&uid  , stream) != 1 ||
	    tbd_read_gid   (&gid  , stream) != 1 ||
	    tbd_read_uint32(&dev  , stream) != 1) {
		free(name);
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
	}

	TBD_DEBUGF("cmd_special_create %s\n", name);

	if(mknod(name, mode, (dev_t)dev) != 0) {
		free(name);
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_CREATE_SPECIAL_FILE);
	}

	struct utimbuf timebuff = { time(NULL), mtime };
	utime(name, &timebuff); /* Don't care if it succeeds right now. */

	if (chown(name, (uid_t)uid, (gid_t)gid) < 0)
		TBD_WARN("Failed to change ownership of node");
	chmod(name, mode);

	free(name);
	return TBD_ERROR_SUCCESS;
}

static int
tbd_apply_cmd_dir_delta(FILE *stream)
{
	uint16_t metadata_mask;
	time_t mtime;
	uid_t uid;
	gid_t gid;
	mode_t mode;

	if(tbd_read_uint16(&metadata_mask, stream) != 1 ||
	   tbd_read_time  (&mtime        , stream) != 1 ||
	   tbd_read_uid   (&uid          , stream) != 1 ||
	   tbd_read_gid   (&gid          , stream) != 1 ||
	   tbd_read_uint32(&mode         , stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);

	char *dname = tbd_apply_read_string(stream);
	if(dname == NULL)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);

	TBD_DEBUGF("cmd_dir_delta %s\n", dname);

	if(metadata_mask & TBD_METADATA_MTIME) {
		struct utimbuf timebuff = { time(NULL), mtime };
		utime(dname, &timebuff); /* Don't care if it succeeds right now. */
	}
	if(metadata_mask & TBD_METADATA_UID || metadata_mask & TBD_METADATA_GID) {
		if (chown(dname, (uid_t)uid, (gid_t)gid) < 0)
			TBD_WARN("Failed to change ownership during file modification");
	}
	if(metadata_mask | TBD_METADATA_MODE)
		chmod(dname, mode);

	free(dname);
	return TBD_ERROR_SUCCESS;
}

static int
tbd_apply_cmd_file_mdata_update(FILE *stream)
{
	uint16_t metadata_mask;
	time_t mtime;
	uid_t uid;
	gid_t gid;
	mode_t mode;

	if(tbd_read_uint16(&metadata_mask, stream) != 1 ||
	   tbd_read_time  (&mtime        , stream) != 1 ||
	   tbd_read_uid   (&uid          , stream) != 1 ||
	   tbd_read_gid   (&gid          , stream) != 1 ||
	   tbd_read_uint32(&mode         , stream) != 1)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);

	char *dname = tbd_apply_read_string(stream);
	if(dname == NULL)
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);

	TBD_DEBUGF("cmd_metadata_update %s\n", dname);

	if(metadata_mask & TBD_METADATA_MTIME) {
		struct utimbuf timebuff = { time(NULL), mtime };
		utime(dname, &timebuff); /* Don't care if it succeeds right now. */
	}
	if(metadata_mask & TBD_METADATA_UID || metadata_mask & TBD_METADATA_GID) {
		if (chown(dname, (uid_t)uid, (gid_t)gid) < 0)
			TBD_WARN("Failed to change ownership"
			         " during file attribute modification");
	}
	if(metadata_mask | TBD_METADATA_MODE)
		chmod(dname, mode);

	free(dname);
	return TBD_ERROR_SUCCESS;
}

static int
tbd_apply_cmd_xattrs_update(FILE *stream)
{
	int err = TBD_ERROR_SUCCESS;
	char *fname;
	uint32_t count;
	void *data = NULL;
	size_t dsize = 0;
	/* read the name of the file to operate on */
	if ((fname = tbd_apply_read_string(stream)) == NULL) {
		return TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
	}

	/* remove all attributes in preparation for adding new ones */
	if ((err = tbd_xattrs_removeall(fname)) != TBD_ERROR_SUCCESS) {
		goto cleanup;
	}

	/* read how many attributes to process */
	if (tbd_read_uint32(&count, stream) != 1) {
		err = TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
		goto cleanup;
	}

	/* operate on each attribute */
	while (count > 0) {
		char *aname = tbd_apply_read_string(stream);
		if (aname == NULL) {
			err=TBD_ERROR(TBD_ERROR_UNABLE_TO_READ_STREAM);
			goto cleanup;
		}

		/* read a block of data, reallocating if needed */
		if ((err = tbd_apply_read_block(stream, &data, &dsize))
		    != TBD_ERROR_SUCCESS) {
			free(aname);
			goto cleanup;
		}

		if (lsetxattr(fname, aname, data, dsize, 0) == -1) {
			free(aname);
			goto cleanup;
		}

		count--;
		free(aname);
	}
cleanup:
	free(data);
	free(fname);
	return err;
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
		tbd_cmd_type cmd;
		if(fread(&cmd, sizeof(tbd_cmd_type), 1, stream) != 1)
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
		case TBD_CMD_XATTRS_UPDATE:
			if ((err = tbd_apply_cmd_xattrs_update(stream)) !=
			    TBD_ERROR_SUCCESS) {
				return err;
			}
			break;
		case TBD_CMD_ENTITY_MOVE:
		case TBD_CMD_ENTITY_COPY:
			return TBD_ERROR(TBD_ERROR_FEATURE_NOT_IMPLEMENTED); /* TODO - Implement. */
		case TBD_CMD_ENTITY_DELETE:
			if((err = tbd_apply_cmd_entity_delete(stream)) != 0)
				return err;
			break;
		case TBD_CMD_UPDATE:
			flush = true;
			break;
		default:
			TBD_DEBUGF("Invalid command 0x%02"PRIx8".\n", cmd);
			return TBD_ERROR(TBD_ERROR_INVALID_PARAMETER);
		}
	}

	return 0;
}
