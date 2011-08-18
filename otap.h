#ifndef __otap_h__
#define __otap_h__

#include <stdbool.h>
#include <stdio.h>

#include "stat.h"



typedef enum {
	otap_cmd_identify      = 0x00,
	otap_cmd_update        = 0x01,
	otap_cmd_dir_create    = 0x10,
	otap_cmd_dir_enter     = 0x11,
	otap_cmd_dir_leave     = 0x12,
	otap_cmd_file_create   = 0x20,
	otap_cmd_file_delta    = 0x21,
	otap_cmd_entity_move   = 0x30,
	otap_cmd_entity_copy   = 0x31,
	otap_cmd_entity_delete = 0x32
} otap_cmd_e;

typedef struct {
	uint32_t mtime;
	union __attribute__((__packed__)) {
		struct __attribute__((__packed__)) {
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



extern bool otap_apply(FILE* stream);
extern bool otap_create(FILE* stream, otap_stat_t* a, otap_stat_t* b);

#endif
