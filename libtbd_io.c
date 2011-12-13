#include <endian.h>
#include <unistd.h>
#include <assert.h>

#include "libtbd_stat.h"

#if __BYTE_ORDER == __BIG_ENDIAN
//inverts the indices of an array of bytes.
static void byteswap (char* value, int size) {
    char tmp;
    int i;
    for (i = 0; i < size/2; i++) {
        tmp = value[i];
        value[i] = value[size-i-1];
        value[size-i-1] = tmp;
    }   
}
#endif

size_t tbd_write_uint16_t (uint16_t value, FILE* stream) {
#if __BYTE_ORDER == __BIG_ENDIAN
        byteswap((char*)&value, sizeof(value));
#endif
    return fwrite(&value, sizeof(value), 1, stream);
}

size_t tbd_write_uint32_t (uint32_t value, FILE* stream) {
#if __BYTE_ORDER == __BIG_ENDIAN
        byteswap((char*)&value, sizeof(value));
#endif
    return fwrite(&value, sizeof(value), 1, stream);
}

size_t tbd_write_uint64_t (uint64_t value, FILE* stream) {
#if __BYTE_ORDER == __BIG_ENDIAN
        byteswap((char*)&value, sizeof(value));
#endif
    return fwrite(&value, sizeof(value), 1, stream);
}

size_t tbd_write_time_t (time_t value, FILE* stream) {
    uint64_t realv = value;
#if __BYTE_ORDER == __BIG_ENDIAN
        byteswap((char*)&realv, sizeof(realv));
#endif
    return fwrite(&realv, sizeof(realv), 1, stream);
}

size_t tbd_write_mode_t (mode_t value, FILE* stream) {
#if __BYTE_ORDER == __BIG_ENDIAN
        byteswap((char*)&value, sizeof(value));
#endif
    return fwrite(&value, sizeof(value), 1, stream);
}

size_t tbd_write_uid_t (uid_t value, FILE* stream) {
#if __BYTE_ORDER == __BIG_ENDIAN
        byteswap((char*)&value, sizeof(value));
#endif
    return fwrite(&value, sizeof(value), 1, stream);
}

size_t tbd_write_gid_t (gid_t value, FILE* stream) {
#if __BYTE_ORDER == __BIG_ENDIAN
        byteswap((char*)&value, sizeof(value));
#endif
    return fwrite(&value, sizeof(value), 1, stream);
}

size_t tbd_write_size_t (size_t value, FILE* stream) {
#if __BYTE_ORDER == __BIG_ENDIAN
        byteswap((char*)&value, sizeof(value));
#endif
    return fwrite(&value, sizeof(value), 1, stream);
}

size_t tbd_read_uint16_t (uint16_t *value, FILE* stream) {
    assert(value != NULL);
    size_t rval = fread(value, sizeof(*value), 1, stream);
#if __BYTE_ORDER == __BIG_ENDIAN
        byteswap((char*)value, sizeof(*value));
#endif
    return rval;
}

size_t tbd_read_uint32_t (uint32_t *value, FILE* stream) {
    assert(value != NULL);
    size_t rval = fread(value, sizeof(*value), 1, stream);
#if __BYTE_ORDER == __BIG_ENDIAN
        byteswap((char*)value, sizeof(*value));
#endif
    return rval;
}

size_t tbd_read_uint64_t (uint64_t *value, FILE* stream) {
    assert(value != NULL);
    size_t rval = fread(value, sizeof(*value), 1, stream);
#if __BYTE_ORDER == __BIG_ENDIAN
        byteswap((char*)value, sizeof(*value));
#endif
    return rval;
}

size_t tbd_read_time_t (time_t *value, FILE* stream) {
    assert(value != NULL);
    uint64_t realv;
    size_t rval = fread(&realv, sizeof(realv), 1, stream);
#if __BYTE_ORDER == __BIG_ENDIAN
        byteswap((char*)&realv, sizeof(realv));        
#endif
    *value = realv;
    return rval;
    }

size_t tbd_read_mode_t (mode_t *value, FILE* stream) {
    assert(value != NULL);
    size_t rval = fread(value, sizeof(*value), 1, stream);
#if __BYTE_ORDER == __BIG_ENDIAN
        byteswap((char*)value, sizeof(*value));
#endif
    return rval;
    }

size_t tbd_read_uid_t (uid_t *value, FILE* stream) {
    assert(value != NULL);
    size_t rval = fread(value, sizeof(*value), 1, stream);
#if __BYTE_ORDER == __BIG_ENDIAN
        byteswap((char*)value, sizeof(*value));
#endif
    return rval;
    }

size_t tbd_read_gid_t (gid_t *value, FILE* stream) {
    assert(value != NULL);
    size_t rval = fread(value, sizeof(*value), 1, stream);
#if __BYTE_ORDER == __BIG_ENDIAN
        byteswap((char*)value, sizeof(*value));
#endif
    return rval;
    }

size_t tbd_read_size_t (size_t *value, FILE* stream) {
    assert(value != NULL);
    size_t rval = fread(value, sizeof(*value), 1, stream);
#if __BYTE_ORDER == __BIG_ENDIAN
        byteswap((char*)value, sizeof(*value));
#endif
    return rval;
    }
