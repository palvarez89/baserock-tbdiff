#include "otap.h"

//#define NDEBUG
#include "error.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <time.h>

#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <utime.h>




static int _otap_apply_identify(FILE* stream) {
	uint8_t cmd;
	if(fread(&cmd, 1, 1, stream) != 1)
		otap_error(otap_error_unable_to_read_stream);
	if(cmd != otap_cmd_identify)
		otap_error(otap_error_invalid_parameter);
	uint16_t nlen;
	if(fread(&nlen, 2, 1, stream) != 1)
		otap_error(otap_error_unable_to_read_stream);
	if(strlen(otap_ident) != nlen)
		otap_error(otap_error_invalid_parameter);
	char nstr[nlen];
	if(fread(nstr, 1, nlen, stream) != nlen)
		otap_error(otap_error_unable_to_read_stream);
	if(strncmp(nstr, otap_ident, nlen) != 0)
		otap_error(otap_error_invalid_parameter);
	return 0;
}

static int _otap_apply_cmd_dir_create(FILE* stream) {
	uint16_t dlen;
	if(fread(&dlen, 2, 1, stream) != 1)
		otap_error(otap_error_unable_to_read_stream);
	char dname[dlen + 1];
	if(fread(dname, 1, dlen, stream) != dlen)
		otap_error(otap_error_unable_to_read_stream);
	dname[dlen] = '\0';
	printf("cmd_dir_create %s\n", dname);
	if(strchr(dname, '/') != NULL)
		otap_error(otap_error_invalid_parameter);
	
	uint32_t mtime;
	if(fread(&mtime, 4, 1, stream) != 1)
		otap_error(otap_error_unable_to_read_stream);
	
	if(mkdir(dname, (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) != 0)
		otap_error(otap_error_unable_to_create_dir);
	
	// Apply metadata.
	struct utimbuf timebuff = { time(NULL), mtime };
	utime(dname, &timebuff); // Don't care if it succeeds right now.
	
	return 0;
}

static int _otap_apply_cmd_dir_enter(FILE* stream, uintptr_t* depth) {
	uint16_t dlen;
	if(fread(&dlen, 2, 1, stream) != 1)
		otap_error(otap_error_unable_to_read_stream);
	char dname[dlen + 1];
	if(fread(dname, 1, dlen, stream) != dlen)
		otap_error(otap_error_unable_to_read_stream);
	dname[dlen] = '\0';
	printf("cmd_dir_enter %s\n", dname);
	if((strchr(dname, '/') != NULL) || (strcmp(dname, "..") == 0))
		otap_error(otap_error_unable_to_change_dir);
	if(depth != NULL)
		(*depth)++;
	if(chdir(dname) != 0)
		otap_error(otap_error_unable_to_change_dir);
	return 0;
}

static int _otap_apply_cmd_dir_leave(FILE* stream, uintptr_t* depth) {
	uint8_t count;
	if(fread(&count, 1, 1, stream) != 1)
		otap_error(otap_error_unable_to_read_stream);
	uintptr_t rcount = count + 1;
	printf("cmd_dir_leave %"PRIuPTR"\n", rcount);

	if((depth != NULL) && (*depth < rcount))
		otap_error(otap_error_invalid_parameter);
		
	uintptr_t i;
	for(i = 0; i < rcount; i++) {
		if(chdir("..") != 0)
			otap_error(otap_error_unable_to_change_dir);
	}
	
	if(depth != NULL)
		*depth -= rcount;
	return 0;
}



static int _otap_apply_cmd_file_create(FILE* stream) {
	uint16_t flen;
	if(fread(&flen, 2, 1, stream) != 1)
		otap_error(otap_error_unable_to_read_stream);
	char fname[flen + 1];
	if(fread(fname, 1, flen, stream) != flen)
		otap_error(otap_error_unable_to_read_stream);
	fname[flen] = '\0';
	if((strchr(fname, '/') != NULL) || (strcmp(fname, "..") == 0))
		otap_error(otap_error_invalid_parameter);

	uint32_t mtime;
	if(fread(&mtime, 4, 1, stream) != 1)
		otap_error(otap_error_unable_to_read_stream);
	
	uint32_t fsize;
	if(fread(&fsize, 4, 1, stream) != 1)
		otap_error(otap_error_unable_to_read_stream);
		
	printf("cmd_file_create %s:%"PRId32"\n", fname, fsize);
	
	FILE* fp = fopen(fname, "rb");
	if(fp != NULL) {
		fclose(fp);
		otap_error(otap_error_file_already_exists);
	}
	
	fp = fopen(fname, "wb");
	if(fp == NULL)
		otap_error(otap_error_unable_to_open_file_for_writing);
	
	uintptr_t block = 256;
	uint8_t fbuff[block];
	for(; fsize != 0; fsize -= block) {
		if(fsize < block)
			block = fsize;
		if(fread(fbuff, 1, block, stream) != block)
			otap_error(otap_error_unable_to_read_stream);
		if(fwrite(fbuff, 1, block, fp) != block)
			otap_error(otap_error_unable_to_write_stream);
	}
	fclose(fp);
	
	// Apply metadata.
	struct utimbuf timebuff = { time(NULL), mtime };
	utime(fname, &timebuff); // Don't care if it succeeds right now.
	
	return 0;
}

static int _otap_apply_cmd_file_delta(FILE* stream) {
	uint16_t flen;
	if(fread(&flen, 2, 1, stream) != 1)
		otap_error(otap_error_unable_to_read_stream);
	char fname[flen + 1];
	if(fread(fname, 1, flen, stream) != flen)
		otap_error(otap_error_unable_to_read_stream);
	fname[flen] = '\0';
	printf("cmd_file_delta %s\n", fname);
	if((strchr(fname, '/') != NULL) || (strcmp(fname, "..") == 0))
		otap_error(otap_error_invalid_parameter);

	uint32_t mtime;
	if(fread(&mtime, 4, 1, stream) != 1)
		otap_error(otap_error_unable_to_read_stream);
	
	FILE* op = fopen(fname, "rb");
	if(op == NULL)
		otap_error(otap_error_unable_to_open_file_for_reading);
	if(remove(fname) != 0) {
		fclose(op);
		otap_error(otap_error_unable_to_remove_file);
	}
	FILE* np = fopen(fname, "wb");
	if(np == NULL) {
		fclose(op);
		otap_error(otap_error_unable_to_open_file_for_writing);
	}
	
	uint32_t dstart, dend;
	if(fread(&dstart, 4, 1, stream) != 1)
		otap_error(otap_error_unable_to_read_stream);
	if(fread(&dend, 4, 1, stream) != 1)
		otap_error(otap_error_unable_to_read_stream);
	
	uintptr_t block;
	uint8_t fbuff[256];
	for(block = 256; dstart != 0; dstart -= block) {
		if(dstart < block)
			block = dstart;
		if(fread(fbuff, 1, block, op) != block)
			otap_error(otap_error_unable_to_read_stream);
		if(fwrite(fbuff, 1, block, np) != block)
			otap_error(otap_error_unable_to_write_stream);
	}
	
	uint32_t fsize;
	if(fread(&fsize, 4, 1, stream) != 1)
		otap_error(otap_error_unable_to_read_stream);
	
	for(block = 256; fsize != 0; fsize -= block) {
		if(fsize < block)
			block = fsize;
		if(fread(fbuff, 1, block, stream) != block)
			otap_error(otap_error_unable_to_read_stream);
		if(fwrite(fbuff, 1, block, np) != block)
			otap_error(otap_error_unable_to_write_stream);
	}
	
	if(fseek(op, dend, SEEK_SET) != 0) {
		fclose(np);
		fclose(op);
		otap_error(otap_error_unable_to_seek_through_stream);
	}
	
	for(block = 256; block != 0;) {
		block = fread(fbuff, 1, block, op);
		if(fwrite(fbuff, 1, block, np) != block)
			otap_error(otap_error_unable_to_write_stream);
	}
	
	fclose(np);
	fclose(op);
	
	// Apply metadata.
	struct utimbuf timebuff = { time(NULL), mtime };
	utime(fname, &timebuff); // Don't care if it succeeds right now.
	
	return 0;
}



static int __otap_apply_cmd_entity_delete(const char* name) {
	DIR* dp = opendir(name);
	if(dp == NULL) {
		FILE* fp = fopen(name, "rb");
		if(fp != NULL) {
			fclose(fp);
			if(remove(name) != 0)
				otap_error(otap_error_unable_to_remove_file);
			return 0;
		}
		otap_error(otap_error_file_does_not_exist);
	}
	if(chdir(name) != 0) {
		closedir(dp);
		otap_error(otap_error_unable_to_change_dir);
	}
	
	struct dirent* entry;
	while((entry = readdir(dp)) != NULL) {
		if((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0))
			continue;
		int err;
		if((err = __otap_apply_cmd_entity_delete(entry->d_name)) != 0) {
			closedir(dp);
			if (chdir("..") != 0)
                                otap_error(otap_error_unable_to_change_dir);
			return err;
		}
	}
	
	closedir(dp);
        if (chdir("..") != 0)
                otap_error(otap_error_unable_to_change_dir);
	if(remove(name) != 0)
		otap_error(otap_error_unable_to_remove_file);
	return 0;
}
	
static int
_otap_apply_cmd_entity_delete(FILE* stream)
{
	uint16_t elen;
	if(fread(&elen, 2, 1, stream) != 1)
		otap_error(otap_error_unable_to_read_stream);
	char ename[elen + 1];
	if(fread(ename, 1, elen, stream) != elen)
		otap_error(otap_error_unable_to_read_stream);
	ename[elen] = '\0';

	printf("cmd_entity_delete %s\n", ename);

	if((strchr(ename, '/') != NULL) || (strcmp(ename, "..") == 0))
		otap_error(otap_error_invalid_parameter);
	return __otap_apply_cmd_entity_delete(ename);
}

static int
_otap_apply_cmd_symlink_create (FILE *stream)
{
	uint16_t len;

	if(fread(&len, sizeof(uint16_t), 1, stream) != 1)
		otap_error(otap_error_unable_to_read_stream);
	char linkname[len + 1];
	linkname[len] = '\0';
		
	if(fread(linkname, sizeof(char), len, stream) != len)
		otap_error(otap_error_unable_to_read_stream);
	
  if(fread(&len, sizeof(uint16_t), 1, stream) != 1)
		otap_error(otap_error_unable_to_read_stream);
  char linkpath[len+1];
  linkpath[len] = '\0';

 	if(fread(linkpath, sizeof(char), len, stream) != len)
		otap_error(otap_error_unable_to_read_stream);
  
  fprintf (stderr, "cmd_symlink_create %s -> %s\n", linkname, linkpath);

  return otap_error_success;
}

int otap_apply(FILE* stream) {
	if(stream == NULL)
		otap_error(otap_error_null_pointer);
	
	int err;
	if((err = _otap_apply_identify(stream)) != 0)
		return err;
		
	uintptr_t depth = 0;
	bool flush = false;
	while(!flush) {
		uint8_t cmd;
		if(fread(&cmd, 1, 1, stream) != 1)
			otap_error(otap_error_unable_to_read_stream);
		switch(cmd) {
			case otap_cmd_dir_create:
				if((err = _otap_apply_cmd_dir_create(stream)) != 0)
					return err;
				break;
			case otap_cmd_dir_enter:
				if((err = _otap_apply_cmd_dir_enter(stream, &depth)) != 0)
					return err;
				break;
			case otap_cmd_dir_leave:
				if((err = _otap_apply_cmd_dir_leave(stream, &depth)) != 0)
					return err;
				break;
			case otap_cmd_file_create:
				if((err = _otap_apply_cmd_file_create(stream)) != 0)
					return err;
				break;
			case otap_cmd_file_delta:
				if((err = _otap_apply_cmd_file_delta(stream)) != 0)
					return err;
				break;
		  case otap_cmd_symlink_create:
		    if ((err = _otap_apply_cmd_symlink_create(stream)) != 0)
		      return err;
		    break;
			case otap_cmd_entity_move:
			case otap_cmd_entity_copy:
				otap_error(otap_error_feature_not_implemented); // TODO - Implement.
			case otap_cmd_entity_delete:
				if((err = _otap_apply_cmd_entity_delete(stream)) != 0)
					return err;
				break;
			case otap_cmd_update:
				flush = true;
				break;
			default:
				fprintf(stderr, "Error: Invalid command 0x%02"PRIx8".\n", cmd);
				otap_error(otap_error_invalid_parameter);
		}
	}
	
	return 0;
}
