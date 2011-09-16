#include "otap.h"
#include "error.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>



static int _otap_create_fwrite_cmd(FILE* stream, uint8_t cmd) {
	if(fwrite(&cmd, 1, 1, stream) != 1)
		otap_error(otap_error_unable_to_write_stream);
	return 0;
}

static int _otap_create_fwrite_string(FILE* stream, const char* string) {
	uint16_t slen = strlen(string);
	if((fwrite(&slen, 2, 1, stream) != 1)
		|| (fwrite(string, 1, slen, stream) != slen))
		otap_error(otap_error_unable_to_write_stream);	
	return 0;
}

static int _otap_create_fwrite_mtime(FILE* stream, uint32_t mtime) {
	if(fwrite(&mtime, 4, 1, stream) != 1)
		otap_error(otap_error_unable_to_write_stream);
	return 0;
}



static int _otap_create_cmd_ident(FILE* stream) {
	int err;
	
	if((err = _otap_create_fwrite_cmd(stream, otap_cmd_identify)) != 0)
		return err;
	if((err = _otap_create_fwrite_string(stream, otap_ident)) != 0)
		return err;
	return 0;
}

static int _otap_create_cmd_update(FILE* stream) {
	return _otap_create_fwrite_cmd(stream, otap_cmd_update);
}

static int _otap_create_cmd_file_create(FILE* stream, otap_stat_t* f) {
	int err;
	if((err = _otap_create_fwrite_cmd(stream, otap_cmd_file_create)) != 0)
		return err;
	if((err = _otap_create_fwrite_string(stream, f->name)) != 0)
		return err;
	if((err = _otap_create_fwrite_mtime(stream, f->mtime)) != 0)
		return err;
	
	uint32_t size = f->size;
	if(fwrite(&size, 4, 1, stream) != 1)
		otap_error(otap_error_unable_to_write_stream);
	
	FILE* fp = otap_stat_fopen(f, "rb");
	if(fp == NULL)
		otap_error(otap_error_unable_to_open_file_for_reading);
	
	uint8_t buff[256];
	uintptr_t b = 256;
	for(b = 256; b == 256; ) {
		b = fread(buff, 1, b, fp);
		if(fwrite(buff, 1, b, stream) != b) {
			fclose(fp);
			otap_error(otap_error_unable_to_write_stream);
		}
	}
	fclose(fp);
	return 0;
}

