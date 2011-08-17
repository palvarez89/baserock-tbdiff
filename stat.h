#ifndef __fdiff_stat_h__
#define __fdiff_stat_h__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>



typedef struct {
	void*    parent;
	char*    name;
	uint32_t mtime;
	uint32_t dcount, fcount;
	void*    entries;
} dir_stat_t;

typedef struct {
	dir_stat_t* parent;
	char*       name;
	uint32_t    mtime;
	uint32_t    size;
} file_stat_t;



extern dir_stat_t*  dir_stat(dir_stat_t* parent, const char* path);
extern void         dir_stat_free(dir_stat_t* dstat);

extern file_stat_t* file_stat(dir_stat_t* parent, const char* path);

extern char* file_stat_path(file_stat_t* fstat);
extern char* dir_stat_path(dir_stat_t* dstat);

extern FILE* file_stat_fopen(file_stat_t* fstat, const char* mode);

extern void file_stat_print(uintptr_t depth, file_stat_t* fstat);
extern void dir_stat_print(uintptr_t depth, dir_stat_t* dstat);

extern dir_stat_t*  dir_stat_find_dir(dir_stat_t* dstat, const char* name);
extern file_stat_t* dir_stat_find_file(dir_stat_t* dstat, const char* name);

#endif
