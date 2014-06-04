/*
 *    Copyright (C) 2011-2014 Codethink Ltd.
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

#if !defined (TBDIFF_INSIDE_TBDIFF_H) && !defined (TBDIFF_COMPILATION)
#error "Only <tbdiff/tbdiff.h> may be included directly. This file might disappear or change contents."
#endif

#ifndef __TBDIFF_IO_H__
#define __TBDIFF_IO_H__

#include <stdbool.h>
#include <endian.h>
#include <unistd.h>
#include <assert.h>

#include <tbdiff/tbdiff-stat.h>

bool tbd_write_uint16(uint16_t value, int stream);
bool tbd_write_uint32(uint32_t value, int stream);
bool tbd_write_uint64(uint64_t value, int stream);
bool tbd_write_time  (time_t   value, int stream);
bool tbd_write_mode  (mode_t   value, int stream);
bool tbd_write_uid   (uid_t    value, int stream);
bool tbd_write_gid   (gid_t    value, int stream);

bool tbd_read_uint16(uint16_t *value, int stream);
bool tbd_read_uint32(uint32_t *value, int stream);
bool tbd_read_uint64(uint64_t *value, int stream);
bool tbd_read_time  (time_t   *value, int stream);
bool tbd_read_mode  (mode_t   *value, int stream);
bool tbd_read_uid   (uid_t    *value, int stream);
bool tbd_read_gid   (gid_t    *value, int stream);

#endif /* !__TBDIFF_IO_H__ */
