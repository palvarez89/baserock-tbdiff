#include "stat.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>



static otap_stat_t*
__otap_stat_fd(const char *name,
               const char *path)
{
    struct stat info;

    if(lstat(path, &info) != 0)
        return NULL;

    size_t nlen = strlen(name);
    otap_stat_t* ret = (otap_stat_t*)malloc(sizeof(otap_stat_t) + (nlen + 1));
    if(ret == NULL)
        return NULL;

    ret->parent = NULL;
    ret->name   = (char*)((uintptr_t)ret + sizeof(otap_stat_t));
    memcpy(ret->name, name, (nlen + 1));

    if(S_ISREG(info.st_mode))
    {
        ret->type = OTAP_STAT_TYPE_FILE;
        ret->size = info.st_size;
    }
    else if(S_ISDIR(info.st_mode))
    {
        int fd = open(path, O_RDONLY);
        if (fd < 0)
        {
            free(ret);
            return NULL;
        }

        ret->type = OTAP_STAT_TYPE_DIR;
        DIR* dp = fdopendir(fd);
        if(dp == NULL)
        {
            free(ret);
            return NULL;
        }

        ret->size = 0;
        struct dirent* ds;
        for(ds = readdir(dp); ds != NULL; ds = readdir(dp))
        {
            if((strcmp(ds->d_name, ".") == 0)
                    || (strcmp(ds->d_name, "..") == 0))
                continue;

            ret->size++;
        }
    }
    else if(S_ISLNK(info.st_mode))
    {
        ret->type = OTAP_STAT_TYPE_SYMLINK;
        ret->size = 0;
    }
    else if(S_ISCHR(info.st_mode))
    {
        ret->type = OTAP_STAT_TYPE_CHRDEV;
        ret->size = 0;
    }
    else if(S_ISBLK(info.st_mode))
    {
        ret->type = OTAP_STAT_TYPE_BLKDEV;
        ret->size = 0;
    }
    else if(S_ISFIFO(info.st_mode))
    {
        ret->type = OTAP_STAT_TYPE_FIFO;
        ret->size = 0;
    }
    else if(S_ISSOCK(info.st_mode))
    {
        ret->type = OTAP_STAT_TYPE_SOCKET;
        ret->size = 0;
    }
    else
    {
        free(ret);
        return NULL;
    }

    ret->rdev  = (uint32_t)info.st_rdev;
    ret->uid   = (uint32_t)info.st_uid;
    ret->gid   = (uint32_t)info.st_gid;
    ret->mode  = (uint32_t)info.st_mode;
    ret->mtime = (uint32_t)info.st_mtime;

    return ret;
}

otap_stat_t*
otap_stat(const char* path)
{
    otap_stat_t* ret = __otap_stat_fd(path, path);
    return ret;
}

void
otap_stat_free(otap_stat_t* file)
{
    free(file);
}

void
otap_stat_print(otap_stat_t* file)
{
    (void)file;
}

otap_stat_t*
otap_stat_entry(otap_stat_t* file, uint32_t entry)
{
    if((file == NULL)
            || (file->type != OTAP_STAT_TYPE_DIR)
            || (entry >= file->size))
        return NULL;

    int fd = otap_stat_open(file, O_RDONLY);
    if(fd < 0)
        return NULL;

    DIR* dp = fdopendir(fd);
    if(dp == NULL)
    {
        close(fd);
        return NULL;
    }

    uintptr_t i;
    struct dirent* ds;
    for(i = 0; i <= entry; i++)
    {
        ds = readdir(dp);
        if(ds == NULL)
        {
            close(fd);
            return NULL;
        }
        if((strcmp(ds->d_name, ".") == 0)
                || (strcmp(ds->d_name, "..") == 0))
            i--;
    }
    close(fd);
    fd = -1;

    char* spath = otap_stat_subpath(file, ds->d_name);
    if(spath == NULL)
        return NULL;

    otap_stat_t* ret = __otap_stat_fd(ds->d_name, (const char*)spath);

    free(spath);

    if (ret == NULL)
        return NULL;

    ret->parent = file;
    return ret;
}

otap_stat_t*
otap_stat_entry_find(otap_stat_t* file, const char* name)
{
    if((file == NULL)
            || (file->type != OTAP_STAT_TYPE_DIR))
        return NULL;

    int fd = otap_stat_open(file, O_RDONLY);
    if(fd < 0)
        return NULL;

    DIR* dp = fdopendir(fd);
    close(fd);
    if(dp == NULL)
        return NULL;

    struct dirent* ds;
    for(ds = readdir(dp); ds != NULL; ds = readdir(dp))
    {
        if(strcmp(ds->d_name, name) == 0)
        {
            char* spath = otap_stat_subpath(file, ds->d_name);
            if(spath == NULL)
                return NULL;

            otap_stat_t* ret = __otap_stat_fd(ds->d_name, (const char *)spath);

            free(spath);
            ret->parent = file;
            return ret;
        }
    }

    return NULL;
}

char*
otap_stat_subpath(otap_stat_t *file,
                  const char  *entry)
{
    if(file == NULL)
        return NULL;

    size_t elen = ((entry == NULL) ? 0 : (strlen(entry) + 1));

    size_t plen;
    otap_stat_t* root;
    for(root = file, plen = 0;
            root != NULL;
            plen += (strlen(root->name) + 1),
            root = (otap_stat_t*)root->parent);
    plen += elen;


    char* path = (char*)malloc(plen);
    if(path == NULL)
        return NULL;
    char* ptr = &path[plen];

    if(entry != NULL)
    {
        ptr = (char*)((uintptr_t)ptr - elen);
        memcpy(ptr, entry, elen);
    }

    for(root = file; root != NULL; root = (otap_stat_t*)root->parent)
    {
        size_t rlen = strlen(root->name) + 1;
        ptr = (char*)((uintptr_t)ptr - rlen);
        memcpy(ptr, root->name, rlen);
        if((file != root) || (entry != NULL))
            ptr[rlen - 1] = '/';
    }

    return path;
}

char*
otap_stat_path(otap_stat_t* file)
{
    return otap_stat_subpath(file, NULL);
}

int
otap_stat_open(otap_stat_t* file, int flags)
{
    char* path = otap_stat_path(file);
    if(path == NULL)
        return -1;
    int fd = open(path, flags);
    free(path);
    return fd;
}

FILE*
otap_stat_fopen(otap_stat_t *file,
                const char  *mode)
{
    char* path = otap_stat_path(file);
    if(path == NULL)
        return NULL;
    FILE* fp = fopen(path, mode);
    free(path);
    return fp;
}
