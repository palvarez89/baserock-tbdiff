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

#if __BYTE_ORDER == __BIG_ENDIAN
/*inverts the indices of an array of bytes. */
static void byteswap(char* value, int size) {
	char tmp;
	int i;
	for (i = 0; i < size/2; i++) {
		tmp = value[i];
		value[i] = value[size-i-1];
		value[size-i-1] = tmp;
	}
}
#endif

size_t tbd_write_uint16_t(uint16_t value, FILE* stream) {
#if __BYTE_ORDER == __BIG_ENDIAN
	byteswap((char*)&value, sizeof(value));
#endif
	return fwrite(&value, sizeof(value), 1, stream);
}

size_t tbd_write_uint32_t(uint32_t value, FILE* stream) {
#if __BYTE_ORDER == __BIG_ENDIAN
	byteswap((char*)&value, sizeof(value));
#endif
	return fwrite(&value, sizeof(value), 1, stream);
}

size_t tbd_write_uint64_t(uint64_t value, FILE* stream) {
#if __BYTE_ORDER == __BIG_ENDIAN
	byteswap((char*)&value, sizeof(value));
#endif
	return fwrite(&value, sizeof(value), 1, stream);
}

size_t tbd_write_time_t(time_t value, FILE* stream) {
	uint64_t realv = value;
#if __BYTE_ORDER == __BIG_ENDIAN
	byteswap((char*)&realv, sizeof(realv));
#endif
	return fwrite(&realv, sizeof(realv), 1, stream);
}

size_t tbd_write_mode_t(mode_t value, FILE* stream) {
#if __BYTE_ORDER == __BIG_ENDIAN
	byteswap((char*)&value, sizeof(value));
#endif
	return fwrite(&value, sizeof(value), 1, stream);
}

size_t tbd_write_uid_t(uid_t value, FILE* stream) {
#if __BYTE_ORDER == __BIG_ENDIAN
	byteswap((char*)&value, sizeof(value));
#endif
	return fwrite(&value, sizeof(value), 1, stream);
}

size_t tbd_write_gid_t(gid_t value, FILE* stream) {
#if __BYTE_ORDER == __BIG_ENDIAN
	byteswap((char*)&value, sizeof(value));
#endif
	return fwrite(&value, sizeof(value), 1, stream);
}

size_t tbd_write_size_t(size_t value, FILE* stream) {
#if __BYTE_ORDER == __BIG_ENDIAN
	byteswap((char*)&value, sizeof(value));
#endif
	return fwrite(&value, sizeof(value), 1, stream);
}

size_t tbd_read_uint16_t(uint16_t *value, FILE* stream) {
	assert(value != NULL);
	size_t rval = fread(value, sizeof(*value), 1, stream);
#if __BYTE_ORDER == __BIG_ENDIAN
	byteswap((char*)value, sizeof(*value));
#endif
	return rval;
}

size_t tbd_read_uint32_t(uint32_t *value, FILE *stream) {
	assert(value != NULL);
	size_t rval = fread(value, sizeof(*value), 1, stream);
#if __BYTE_ORDER == __BIG_ENDIAN
	byteswap((char*)value, sizeof(*value));
#endif
	return rval;
}

size_t tbd_read_uint64_t(uint64_t *value, FILE *stream) {
	assert(value != NULL);
	size_t rval = fread(value, sizeof(*value), 1, stream);
#if __BYTE_ORDER == __BIG_ENDIAN
	byteswap((char*)value, sizeof(*value));
#endif
	return rval;
}

size_t tbd_read_time_t(time_t *value, FILE *stream) {
	assert(value != NULL);
	uint64_t realv;
	size_t rval = fread(&realv, sizeof(realv), 1, stream);
#if __BYTE_ORDER == __BIG_ENDIAN
	byteswap((char*)&realv, sizeof(realv));
#endif
	*value = realv;
	return rval;
}

size_t tbd_read_mode_t(mode_t *value, FILE *stream) {
	assert(value != NULL);
	size_t rval = fread(value, sizeof(*value), 1, stream);
#if __BYTE_ORDER == __BIG_ENDIAN
	byteswap((char*)value, sizeof(*value));
#endif
	return rval;
}

size_t tbd_read_uid_t(uid_t *value, FILE *stream) {
	assert(value != NULL);
	size_t rval = fread(value, sizeof(*value), 1, stream);
#if __BYTE_ORDER == __BIG_ENDIAN
	byteswap((char*)value, sizeof(*value));
#endif
	return rval;
}

size_t tbd_read_gid_t(gid_t *value, FILE *stream) {
	assert(value != NULL);
	size_t rval = fread(value, sizeof(*value), 1, stream);
#if __BYTE_ORDER == __BIG_ENDIAN
	byteswap((char*)value, sizeof(*value));
#endif
	return rval;
}

size_t tbd_read_size_t(size_t *value, FILE *stream) {
	assert(value != NULL);
	size_t rval = fread(value, sizeof(*value), 1, stream);
#if __BYTE_ORDER == __BIG_ENDIAN
	byteswap((char*)value, sizeof(*value));
#endif
	return rval;
}
