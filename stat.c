#include "stat.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>



static bool _isdir(const char* path) {
	DIR* dp = opendir(path);
	if(dp == NULL)
		return false;
	closedir(dp);
	return true;
}

file_stat_t* file_stat(dir_stat_t* parent, const char* path) {
	struct stat st;
	if(stat(path, &st) < 0)
		return NULL;
	
	uintptr_t pathlen = strlen(path);	
	file_stat_t* fstat = (file_stat_t*)malloc(sizeof(file_stat_t) + (pathlen + 1));
	if(fstat == NULL)
		return NULL;
	
	fstat->parent = parent;
	fstat->name   = (char*)((uintptr_t)fstat + sizeof(file_stat_t));
	memcpy(fstat->name, path, (pathlen + 1));
	fstat->mtime = st.st_mtime;
	fstat->size  = st.st_size;
	
	return fstat;
}

dir_stat_t* dir_stat(dir_stat_t* parent, const char* path) {
	if((parent != NULL) && ((strcmp(path, ".") == 0) || (strcmp(path, "..") == 0)))
		return NULL;

	DIR* dp = opendir(path);
	if(dp == NULL)
		return NULL;
	chdir(path);
	
	uint32_t dcount = 0, fcount = 0;
	struct dirent* entry;
	while((entry = readdir(dp)) != NULL) {
		if((strcmp(entry->d_name, "..") == 0) || (strcmp(entry->d_name, ".") == 0))
			continue;
		if(_isdir(entry->d_name))
			dcount++;
		else
			fcount++;
	}
	
	uintptr_t pathlen = strlen(path);
	dir_stat_t* dstat = (dir_stat_t*)malloc(sizeof(dir_stat_t) + (sizeof(void*) * (dcount + fcount)) + (pathlen + 1));
	if(dstat == NULL) {
		closedir(dp);
		chdir("..");
		return NULL;
	}
	
	dstat->parent  = parent;
	dstat->entries = (void*)((uintptr_t)dstat + sizeof(dir_stat_t));
	dstat->name    = (char*)((uintptr_t)dstat->entries + (sizeof(void*) * (dcount + fcount)));
	memcpy(dstat->name, path, (pathlen + 1));
	dstat->dcount  = dcount;
	dstat->fcount  = fcount;
	
	rewinddir(dp);
	
	uintptr_t f = dcount, d = 0;
	while((entry = readdir(dp)) != NULL) {
		if((strcmp(entry->d_name, "..") == 0) || (strcmp(entry->d_name, ".") == 0))
				continue;
		if(_isdir(entry->d_name))
			((dir_stat_t**)dstat->entries)[d++] = dir_stat(dstat, entry->d_name);
		else
			((file_stat_t**)dstat->entries)[f++] = file_stat(dstat, entry->d_name);
	}
	
	closedir(dp);
	chdir("..");
	
	struct stat st;
	if(stat(path, &st) >= 0)
		dstat->mtime = st.st_mtime;
	else
		dstat->mtime = 0;

	return dstat;
}

void dir_stat_free(dir_stat_t* dstat) {
	if(dstat == NULL)
		return;
	if(dstat->parent != NULL) {	
		dir_stat_free(dstat->parent);
		return;
	}
	
	uintptr_t i;
	for(i = 0; i < dstat->dcount; i++)
		dir_stat_free(((dir_stat_t**)dstat->entries)[i]);
	for(; i < (dstat->dcount + dstat->fcount); i++)
		free(((file_stat_t**)dstat->entries)[i]);
	free(dstat);
}



void file_stat_print(uintptr_t depth, file_stat_t* fstat) {
	if(fstat == NULL)
		return;
	
	uintptr_t i;
	for(i = 0; i < depth; i++)
		printf("\t");
	printf("[%s %"PRIu32" %"PRIu32"]\n", fstat->name, fstat->size, fstat->mtime);
}

