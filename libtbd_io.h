#ifndef __LIBTBD_IO_H__
#define __LIBTBD_IO_H__


#include <endian.h>
#include <unistd.h>
#include <assert.h>
#include "libtbd_stat.h"

size_t tbd_write_uint16_t (uint16_t value, FILE* stream);

size_t tbd_write_uint32_t (uint32_t value, FILE* stream);

size_t tbd_write_uint64_t (uint64_t value, FILE* stream);

size_t tbd_write_time_t (time_t value, FILE* stream);

size_t tbd_write_mode_t (mode_t value, FILE* stream);

size_t tbd_write_uid_t (uid_t value, FILE* stream);

size_t tbd_write_gid_t (gid_t value, FILE* stream);

size_t tbd_write_size_t (size_t value, FILE* stream);

size_t tbd_read_uint16_t (uint16_t *value, FILE* stream);

size_t tbd_read_uint32_t (uint32_t *value, FILE* stream);

size_t tbd_read_uint64_t (uint64_t *value, FILE* stream);

size_t tbd_read_time_t (time_t *value, FILE* stream);

size_t tbd_read_mode_t (mode_t *value, FILE* stream);

size_t tbd_read_uid_t (uid_t *value, FILE* stream);

size_t tbd_read_gid_t (gid_t *value, FILE* stream);

size_t tbd_read_size_t (size_t *value, FILE* stream);

#endif