static int _otap_create_cmd_file_delta(FILE* stream, otap_stat_t* a, otap_stat_t* b) {
	FILE* fpa = otap_stat_fopen(a, "rb");
	if(fpa == NULL)
		otap_error(otap_error_unable_to_open_file_for_reading);
	FILE* fpb = otap_stat_fopen(b, "rb");
	if(fpb == NULL) {
		fclose(fpa);
		otap_error(otap_error_unable_to_open_file_for_reading);
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
		fclose(fpa); fclose(fpb);
		otap_error(otap_error_unable_to_seek_through_stream);
	}
	
	// Find length.
	long flena = ftell(fpa);
	long flenb = ftell(fpb);
	
	if((flena < 0) || (flenb < 0)) {
		fclose(fpa); fclose(fpb);
		otap_error(otap_error_unable_to_detect_stream_position);
	}
	
	// Find end.
	blks[0] = 256; blks[1] = 256;
	for(o = 0; true; o += blks[1]) {
		blks[0] = ((flena - o) < 256     ? (flena - o) : 256    );
		blks[1] = ((flenb - o) < blks[0] ? (flenb - o) : blks[0]);
		if((blks[0] == 0) || (blks[1] == 0))
			break;
		
		if((fseek(fpa, flena - (o + blks[0]), SEEK_SET) != 0)
			|| (fseek(fpb, flenb - (o + blks[1]), SEEK_SET) != 0)) {
			fclose(fpa); fclose(fpb);
			otap_error(otap_error_unable_to_seek_through_stream);
		}
		
		if((fread(buff[0], 1, blks[0], fpa) != blks[0])
			|| (fread(buff[1], 1, blks[1], fpb) != blks[1])) {
			fclose(fpa); fclose(fpb);
			otap_error(otap_error_unable_to_read_stream);
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
		fclose(fpb);
		return 0;
	}
	
	int err;
	if(((err = _otap_create_fwrite_cmd(stream, otap_cmd_file_delta)) != 0)
		|| ((err = _otap_create_fwrite_string(stream, b->name)) != 0)
		|| ((err = _otap_create_fwrite_mtime(stream, b->mtime)) != 0)) {
		fclose(fpb);
		return err;
	}
	if((fwrite(&start, 4, 1, stream) != 1)
		|| (fwrite(&end, 4, 1, stream) != 1)
		|| (fwrite(&size, 4, 1, stream) != 1)) {
		fclose(fpb);
		otap_error(otap_error_unable_to_write_stream);
	}
	if(fseek(fpb, start, SEEK_SET) != 0) {
		fclose(fpb);
		otap_error(otap_error_unable_to_seek_through_stream);
	}
	
	for(o = 0; o < size; o += 256) {
		uintptr_t csize = ((size - o) > 256 ? 256 : (size - o));
		if(fread(buff[0], 1, csize, fpb) != csize) {
			fclose(fpb);
			otap_error(otap_error_unable_to_read_stream);
		}
		if(fwrite(buff[0], 1, csize, stream) != csize) {
			fclose(fpb);
			otap_error(otap_error_unable_to_write_stream);
		}
	}
	
	fclose(fpb);
	return 0;
}

static int _otap_create_cmd_dir_create(FILE* stream, otap_stat_t* d) {
	int err;
	if(((err = _otap_create_fwrite_cmd(stream, otap_cmd_dir_create)) != 0)
		|| ((err = _otap_create_fwrite_string(stream, d->name)) != 0))
		return err;
	return _otap_create_fwrite_mtime(stream, d->mtime);
}

static int _otap_create_cmd_dir_enter(FILE* stream, const char* name) {
	int err;
	if((err = _otap_create_fwrite_cmd(stream, otap_cmd_dir_enter)) != 0)
		return err;
	return _otap_create_fwrite_string(stream, name);
}

static int _otap_create_cmd_dir_leave(FILE* stream, uintptr_t count) {
	if(count == 0)	
		return 0;
	int err;
	if((err = _otap_create_fwrite_cmd(stream, otap_cmd_dir_leave)) != 0)
		return err;
	
	uint8_t token;
	if(count > 256) {
		token = 255;
		for(; count > 256; count -= 256) {
			if(fwrite(&token, 1, 1, stream) != 1)
				otap_error(otap_error_unable_to_write_stream);
		}
	}
	
	token = (count - 1);
	if(fwrite(&token, 1, 1, stream) != 1)
		otap_error(otap_error_unable_to_write_stream);
	return 0;
}

static int _otap_create_cmd_entity_delete(FILE* stream, const char* name) {
	int err;
	if((err = _otap_create_fwrite_cmd(stream, otap_cmd_entity_delete)) != 0)
		return err;
	return _otap_create_fwrite_string(stream, name);
}



static int _otap_create_dir(FILE* stream, otap_stat_t* d) {
	int err;
	if(((err =_otap_create_cmd_dir_create(stream, d)) != 0)
		|| ((err = _otap_create_cmd_dir_enter(stream, d->name)) != 0))
		return err;

	uintptr_t i;
	for(i = 0; i < d->size; i++) {
		otap_stat_t* f = otap_stat_entry(d, i);
		if(f == NULL)
			otap_error(otap_error_unable_to_stat_file);
		switch(f->type) {
			case otap_stat_type_file:
				err = _otap_create_cmd_file_create(stream, f);
				break;
			case otap_stat_type_dir:
				err = _otap_create_dir(stream, f);
				break;
			default:
				otap_stat_free(f);
				otap_error(otap_error_feature_not_implemented);
				break;
		}
		otap_stat_free(f);
		if(err != 0)
			return err;
	}
	
	return _otap_create_cmd_dir_leave(stream, 1);
}

static int _otap_create(FILE* stream, otap_stat_t* a, otap_stat_t* b, bool top) {
	if((a == NULL) && (b == NULL))
		otap_error(otap_error_null_pointer);

	int err;
	if(((b == NULL) || ((a != NULL) && (a->type != b->type)))
		&& ((err = _otap_create_cmd_entity_delete(stream, a->name)) != 0))
		return err;
		
	if((a == NULL) || ((b != NULL) && (a->type != b->type))) {
		switch(b->type) {
			case otap_stat_type_file:
				return _otap_create_cmd_file_create(stream, b);
			case otap_stat_type_dir:
				return _otap_create_dir(stream, b);
			default:
				break;
		}
		otap_error(otap_error_feature_not_implemented);
	}

	if(a->type == otap_stat_type_file)
		return _otap_create_cmd_file_delta(stream, a, b);
	
	if(a->type != otap_stat_type_dir)
		otap_error(otap_error_feature_not_implemented); // TODO - Handle other types of files (incl. symlink);
	
	if(!top && ((err = _otap_create_cmd_dir_enter(stream, b->name)) != 0))
		return err;
	
	// Handle changes/additions.
	uintptr_t i;
	for(i = 0; i < b->size; i++) {
		otap_stat_t* _b = otap_stat_entry(b, i);
		if(_b == NULL)
			otap_error(otap_error_unable_to_stat_file);
		otap_stat_t* _a = otap_stat_entry_find(a, _b->name);
		err = _otap_create(stream, _a, _b, false);
		otap_stat_free(_a);
		otap_stat_free(_b);
		if(err != 0)
			return err;
	}
	
	// Handle deletions.
	for(i = 0; i < a->size; i++) {
		otap_stat_t* _a = otap_stat_entry(a, i);
		if(_a == NULL)
			otap_error(otap_error_unable_to_stat_file);
		otap_stat_t* _b = otap_stat_entry_find(b, _a->name);
		err = (_b != NULL ? 0 : _otap_create_cmd_entity_delete(stream, _a->name));
		otap_stat_free(_b);
		otap_stat_free(_a);
		if(err != 0)
			return err;
	}
	
	if(!top && ((err = _otap_create_cmd_dir_leave(stream, 1)) != 0))
		return err;
	return 0;
}

int otap_create(FILE* stream, otap_stat_t* a, otap_stat_t* b) {
	if((stream == NULL) || (a == NULL) || (b == NULL))
		otap_error(otap_error_null_pointer);
	
	int err;
	if((err = _otap_create_cmd_ident(stream)) != 0)
		return err;
		
	if((err = _otap_create(stream, a, b, true)) != 0)
		return err;
	
	return _otap_create_cmd_update(stream);
}
