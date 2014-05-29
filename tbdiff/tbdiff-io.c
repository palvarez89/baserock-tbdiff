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

#include <endian.h>
#include <unistd.h>
#include <assert.h>

#include <tbdiff/tbdiff-stat.h>

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

size_t tbd_write_uint16_t(uint16_t value, FILE *stream) {
	ENDIANSWAP(&value);
	return fwrite(&value, sizeof(value), 1, stream);
}

size_t tbd_write_uint32_t(uint32_t value, FILE *stream) {
	ENDIANSWAP(&value);
	return fwrite(&value, sizeof(value), 1, stream);
}

size_t tbd_write_uint64_t(uint64_t value, FILE *stream) {
	ENDIANSWAP(&value);
	return fwrite(&value, sizeof(value), 1, stream);
}

size_t tbd_write_time_t(time_t value, FILE *stream) {
	uint64_t realv = value;
	ENDIANSWAP(&realv);
	return fwrite(&realv, sizeof(realv), 1, stream);
}

size_t tbd_write_mode_t(mode_t value, FILE *stream) {
	ENDIANSWAP(&value);
	return fwrite(&value, sizeof(value), 1, stream);
}

size_t tbd_write_uid_t(uid_t value, FILE *stream) {
	ENDIANSWAP(&value);
	return fwrite(&value, sizeof(value), 1, stream);
}

size_t tbd_write_gid_t(gid_t value, FILE *stream) {
	ENDIANSWAP(&value);
	return fwrite(&value, sizeof(value), 1, stream);
}

size_t tbd_write_size_t(size_t value, FILE *stream) {
	ENDIANSWAP(&value);
	return fwrite(&value, sizeof(value), 1, stream);
}

size_t tbd_read_uint16_t(uint16_t *value, FILE *stream) {
	assert(value != NULL);
	size_t rval = fread(value, sizeof(*value), 1, stream);
	ENDIANSWAP(value);
	return rval;
}

size_t tbd_read_uint32_t(uint32_t *value, FILE *stream) {
	assert(value != NULL);
	size_t rval = fread(value, sizeof(*value), 1, stream);
	ENDIANSWAP(value);
	return rval;
}

size_t tbd_read_uint64_t(uint64_t *value, FILE *stream) {
	assert(value != NULL);
	size_t rval = fread(value, sizeof(*value), 1, stream);
	ENDIANSWAP(value);
	return rval;
}

size_t tbd_read_time_t(time_t *value, FILE *stream) {
	assert(value != NULL);
	uint64_t realv;
	size_t rval = fread(&realv, sizeof(realv), 1, stream);
	ENDIANSWAP(&realv);
	*value = realv;
	return rval;
}

size_t tbd_read_mode_t(mode_t *value, FILE *stream) {
	assert(value != NULL);
	size_t rval = fread(value, sizeof(*value), 1, stream);
	ENDIANSWAP(value);
	return rval;
}

size_t tbd_read_uid_t(uid_t *value, FILE *stream) {
	assert(value != NULL);
	size_t rval = fread(value, sizeof(*value), 1, stream);
	ENDIANSWAP(value);
	return rval;
}

size_t tbd_read_gid_t(gid_t *value, FILE *stream) {
	assert(value != NULL);
	size_t rval = fread(value, sizeof(*value), 1, stream);
	ENDIANSWAP(value);
	return rval;
}

size_t tbd_read_size_t(size_t *value, FILE *stream) {
	assert(value != NULL);
	size_t rval = fread(value, sizeof(*value), 1, stream);
	ENDIANSWAP(value);
	return rval;
}
