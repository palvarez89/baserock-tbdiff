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
	uint16_t len;
	char*    name;
} otap_name_t;

typedef struct {
	uint32_t mtime;
} otap_metadata_t;

typedef struct {
	uint32_t size;
	void*    data;
} otap_data_t;

typedef struct {
	uint32_t start, end;
	otap_data_t delta;
} otap_delta_t;

static const otap_name_t otap_ident = { 16, "CodeThink:OTAPv0" };



extern bool otap_apply(FILE* stream);
extern bool otap_create(FILE* stream, dir_stat_t* a, dir_stat_t* b);

#endif
