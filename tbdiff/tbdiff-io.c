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

#include <stdbool.h>
#include <endian.h>
#include <unistd.h>
#include <assert.h>

#include "tbdiff-stat.h"

#if __STDC_VERSION__ >= 199901L
#define RESTRICT restrict
#elif defined(__GNUC__)
#define RESTRICT __restrict__
#else
#define RESTRICT
#endif

#if __BYTE_ORDER == __BIG_ENDIAN
static inline void byteswap(char *RESTRICT a, char *RESTRICT b)
{
	char swap = *a;
	*a = *b;
	*b = swap;
}

/*inverts the indices of an array of bytes. */
static inline void endianswap(void* value, size_t size)
{

	char* cvalue = value;
	int i, j;
	for (i = 0, j = (size - 1); i < (size / 2); i++, j--)
		byteswap(&cvalue[i], &cvalue[j]);
}

#define ENDIANSWAP(v) endianswap(v, sizeof(*v))
#else
#define ENDIANSWAP(v)
#endif

bool tbd_write_uint16(uint16_t value, int stream) {
	ENDIANSWAP(&value);
	return (write(stream, &value, sizeof(value)) == sizeof(value));
}

bool tbd_write_uint32(uint32_t value, int stream) {
	ENDIANSWAP(&value);
	return (write(stream, &value, sizeof(value)) == sizeof(value));
}

bool tbd_write_uint64(uint64_t value, int stream) {
	ENDIANSWAP(&value);
	return (write(stream, &value, sizeof(value)) == sizeof(value));
}

bool tbd_write_time(time_t value, int stream) {
	uint64_t realv = value;
	ENDIANSWAP(&realv);
	return (write(stream, &realv, sizeof(realv)) == sizeof(value));
}

bool tbd_write_mode(mode_t value, int stream) {
	ENDIANSWAP(&value);
	return (write(stream, &value, sizeof(value)) == sizeof(value));
}

bool tbd_write_uid(uid_t value, int stream) {
	ENDIANSWAP(&value);
	return (write(stream, &value, sizeof(value)) == sizeof(value));
}

bool tbd_write_gid(gid_t value, int stream) {
	ENDIANSWAP(&value);
	return (write(stream, &value, sizeof(value)) == sizeof(value));
}


bool tbd_read_uint16(uint16_t *value, int stream) {
	assert(value != NULL);
	size_t rval = read(stream, value, sizeof(*value));
	if (rval != sizeof(*value))
		return false;
	ENDIANSWAP(value);
	return true;
}

bool tbd_read_uint32(uint32_t *value, int stream) {
	assert(value != NULL);
	size_t rval = read(stream, value, sizeof(*value));
	if (rval != sizeof(*value))
		return false;
	ENDIANSWAP(value);
	return true;
}

bool tbd_read_uint64(uint64_t *value, int stream) {
	assert(value != NULL);
	size_t rval = read(stream, value, sizeof(*value));
	if (rval != sizeof(*value))
		return false;
	ENDIANSWAP(value);
	return true;
}

bool tbd_read_time(time_t *value, int stream) {
	assert(value != NULL);
	uint64_t realv;
	size_t rval = read(stream, &realv, sizeof(realv));
	if (rval != sizeof(*value))
		return false;
	ENDIANSWAP(&realv);
	*value = realv;
	return true;
}

bool tbd_read_mode(mode_t *value, int stream) {
	assert(value != NULL);
	ssize_t rval = read(stream, value, sizeof(*value));
	if (rval != sizeof(*value))
		return false;
	ENDIANSWAP(value);
	return true;
}

bool tbd_read_uid(uid_t *value, int stream) {
	assert(value != NULL);
	size_t rval = read(stream, value, sizeof(*value));
	if (rval != sizeof(*value))
		return false;
	ENDIANSWAP(value);
	return true;
}

bool tbd_read_gid(gid_t *value, int stream) {
	assert(value != NULL);
	size_t rval = read(stream, value, sizeof(*value));
	if (rval != sizeof(*value))
		return false;
	ENDIANSWAP(value);
	return true;
}
