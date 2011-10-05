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
	TBD_CMD_IDENTIFY             = 0x00,
	TBD_CMD_UPDATE               = 0x01,
	TBD_CMD_DIR_CREATE           = 0x10,
	TBD_CMD_DIR_ENTER            = 0x11,
	TBD_CMD_DIR_LEAVE            = 0x12,
	TBD_CMD_DIR_DELTA            = 0x13,
	TBD_CMD_FILE_CREATE          = 0x20,
	TBD_CMD_FILE_DELTA           = 0x21,
	TBD_CMD_FILE_METADATA_UPDATE = 0x22,
	TBD_CMD_ENTITY_MOVE          = 0x30,
	TBD_CMD_ENTITY_COPY          = 0x31,
	TBD_CMD_ENTITY_DELETE        = 0x32,
	TBD_CMD_SYMLINK_CREATE       = 0x40,
	TBD_CMD_SPECIAL_CREATE       = 0x50,
} tbd_cmd_e;

typedef enum {
	TBD_METADATA_NONE  = 0x0,
	TBD_METADATA_MTIME = 0x1,
	TBD_METADATA_MODE  = 0x2,
	TBD_METADATA_UID   = 0x4,
	TBD_METADATA_GID   = 0x8,
	TBD_METADATA_RDEV  = 0x10,
} tbd_metadata_type_e;

typedef enum {
	TBD_ERROR_SUCCESS =  0,
	TBD_ERROR_FAILURE = -1,
	TBD_ERROR_OUT_OF_MEMORY = -2,
	TBD_ERROR_NULL_POINTER = -3,
	TBD_ERROR_INVALID_PARAMETER = -4,
	TBD_ERROR_UNABLE_TO_READ_STREAM  = -5,
	TBD_ERROR_UNABLE_TO_WRITE_STREAM = -6,
	TBD_ERROR_UNABLE_TO_CREATE_DIR = -7,
	TBD_ERROR_UNABLE_TO_CHANGE_DIR = -8,
	TBD_ERROR_UNABLE_TO_OPEN_FILE_FOR_READING = -9,
	TBD_ERROR_UNABLE_TO_OPEN_FILE_FOR_WRITING = -10,
	TBD_ERROR_FILE_ALREADY_EXISTS = -11,
	TBD_ERROR_UNABLE_TO_REMOVE_FILE = -12,
	TBD_ERROR_UNABLE_TO_SEEK_THROUGH_STREAM = -13,
	TBD_ERROR_FEATURE_NOT_IMPLEMENTED = -14,
	TBD_ERROR_FILE_DOES_NOT_EXIST = -15,
	TBD_ERROR_UNABLE_TO_DETECT_STREAM_POSITION = -16,
	TBD_ERROR_UNABLE_TO_STAT_FILE = -17,
	TBD_ERROR_UNABLE_TO_READ_SYMLINK = -18,
	TBD_ERROR_UNABLE_TO_CREATE_SYMLINK = -19,
	TBD_ERROR_UNABLE_TO_READ_SPECIAL_FILE = -20,
	TBD_ERROR_UNABLE_TO_CREATE_SPECIAL_FILE = - 21,
} tbd_error_e;

#ifdef NDEBUG
#define TBD_ERROR(e) (e)
#else
#define TBD_ERROR(e) tbd_error(e, #e, __func__, __LINE__, __FILE__)
inline tbd_error_e tbd_error(tbd_error_e e, char const *s, char const *func,
                                                     int line, char const* file)
{
	if (e != TBD_ERROR_SUCCESS)
		fprintf(stderr, "TBDiff error '%s' in function '%s' at line %d "
		                        "of file '%s'.\n", s, func, line, file);
	return e;
}
#endif

extern int         tbd_apply (FILE *stream);
extern int         tbd_create(FILE *stream, tbd_stat_t *a, tbd_stat_t *b);

#endif /* __TBDIFF_H__ */
