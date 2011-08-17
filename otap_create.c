#include "otap.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>



static bool _otap_create_fwrite_cmd(FILE* stream, uint8_t cmd) {
	return (fwrite(&cmd, 1, 1, stream) == 1);
}

static bool _otap_create_fwrite_string(FILE* stream, const char* string) {
	uint16_t slen = strlen(string);
	if(fwrite(&slen, 2, 1, stream) != 1)
		return false;
	return (fwrite(string, 1, slen, stream) == slen);
}

static bool _otap_create_fwrite_mtime(FILE* stream, uint32_t mtime) {
	return (fwrite(&mtime, 4, 1, stream) == 1);
}



static bool _otap_create_cmd_ident(FILE* stream) {
	return (_otap_create_fwrite_cmd(stream, otap_cmd_identify)
		&& _otap_create_fwrite_string(stream, otap_ident));
}

static bool _otap_create_cmd_update(FILE* stream) {
	return _otap_create_fwrite_cmd(stream, otap_cmd_update);
}

static bool _otap_create_cmd_file_create(FILE* stream, file_stat_t* f) {
	if(!_otap_create_fwrite_cmd(stream, otap_cmd_file_create)
		|| !_otap_create_fwrite_string(stream, f->name)
		|| !_otap_create_fwrite_mtime(stream, f->mtime))
		return false;
	
	uint32_t size = f->size;
	if(fwrite(&size, 4, 1, stream) != 1)
		return false;
	
	FILE* fp = file_stat_fopen(f, "rb");
	if(fp == NULL)
		return false;
	
	uint8_t buff[256];
	uintptr_t b = 256;
	for(b = 256; b == 256; ) {
		b = fread(buff, 1, b, fp);
		if(fwrite(buff, 1, b, stream) != b) {
			fclose(fp);
			return false;
		}
	}
	
	return true;
}

