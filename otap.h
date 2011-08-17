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

extern const char* otap_ident;;



extern bool otap_apply(FILE* stream);
extern bool otap_create(FILE* stream, dir_stat_t* a, dir_stat_t* b);

#endif
