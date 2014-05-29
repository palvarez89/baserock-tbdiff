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

#include <endian.h>
#include <unistd.h>
#include <assert.h>

#include <tbdiff/tbdiff-stat.h>

extern size_t tbd_write_uint16_t(uint16_t value, FILE* stream);
extern size_t tbd_write_uint32_t(uint32_t value, FILE* stream);
extern size_t tbd_write_uint64_t(uint64_t value, FILE* stream);
extern size_t tbd_write_time_t(time_t value, FILE* stream);
extern size_t tbd_write_mode_t(mode_t value, FILE* stream);
extern size_t tbd_write_uid_t(uid_t value, FILE* stream);
extern size_t tbd_write_gid_t(gid_t value, FILE* stream);
extern size_t tbd_write_size_t(size_t value, FILE* stream);

extern size_t tbd_read_uint16_t(uint16_t *value, FILE* stream);
extern size_t tbd_read_uint32_t(uint32_t *value, FILE* stream);
extern size_t tbd_read_uint64_t(uint64_t *value, FILE* stream);
extern size_t tbd_read_time_t(time_t *value, FILE* stream);
extern size_t tbd_read_mode_t(mode_t *value, FILE* stream);
extern size_t tbd_read_uid_t(uid_t *value, FILE* stream);
extern size_t tbd_read_gid_t(gid_t *value, FILE* stream);
extern size_t tbd_read_size_t(size_t *value, FILE* stream);

#endif /* !__TBDIFF_IO_H__ */