void dir_stat_print(uintptr_t depth, dir_stat_t* dstat) {
	if(dstat == NULL)
		return;

	uintptr_t i;
	for(i = 0; i < depth; i++)
		printf("\t");
	printf("%s/\n", dstat->name);
	
	for(i = 0; i < dstat->dcount; i++)
		dir_stat_print((depth + 1), ((dir_stat_t**)dstat->entries)[i]);
	for(; i < (dstat->dcount + dstat->fcount); i++)
		file_stat_print((depth + 1), ((file_stat_t**)dstat->entries)[i]);
}

char* file_stat_path(file_stat_t* fstat) {
	if(fstat == NULL)
		return NULL;
		
	dir_stat_t* root;
	uintptr_t pathlen;
	for(root = fstat->parent, pathlen = strlen(fstat->name);
		root != NULL;
		pathlen += (strlen(root->name) + 1), root = (dir_stat_t*)root->parent);

	char* ret = (char*)malloc(pathlen + 1);
	if(ret == NULL)
		return NULL;
	ret[pathlen] = '\0';
	
	char* ptr = &ret[pathlen];
	{
		uintptr_t nlen = strlen(fstat->name);
		ptr = (char*)((uintptr_t)ptr - nlen);
		memcpy(ptr, fstat->name, nlen);
		if(fstat->parent != NULL) {
			ptr = (char*)((uintptr_t)ptr - 1);
			*ptr = '/';
		}
	}
	for(root = fstat->parent; root != NULL; root = (dir_stat_t*)root->parent) {
		uintptr_t nlen = strlen(root->name);
		ptr = (char*)((uintptr_t)ptr - nlen);
		memcpy(ptr, root->name, nlen);
		if(root->parent != NULL) {
			ptr = (char*)((uintptr_t)ptr - 1);
			*ptr = '/';
		}
	}
	
	return ret;
}

char* dir_stat_path(dir_stat_t* dstat) {
	if(dstat == NULL)
		return NULL;
	dir_stat_t* root;
	uintptr_t pathlen;
	for(root = dstat, pathlen = strlen(dstat->name);
		root->parent != NULL;
		root = (dir_stat_t*)root->parent, pathlen += (strlen(root->name) + 1));

	char* ret = (char*)malloc(pathlen + 1);
	if(ret == NULL)
		return NULL;
	ret[pathlen] = '\0';
	
	char* ptr = &ret[pathlen];
	for(root = dstat; root != NULL; root = (dir_stat_t*)root->parent) {
		uintptr_t nlen = strlen(root->name);
		ptr = (char*)((uintptr_t)ptr - nlen);
		memcpy(ptr, root->name, nlen);
		if(root->parent != NULL) {
			ptr = (char*)((uintptr_t)ptr - 1);
			*ptr = '/';
		}
	}
	
	return ret;
}



FILE* file_stat_fopen(file_stat_t* fstat, const char* mode) {
	char* path = file_stat_path(fstat);
	FILE* fp = fopen(path, mode);
	free(path);
	return fp;
}




dir_stat_t* dir_stat_find_dir(dir_stat_t* dstat, const char* name) {
	if(dstat == NULL)
		return NULL;
	uintptr_t i;
	for(i = 0; i < dstat->dcount; i++) {
		dir_stat_t* ret = ((dir_stat_t**)dstat->entries)[i];
		if(strcmp(ret->name, name) == 0)
			return ret;
	}
	
	return NULL;
}

file_stat_t* dir_stat_find_file(dir_stat_t* dstat, const char* name) {
	if(dstat == NULL)
		return NULL;
	uintptr_t i;
	for(i = dstat->dcount; i < (dstat->dcount + dstat->fcount); i++) {
		file_stat_t* ret = ((file_stat_t**)dstat->entries)[i];
		if(strcmp(ret->name, name) == 0)
			return ret;
	}
	
	return NULL;
}
