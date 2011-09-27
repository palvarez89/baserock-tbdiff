#ifndef __otap_stat_h__
#define __otap_stat_h__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

typedef enum
{
    OTAP_STAT_TYPE_FILE    = 'f',
    OTAP_STAT_TYPE_DIR     = 'd',
    OTAP_STAT_TYPE_SYMLINK = 'l',
    OTAP_STAT_TYPE_CHRDEV  = 'c',
    OTAP_STAT_TYPE_BLKDEV  = 'b',
    OTAP_STAT_TYPE_FIFO    = 'p',
    OTAP_STAT_TYPE_SOCKET  = 's'
} otap_stat_type_e;

typedef struct
{
    void*            parent;
    char*            name;
    otap_stat_type_e type;
    uint32_t         mtime;
    uint32_t         size; // Count for directory.
    uid_t            user;
    uid_t            group;
    mode_t           mode;
    dev_t            dev;
} otap_stat_t;


extern otap_stat_t* otap_stat(const char* path);
extern void         otap_stat_free(otap_stat_t* file);
extern void         otap_stat_print(otap_stat_t* file);
extern otap_stat_t* otap_stat_entry(otap_stat_t* file, uint32_t entry);
extern otap_stat_t* otap_stat_entry_find(otap_stat_t* file, const char* name);
extern char*        otap_stat_subpath(otap_stat_t* file, const char* entry);
extern char*        otap_stat_path(otap_stat_t* file);
extern int          otap_stat_open(otap_stat_t* file, int flags);
extern FILE*        otap_stat_fopen(otap_stat_t* file, const char* mode);

#endif
