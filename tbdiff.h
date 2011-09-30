/*
 *    Copyright (C) 2011 Codethink Ltd.
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License Version 2 as
 *    published by the Free Software Foundation.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License along
 *    with this program; if not, write to the Free Software Foundation, Inc.,
 *    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __TBDIFF_H__
#define __TBDIFF_H__

#include <stdio.h>
#include <stdint.h>

#include "libtbd_stat.h"

typedef enum {
	OTAP_CMD_IDENTIFY             = 0x00,
	OTAP_CMD_UPDATE               = 0x01,
	OTAP_CMD_DIR_CREATE           = 0x10,
	OTAP_CMD_DIR_ENTER            = 0x11,
	OTAP_CMD_DIR_LEAVE            = 0x12,
	OTAP_CMD_DIR_DELTA            = 0x13,
	OTAP_CMD_FILE_CREATE          = 0x20,
	OTAP_CMD_FILE_DELTA           = 0x21,
	OTAP_CMD_FILE_METADATA_UPDATE = 0x22,
	OTAP_CMD_ENTITY_MOVE          = 0x30,
	OTAP_CMD_ENTITY_COPY          = 0x31,
	OTAP_CMD_ENTITY_DELETE        = 0x32,
	OTAP_CMD_SYMLINK_CREATE       = 0x40,
	OTAP_CMD_SPECIAL_CREATE       = 0x50,
} tbd_cmd_e;

typedef enum {
	OTAP_METADATA_NONE  = 0x0,
	OTAP_METADATA_MTIME = 0x1,
	OTAP_METADATA_MODE  = 0x2,
	OTAP_METADATA_UID   = 0x4,
	OTAP_METADATA_GID   = 0x8,
	OTAP_METADATA_RDEV   = 0x10,
} tbd_metadata_type_e;

typedef enum {
	OTAP_ERROR_SUCCESS =  0,
	OTAP_ERROR_FAILURE = -1,
	OTAP_ERROR_OUT_OF_MEMORY = -2,
	OTAP_ERROR_NULL_POINTER = -3,
	OTAP_ERROR_INVALID_PARAMETER = -4,
	OTAP_ERROR_UNABLE_TO_READ_STREAM  = -5,
	OTAP_ERROR_UNABLE_TO_WRITE_STREAM = -6,
	OTAP_ERROR_UNABLE_TO_CREATE_DIR = -7,
	OTAP_ERROR_UNABLE_TO_CHANGE_DIR = -8,
	OTAP_ERROR_UNABLE_TO_OPEN_FILE_FOR_READING = -9,
	OTAP_ERROR_UNABLE_TO_OPEN_FILE_FOR_WRITING = -10,
	OTAP_ERROR_FILE_ALREADY_EXISTS = -11,
	OTAP_ERROR_UNABLE_TO_REMOVE_FILE = -12,
	OTAP_ERROR_UNABLE_TO_SEEK_THROUGH_STREAM = -13,
	OTAP_ERROR_FEATURE_NOT_IMPLEMENTED = -14,
	OTAP_ERROR_FILE_DOES_NOT_EXIST = -15,
	OTAP_ERROR_UNABLE_TO_DETECT_STREAM_POSITION = -16,
	OTAP_ERROR_UNABLE_TO_STAT_FILE = -17,
	OTAP_ERROR_UNABLE_TO_READ_SYMLINK = -18,
	OTAP_ERROR_UNABLE_TO_CREATE_SYMLINK = -19,
	OTAP_ERROR_UNABLE_TO_READ_SPECIAL_FILE = -20,
	OTAP_ERROR_UNABLE_TO_CREATE_SPECIAL_FILE = - 21,
} tbd_error_e;

#ifdef NDEBUG
#define otap_error(e) return e
#else
#define otap_error(e) { if(e != 0) fprintf(stderr, "OTAP error '%s' in function '%s' at line %d of file '%s'.\n", #e, __FUNCTION__, __LINE__, __FILE__); return e; }
#endif

extern const char* otap_ident;
extern int         otap_apply(FILE* stream);
extern int         otap_create(FILE* stream, otap_stat_t* a, otap_stat_t* b);

#endif /* __TBDIFF_H__ */
