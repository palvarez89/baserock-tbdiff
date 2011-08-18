#include "otap.h"

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




static bool _otap_apply_identify(FILE* stream) {
	uint8_t cmd;
	if(fread(&cmd, 1, 1, stream) != 1)
		return false;
	if(cmd != otap_cmd_identify)
		return false;
	uint16_t nlen;
	if(fread(&nlen, 2, 1, stream) != 1)
		return false;
	if(strlen(otap_ident) != nlen)
		return false;
	char nstr[nlen];
	if(fread(nstr, 1, nlen, stream) != nlen)
		return false;
	return (strncmp(nstr, otap_ident, nlen) == 0);
}

static bool _otap_apply_cmd_dir_create(FILE* stream) {
	uint16_t dlen;
	if(fread(&dlen, 2, 1, stream) != 1)
		return false;
	char dname[dlen + 1];
	if(fread(dname, 1, dlen, stream) != dlen)
		return false;
	dname[dlen] = '\0';
	printf("cmd_dir_create %s\n", dname);
	if(strchr(dname, '/') != NULL)
		return false;
	
	// TODO - Apply metadata.
	uint32_t mtime;
	if(fread(&mtime, 4, 1, stream) != 1)
		return false;
	
	return (mkdir(dname, (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) == 0);
}

static bool _otap_apply_cmd_dir_enter(FILE* stream, uintptr_t* depth) {
	uint16_t dlen;
	if(fread(&dlen, 2, 1, stream) != 1)
		return false;
	char dname[dlen + 1];
	if(fread(dname, 1, dlen, stream) != dlen)
		return false;
	dname[dlen] = '\0';
	printf("cmd_dir_enter %s\n", dname);
	if((strchr(dname, '/') != NULL) || (strcmp(dname, "..") == 0))
		return false;
	if(depth != NULL)
		(*depth)++;
	return (chdir(dname) == 0);
}

static bool _otap_apply_cmd_dir_leave(FILE* stream, uintptr_t* depth) {
	uint8_t count;
	if(fread(&count, 1, 1, stream) != 1)
		return false;
	uintptr_t rcount = count + 1;
	printf("cmd_dir_leave %"PRIuPTR"\n", rcount);

	if((depth != NULL) && (*depth < rcount))
		return false;
		
	uintptr_t i;
	for(i = 0; i < rcount; i++) {
		if(chdir("..") != 0)
			return false;
	}
	
	if(depth != NULL)
		*depth -= rcount;
	return true;
}



static bool _otap_apply_cmd_file_create(FILE* stream) {
	uint16_t flen;
	if(fread(&flen, 2, 1, stream) != 1)
		return false;
	char fname[flen + 1];
	if(fread(fname, 1, flen, stream) != flen)
		return false;
	fname[flen] = '\0';
	if((strchr(fname, '/') != NULL) || (strcmp(fname, "..") == 0))
		return false;

	uint32_t mtime;
	if(fread(&mtime, 4, 1, stream) != 1)
		return false;
	
	uint32_t fsize;
	if(fread(&fsize, 4, 1, stream) != 1)
		return false;
		
	printf("cmd_file_create %s:%"PRIuPTR"\n", fname, fsize);
	
	FILE* fp = fopen(fname, "rb");
	if(fp != NULL) {
		fclose(fp);
		return false;
	}
	
	fp = fopen(fname, "wb");
	if(fp == NULL)
		return false;
	
	uintptr_t block = 256;
	uint8_t fbuff[block];
	for(; fsize != 0; fsize -= block) {
		if(fsize < block)
			block = fsize;
		if(fread(fbuff, 1, block, stream) != block)
			return false;
		if(fwrite(fbuff, 1, block, fp) != block)
			return false;
	}
	fclose(fp);
	
	// Apply metadata.
	struct utimbuf timebuff = { time(NULL), mtime };
	utime(fname, &timebuff); // Don't care if it succeeds right now.
	
	return true;
}

static bool _otap_apply_cmd_file_delta(FILE* stream) {
	uint16_t flen;
	if(fread(&flen, 2, 1, stream) != 1)
		return false;
	char fname[flen + 1];
	if(fread(fname, 1, flen, stream) != flen)
		return false;
	fname[flen] = '\0';
	printf("cmd_file_delta %s\n", fname);
	if((strchr(fname, '/') != NULL) || (strcmp(fname, "..") == 0))
		return false;

	uint32_t mtime;
	if(fread(&mtime, 4, 1, stream) != 1)
		return false;
	
	FILE* op = fopen(fname, "rb");
	if(op == NULL)
		return false;
	if(remove(fname) != 0) {
		fclose(op);
		return false;
	}
	FILE* np = fopen(fname, "wb");
	if(np == NULL) {
		fclose(op);
		return false;
	}
	
	uint32_t dstart, dend;
	if(fread(&dstart, 4, 1, stream) != 1)
		return false;
	if(fread(&dend, 4, 1, stream) != 1)
		return false;
	
	uintptr_t block;
	uint8_t fbuff[256];
	for(block = 256; dstart != 0; dstart -= block) {
		if(dstart < block)
			block = dstart;
		if(fread(fbuff, 1, block, op) != block)
			return false;
		if(fwrite(fbuff, 1, block, np) != block)
			return false;
	}
	
	uint32_t fsize;
	if(fread(&fsize, 4, 1, stream) != 1)
		return false;
	
	for(block = 256; fsize != 0; fsize -= block) {
		if(fsize < block)
			block = fsize;
		if(fread(fbuff, 1, block, stream) != block)
			return false;
		if(fwrite(fbuff, 1, block, np) != block)
			return false;
	}
	
	if(fseek(op, dend, SEEK_SET) != 0) {
		fclose(np);
		fclose(op);
		return false;
	}
	
	for(block = 256; block != 0;) {
		block = fread(fbuff, 1, block, op);
		if(fwrite(fbuff, 1, block, np) != block)
			return false;
	}
	
	fclose(np);
	fclose(op);
	
	// Apply metadata.
	struct utimbuf timebuff = { time(NULL), mtime };
	utime(fname, &timebuff); // Don't care if it succeeds right now.
	
	return true;
}



static bool __otap_apply_cmd_entity_delete(const char* name) {
	DIR* dp = opendir(name);
	if(dp == NULL) {
		FILE* fp = fopen(name, "rb");
		if(fp != NULL) {
			fclose(fp);
			return (remove(name) == 0);
		}
		return false;
	}
	if(chdir(name) != 0) {
		closedir(dp);
		return false;
	}
	
	struct dirent* entry;
	while((entry = readdir(dp)) != NULL) {
		if((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0))
			continue;
		if(!__otap_apply_cmd_entity_delete(entry->d_name)) {
			closedir(dp);
			chdir("..");
			return false;
		}
	}
	
	closedir(dp);
	chdir("..");
	return (remove(name) == 0);
}
	
static bool _otap_apply_cmd_entity_delete(FILE* stream) {
	uint16_t elen;
	if(fread(&elen, 2, 1, stream) != 1)
		return false;
	char ename[elen + 1];
	if(fread(ename, 1, elen, stream) != elen)
		return false;
	ename[elen] = '\0';
	printf("cmd_entity_delete %s\n", ename);
	if((strchr(ename, '/') != NULL) || (strcmp(ename, "..") == 0))
		return false;
	return __otap_apply_cmd_entity_delete(ename);
}



bool otap_apply(FILE* stream) {
	if(stream == NULL)
		return false;
	
	if(!_otap_apply_identify(stream))
		return false;
		
	uintptr_t depth = 0;
	bool flush = false;
	while(!flush) {
		uint8_t cmd;
		if(fread(&cmd, 1, 1, stream) != 1)
			return false;
		switch(cmd) {
			case otap_cmd_dir_create:
				if(!_otap_apply_cmd_dir_create(stream))
					return false;
				break;
			case otap_cmd_dir_enter:
				if(!_otap_apply_cmd_dir_enter(stream, &depth))
					return false;
				break;
			case otap_cmd_dir_leave:
				if(!_otap_apply_cmd_dir_leave(stream, &depth))
					return false;
				break;
			case otap_cmd_file_create:
				if(!_otap_apply_cmd_file_create(stream))
					return false;
				break;
			case otap_cmd_file_delta:
				if(!_otap_apply_cmd_file_delta(stream))
					return false;
				break;
			case otap_cmd_entity_move:
			case otap_cmd_entity_copy:
				return false; // TODO - Implement.
			case otap_cmd_entity_delete:
				if(!_otap_apply_cmd_entity_delete(stream))
					return false;
				break;
			case otap_cmd_update:
				flush = true;
				break;
			default:
				fprintf(stderr, "Error: Invalid command 0x%02"PRIx8".\n", cmd);
				return false;
		}
	}
	
	return true;
}
