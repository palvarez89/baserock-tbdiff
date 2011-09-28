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
    OTAP_CMD_DIR_DELTA      = 0x13,
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
    OTAP_METADATA_NONE  = 0x0,
    OTAP_METADATA_MTIME = 0x1,
    OTAP_METADATA_MODE  = 0x2,
    OTAP_METADATA_UID   = 0x4,
    OTAP_METADATA_GID   = 0x8,
} otap_metadata_type_e;

extern const char* otap_ident;

extern int otap_apply(FILE* stream);
extern int otap_create(FILE* stream, otap_stat_t* a, otap_stat_t* b);

#endif
