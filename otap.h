#ifndef __otap_h__
#define __otap_h__

#include <stdio.h>
#include <stdint.h>

#include "stat.h"

typedef enum
{
    OTAP_CMD_IDENTIFY       = 0x00,
    OTAP_CMD_UPDATE         = 0x01,
    OTAP_CMD_DIR_CREATE     = 0x10,
    OTAP_CMD_DIR_ENTER      = 0x11,
    OTAP_CMD_DIR_LEAVE      = 0x12,
    OTAP_CMD_FILE_CREATE    = 0x20,
    OTAP_CMD_FILE_DELTA     = 0x21,
    OTAP_CMD_ENTITY_MOVE    = 0x30,
    OTAP_CMD_ENTITY_COPY    = 0x31,
    OTAP_CMD_ENTITY_DELETE  = 0x32,
    OTAP_CMD_SYMLINK_CREATE = 0x40,
    OTAP_CMD_SPECIAL_CREATE = 0x50
} otap_cmd_e;

typedef enum
{
    OTAP_METADATA_MTIME,
    OTAP_METADATA_PERMISSION,
    OTAP_METADATA_UID,
    OTAP_METADATA_GID,
} otap_metadata_type_e;

typedef struct
{
    uint32_t metadata_mask;
    uint32_t mtime;
    uint32_t uid;
    uint32_t gid;
    union __attribute__((__packed__))
    {
        struct __attribute__((__packed__))
        {
            unsigned or:1, ow:1, ox:1;
            unsigned gr:1, gw:1, gx:1;
            unsigned ur:1, uw:1, ux:1;
            unsigned reserved:7;
        };
        uint16_t mask;
    } perms;
    uint16_t reserved;
} __attribute__((__packed__)) otap_metadata_t;

extern const char* otap_ident;

extern int otap_apply(FILE* stream);
extern int otap_create(FILE* stream, otap_stat_t* a, otap_stat_t* b);

#endif