static bool _otap_create_cmd_file_delta(FILE* stream, file_stat_t* a, file_stat_t* b) {
	if(!_otap_create_fwrite_cmd(stream, otap_cmd_file_delta)
		|| !_otap_create_fwrite_string(stream, b->name)
		|| !_otap_create_fwrite_mtime(stream, b->mtime))
		return false;
	
	FILE* fpa = file_stat_fopen(a, "rb");
	if(fpa == NULL)
		return false;
	FILE* fpb = file_stat_fopen(b, "rb");
	if(fpb == NULL) {
		fclose(fpa);
		return false;
	}
	
	// Calculate start.
	uintptr_t blks[2] = { 256, 256 };
	uint8_t   buff[2][256];
	
	uintptr_t o;
	for(o = 0; (blks[1] == blks[0]) && (blks[0] != 0); o += blks[1]) {
		blks[0] = fread(buff[0], 1, blks[0], fpa);
		blks[1] = fread(buff[1], 1, blks[1], fpb);
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
	
	if(fwrite(&start, 4, 1, stream) != 1) {
		fclose(fpa); fclose(fpb);
		return false;
	}
	
	if((fseek(fpa, 0, SEEK_END) != 0) || (fseek(fpb, 0, SEEK_END) != 0)) {
		fclose(fpa); fclose(fpb);
		return false;
	}
	
	// TODO - Find end.
	long flena = ftell(fpa);
	long flenb = ftell(fpb);
	
	if((flena < 0) || (flenb < 0)) {
		fclose(fpa); fclose(fpb);
		return false;
	}
	
	blks[0] = 256; blks[1] = 256;
	for(o = 0; true; o += blks[1]) {
		blks[0] = ((flena - o) < 256     ? (flena - o) : 256    );
		blks[1] = ((flenb - o) < blks[0] ? (flenb - o) : blks[0]);
		if((blks[0] == 0) || (blks[1] == 0))
			break;
		
		if((fseek(fpa, flena - (o + blks[0]), SEEK_SET) != 0)
			|| (fseek(fpb, flenb - (o + blks[1]), SEEK_SET) != 0)) {
			fclose(fpa); fclose(fpb);
			return false;
		}
		
		if((fread(buff[0], 1, blks[0], fpa) != blks[0])
			|| (fread(buff[1], 1, blks[1], fpb) != blks[1])) {
			fclose(fpa); fclose(fpb);
			return false;
		}
		
		uintptr_t i, ja, jb;
		for(i = 0, ja = (blks[0] - 1), jb = (blks[1] - 1); i < blks[1]; i++, ja--, jb--) {
			if(buff[0][ja] != buff[1][jb]) {
				o += i;
				break;
			}
		}
		if(i != 0)
			break;
	}
	fclose(fpa);
	
	if((flenb - o) < start)
		o = (flenb - start);

	uint32_t end = (flena - o);
	if(end < start)
		end = start;
	if(fwrite(&end, 4, 1, stream) != 1) {
		fclose(fpb);
		return false;
	}
	
	uint32_t size = (flenb - (o + start));
	if(fwrite(&size, 4, 1, stream) != 1) {
		fclose(fpb);
		return false;
	}
	
	if(fseek(fpb, start, SEEK_SET) != 0) {
		fclose(fpb);
		return false;
	}
	
	for(o = 0; o < size; o += 256) {
		uintptr_t csize = ((size - o) > 256 ? 256 : (size - o));
		if((fread(buff[0], 1, csize, fpb) != csize)
			|| (fwrite(buff[0], 1, csize, stream) != csize)) {
			fclose(fpb);
			return false;
		}
	}
	
	fclose(fpb);
	return true;
}

static bool _otap_create_cmd_dir_create(FILE* stream, dir_stat_t* d) {
	return (_otap_create_fwrite_cmd(stream, otap_cmd_dir_create)
		&& _otap_create_fwrite_string(stream, d->name)
		&& _otap_create_fwrite_mtime(stream, d->mtime));
}

static bool _otap_create_cmd_dir_enter(FILE* stream, const char* name) {
	return (_otap_create_fwrite_cmd(stream, otap_cmd_dir_enter)
		&& _otap_create_fwrite_string(stream, name));
}

static bool _otap_create_cmd_dir_leave(FILE* stream, uintptr_t count) {
	if(count == 0)	
		return true;
	if(!_otap_create_fwrite_cmd(stream, otap_cmd_dir_leave))
		return false;
	
	uint8_t token;
	if(count > 256) {
		token = 255;
		for(; count > 256; count -= 256) {
			if(fwrite(&token, 1, 1, stream) != 1)
				return false;
		}
	}
	
	token = (count - 1);
	return (fwrite(&token, 1, 1, stream) == 1);
}

static bool _otap_create_cmd_entity_delete(FILE* stream, const char* name) {
	return (_otap_create_fwrite_cmd(stream, otap_cmd_entity_delete)
		&& _otap_create_fwrite_string(stream, name));
}



static bool _otap_create_dir(FILE* stream, dir_stat_t* d) {
	if(!_otap_create_cmd_dir_create(stream, d))
		return false;
	if(!_otap_create_cmd_dir_enter(stream, d->name))
		return false;

	uintptr_t i;
	for(i = 0; i < d->dcount; i++) {
		if(!_otap_create_dir(stream, ((dir_stat_t**)d->entries)[i]))
			return false;
	}
	for(; i < (d->dcount + d->fcount); i++) {
		if(!_otap_create_cmd_file_create(stream, ((file_stat_t**)d->entries)[i]))
			return false;
	}
	
	if(!_otap_create_cmd_dir_leave(stream, 1))
		return false;
	return true;
}

static bool _otap_create(FILE* stream, dir_stat_t* a, dir_stat_t* b) {
	uintptr_t i;
	for(i = 0; i < b->dcount; i++) {
		dir_stat_t* _b = ((dir_stat_t**)b->entries)[i];
		dir_stat_t* _a = dir_stat_find_dir(a, _b->name);
		if(_a == NULL) {
			if(!_otap_create_dir(stream, _b))
				return false;
		} else {
			if(!_otap_create_cmd_dir_enter(stream, _b->name))
				return false;
			if(!_otap_create(stream, _a, _b))
				return false;
			if(!_otap_create_cmd_dir_leave(stream, 1))
				return false;
		}
	}
	for(; i < (b->dcount + b->fcount); i++) {
		file_stat_t* _b = ((file_stat_t**)b->entries)[i];
		file_stat_t* _a = dir_stat_find_file(a, _b->name);
		if(_a == NULL) {
			if(!_otap_create_cmd_file_create(stream, _b))
				return false;
		} else {
			if(!_otap_create_cmd_file_delta(stream, _a, _b))
				return false;
		}
	}
	
	// Handle deletions.
	for(i = 0; i < a->dcount; i++) {
		dir_stat_t* _a = ((dir_stat_t**)a->entries)[i];
		dir_stat_t* _b = dir_stat_find_dir(b, _a->name);
		if((_b == NULL) && !_otap_create_cmd_entity_delete(stream, _a->name))
			return false;
	}
	for(; i < (a->dcount + a->fcount); i++) {
		file_stat_t* _a = ((file_stat_t**)a->entries)[i];
		file_stat_t* _b = dir_stat_find_file(b, _a->name);
		if((_b == NULL) && !_otap_create_cmd_entity_delete(stream, _a->name))
			return false;
	}
	return true;
}



bool otap_create(FILE* stream, dir_stat_t* a, dir_stat_t* b) {
	if((stream == NULL) || (a == NULL) || (b == NULL))
		return false;
	
	if(!_otap_create_cmd_ident(stream))
		return false;
		
	if(!_otap_create(stream, a, b))
		return false;
	
	return _otap_create_cmd_update(stream);
}
