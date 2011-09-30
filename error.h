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

#ifndef __otap_error_h__
#define __otap_error_h__

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
} otap_error_e;

#ifdef NDEBUG
#define otap_error(e) return e
#else
#define otap_error(e) { if(e != 0) fprintf(stderr, "OTAP error '%s' in function '%s' at line %d of file '%s'.\n", #e, __FUNCTION__, __LINE__, __FILE__); return e; }
#endif

#endif
