#include "otap.h"
#include "error.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#define PATH_BUFFER_LENGTH 4096

static int
_otap_create_fwrite_cmd (FILE    *stream,
                         uint8_t  cmd)
{
    if(fwrite(&cmd, 1, 1, stream) != 1)
        otap_error(OTAP_ERROR_UNABLE_TO_WRITE_STREAM);
    return 0;
}

static int
_otap_create_fwrite_string(FILE       *stream,
                           const char *string)
{
    uint16_t slen = strlen(string);
    if((fwrite(&slen, 2, 1, stream) != 1)
            || (fwrite(string, 1, slen, stream) != slen))
        otap_error(OTAP_ERROR_UNABLE_TO_WRITE_STREAM);
    return 0;
}

static int
_otap_create_fwrite_mtime (FILE    *stream,
                           uint32_t  mtime)
{
    if(fwrite(&mtime, sizeof(uint32_t), 1, stream) != 1)
        otap_error(OTAP_ERROR_UNABLE_TO_WRITE_STREAM);
    return 0;
}

static int
_otap_create_fwrite_mode (FILE     *stream,
                          uint32_t  mode)
{
    if(fwrite(&mode, sizeof(uint32_t), 1, stream) != 1)
        otap_error(OTAP_ERROR_UNABLE_TO_WRITE_STREAM);
    return 0;
}

static int
_otap_create_fwrite_gid (FILE     *stream,
                         gid_t     gid)
{
    if(fwrite(&gid, sizeof(gid_t), 1, stream) != 1)
        otap_error(OTAP_ERROR_UNABLE_TO_WRITE_STREAM);
    return 0;
}

static int
_otap_create_fwrite_uid (FILE     *stream,
                         uid_t     uid)
{
    if(fwrite(&uid, sizeof(uid_t), 1, stream) != 1)
        otap_error(OTAP_ERROR_UNABLE_TO_WRITE_STREAM);
    return 0;
}

static int
_otap_create_fwrite_dev (FILE     *stream,
                         uint32_t  dev)
{
    if(fwrite(&dev, sizeof(uint32_t), 1, stream) != 1)
        otap_error(OTAP_ERROR_UNABLE_TO_WRITE_STREAM);
    return 0;
}

static int
_otap_create_cmd_ident (FILE* stream)
{
    int err;

    if((err = _otap_create_fwrite_cmd(stream, OTAP_CMD_IDENTIFY)) != 0)
        return err;
    if((err = _otap_create_fwrite_string(stream, otap_ident)) != 0)
        return err;
    return 0;
}

static int
_otap_create_cmd_update(FILE* stream)
{
    return _otap_create_fwrite_cmd(stream, OTAP_CMD_UPDATE);
}

static int
_otap_create_cmd_file_create (FILE* stream, otap_stat_t* f)
{
    int err;
    if((err = _otap_create_fwrite_cmd(stream, OTAP_CMD_FILE_CREATE)) != 0)
        return err;
    if((err = _otap_create_fwrite_string(stream, f->name)) != 0)
        return err;
    if((err = _otap_create_fwrite_mtime(stream, f->mtime)) != 0)
        return err;
    if((err = _otap_create_fwrite_mode(stream, f->mode)) != 0)
        return err;
    if((err = _otap_create_fwrite_uid(stream, f->uid)) != 0)
        return err;
    if((err = _otap_create_fwrite_gid(stream, f->gid)) != 0)
        return err;

    uint32_t size = f->size;
    if(fwrite(&size, sizeof(uint32_t), 1, stream) != 1)
        otap_error(OTAP_ERROR_UNABLE_TO_WRITE_STREAM);

    FILE* fp = otap_stat_fopen(f, "rb");
    if(fp == NULL)
        otap_error(OTAP_ERROR_UNABLE_TO_OPEN_FILE_FOR_READING);

    uint8_t buff[256];
    uintptr_t b = 256;
    for(b = 256; b == 256; )
    {
        b = fread(buff, 1, b, fp);
        if(fwrite(buff, 1, b, stream) != b)
        {
            fclose(fp);
            otap_error(OTAP_ERROR_UNABLE_TO_WRITE_STREAM);
        }
    }
    fclose(fp);
    
    return 0;
}

static int
_otap_create_cmd_file_delta(FILE        *stream,
                            otap_stat_t *a,
                            otap_stat_t *b)
{
    FILE* fpa = otap_stat_fopen(a, "rb");
    if(fpa == NULL)
        otap_error(OTAP_ERROR_UNABLE_TO_OPEN_FILE_FOR_READING);
    FILE* fpb = otap_stat_fopen(b, "rb");
    if(fpb == NULL)
    {
        fclose(fpa);
        otap_error(OTAP_ERROR_UNABLE_TO_OPEN_FILE_FOR_READING);
    }

    // Calculate start.
    uintptr_t blks[2] = { 256, 256 };
    uint8_t   buff[2][256];

    uintptr_t o;
    for(o = 0; (blks[1] == blks[0]) && (blks[0] != 0); o += blks[1])
    {
        blks[0] = fread(buff[0], 1, blks[0], fpa);
        blks[1] = fread(buff[1], 1, blks[0], fpb);
        if((blks[0] == 0) || (blks[1] == 0))
            break;

        uintptr_t i;
        for(i = 0; i < blks[1]; i++)
        {
            if(buff[0][i] != buff[1][i])
            {
                o += i;
                break;
            }
        }
        if(i < blks[1])
            break;
    }
    uint32_t start = o;

    if((fseek(fpa, 0, SEEK_END) != 0) || (fseek(fpb, 0, SEEK_END) != 0))
    {
        fclose(fpa);
        fclose(fpb);
        otap_error(OTAP_ERROR_UNABLE_TO_SEEK_THROUGH_STREAM);
    }

    // Find length.
    long flena = ftell(fpa);
    long flenb = ftell(fpb);

    if((flena < 0) || (flenb < 0))
    {
        fclose(fpa);
        fclose(fpb);
        otap_error(OTAP_ERROR_UNABLE_TO_DETECT_STREAM_POSITION);
    }

    // Find end.
    blks[0] = 256;
    blks[1] = 256;
    for(o = 0; true; o += blks[1])
    {
        blks[0] = ((flena - o) < 256     ? (flena - o) : 256    );
        blks[1] = ((flenb - o) < blks[0] ? (flenb - o) : blks[0]);
        if((blks[0] == 0) || (blks[1] == 0))
            break;

        if((fseek(fpa, flena - (o + blks[0]), SEEK_SET) != 0)
                || (fseek(fpb, flenb - (o + blks[1]), SEEK_SET) != 0))
        {
            fclose(fpa);
            fclose(fpb);
            otap_error(OTAP_ERROR_UNABLE_TO_SEEK_THROUGH_STREAM);
        }

        if((fread(buff[0], 1, blks[0], fpa) != blks[0])
                || (fread(buff[1], 1, blks[1], fpb) != blks[1]))
        {
            fclose(fpa);
            fclose(fpb);
            otap_error(OTAP_ERROR_UNABLE_TO_READ_STREAM);
        }

        uintptr_t i, ja, jb;
        for(i = 0, ja = (blks[0] - 1), jb = (blks[1] - 1); i < blks[1]; i++, ja--, jb--)
        {
            if(buff[0][ja] != buff[1][jb])
            {
                o += i;
                break;
            }
        }
        if(i < blks[1])
            break;
    }
    fclose(fpa);

    // Ensure that the start and end don't overlap for the new file.
    if((flenb - o) < start)
        o = (flenb - start);

    uint32_t end = (flena - o);
    if(end < start)
        end = start;

    uint32_t size = flenb - ((flena - end) + start); //(flenb - (o + start));

    if((end == start) && (size == 0))
    {
        fclose(fpb);
        return 0;
    }

    int err;
    if(((err = _otap_create_fwrite_cmd(stream, OTAP_CMD_FILE_DELTA)) != 0)
            || ((err = _otap_create_fwrite_string(stream, b->name)) != 0)
            || ((err = _otap_create_fwrite_mtime(stream, b->mtime)) != 0))
    {
        fclose(fpb);
        return err;
    }
    if((fwrite(&start, 4, 1, stream) != 1)
            || (fwrite(&end, 4, 1, stream) != 1)
            || (fwrite(&size, 4, 1, stream) != 1))
    {
        fclose(fpb);
        otap_error(OTAP_ERROR_UNABLE_TO_WRITE_STREAM);
    }
    if(fseek(fpb, start, SEEK_SET) != 0)
    {
        fclose(fpb);
        otap_error(OTAP_ERROR_UNABLE_TO_SEEK_THROUGH_STREAM);
    }

    for(o = 0; o < size; o += 256)
    {
        uintptr_t csize = ((size - o) > 256 ? 256 : (size - o));
        if(fread(buff[0], 1, csize, fpb) != csize)
        {
            fclose(fpb);
            otap_error(OTAP_ERROR_UNABLE_TO_READ_STREAM);
        }
        if(fwrite(buff[0], 1, csize, stream) != csize)
        {
            fclose(fpb);
            otap_error(OTAP_ERROR_UNABLE_TO_WRITE_STREAM);
        }
    }

    fclose(fpb);
    return 0;
}

static int
_otap_create_cmd_dir_create(FILE        *stream,
                            otap_stat_t *d)
{
    int err;
    
    err = _otap_create_fwrite_cmd(stream, OTAP_CMD_DIR_CREATE);
    if(err != 0)
        return err;
        
    err = _otap_create_fwrite_string(stream, d->name);
    if(err != 0)
        return err;

    err = _otap_create_fwrite_mtime(stream, d->mtime);
    if(err != 0)
        return err;

    err = _otap_create_fwrite_uid(stream, d->uid);
    if(err != 0)
        return err;
        
    err = _otap_create_fwrite_gid(stream, d->gid);
    if(err != 0)
        return err;
    
    return _otap_create_fwrite_mode (stream, d->mode);
}

static int
_otap_create_cmd_dir_enter(FILE       *stream,
                           const char *name)
{
    int err;
    if((err = _otap_create_fwrite_cmd(stream, OTAP_CMD_DIR_ENTER)) != 0)
        return err;
    return _otap_create_fwrite_string(stream, name);
}

static int
_otap_create_cmd_dir_leave(FILE      *stream,
                           uintptr_t  count)
{
    if(count == 0)
        return 0;
    int err;
    if((err = _otap_create_fwrite_cmd(stream, OTAP_CMD_DIR_LEAVE)) != 0)
        return err;

    uint8_t token;
    if(count > 256)
    {
        token = 255;
        for(; count > 256; count -= 256)
        {
            if(fwrite(&token, sizeof (uint8_t), 1, stream) != 1)
                otap_error(OTAP_ERROR_UNABLE_TO_WRITE_STREAM);
        }
    }

    token = (count - 1);
    if(fwrite(&token, 1, 1, stream) != 1)
        otap_error(OTAP_ERROR_UNABLE_TO_WRITE_STREAM);
    return 0;
}

static int
_otap_create_cmd_entity_delete (FILE       *stream,
                                const char *name)
{
    int err;
    if((err = _otap_create_fwrite_cmd(stream, OTAP_CMD_ENTITY_DELETE)) != 0)
        return err;
    return _otap_create_fwrite_string(stream, name);
}

static int
_otap_create_dir(FILE        *stream,
                 otap_stat_t *d)
{
    int err;
    if(((err =_otap_create_cmd_dir_create(stream, d)) != 0)
            || ((err = _otap_create_cmd_dir_enter(stream, d->name)) != 0))
        return err;

    uintptr_t i;
    for(i = 0; i < d->size; i++)
    {
        otap_stat_t* f = otap_stat_entry(d, i);

        if(f == NULL)
            otap_error(OTAP_ERROR_UNABLE_TO_STAT_FILE);

        switch(f->type)
        {
        case OTAP_STAT_TYPE_FILE:
            err = _otap_create_cmd_file_create(stream, f);
            break;
        case OTAP_STAT_TYPE_DIR:
            err = _otap_create_dir(stream, f);
            break;
        default:
            otap_stat_free(f);
            otap_error(OTAP_ERROR_FEATURE_NOT_IMPLEMENTED);
            break;
        }
        otap_stat_free(f);
        if(err != 0)
            return err;
    }

    return _otap_create_cmd_dir_leave(stream, 1);
}

static int
_otap_create_cmd_dir_delta (FILE        *stream,
                            otap_stat_t *a,
                            otap_stat_t *b)
{
    int err;
    uint16_t metadata_mask = OTAP_METADATA_NONE;

    /* If nothing changes we issue no command */
    if (a->mtime == b->mtime)
        metadata_mask &= OTAP_METADATA_MTIME;
    if (a->uid == b->uid)
        metadata_mask &= OTAP_METADATA_UID;
    if (a->gid == b->gid)
        metadata_mask &= OTAP_METADATA_GID;
    if (a->mode == b->mode) 
        metadata_mask &= OTAP_METADATA_MODE;
        
    if (metadata_mask == OTAP_METADATA_NONE)
        return 0;

    err = _otap_create_fwrite_cmd(stream, OTAP_CMD_DIR_DELTA);
    if (err != 0)
        return err;

    if(fwrite(&metadata_mask, sizeof (uint16_t), 1, stream) != 1)
        otap_error(OTAP_ERROR_UNABLE_TO_WRITE_STREAM);

    err = _otap_create_fwrite_mtime (stream, b->mtime);
    if (err != 0)
        return err;

    err = _otap_create_fwrite_uid (stream, b->uid);
    if (err != 0)
        return err;

    err = _otap_create_fwrite_gid (stream, b->gid);
    if (err != 0)
        return err;

    err = _otap_create_fwrite_mode (stream, b->mode);
    if (err != 0)
        return err;

    return _otap_create_fwrite_string(stream, b->name);
}

static int
_otap_create_cmd_symlink_create (FILE        *stream,
                                 otap_stat_t *symlink)
{
    int err;
    char path[PATH_BUFFER_LENGTH];
    char *slpath = otap_stat_path (symlink);
    ssize_t len = readlink (slpath, path, sizeof(path)-1);
    free (slpath);

    if (len < 0)
        return OTAP_ERROR_UNABLE_TO_READ_SYMLINK;

    path[len] = '\0';

    err = _otap_create_fwrite_cmd(stream, OTAP_CMD_SYMLINK_CREATE);
    if (err != 0)
        return err;

    err = _otap_create_fwrite_mtime (stream, symlink->mtime);
    if (err != 0)
        return err;

    err = _otap_create_fwrite_string(stream, symlink->name);
    if (err != 0)
        return err;

    return _otap_create_fwrite_string(stream, path);
}

static int
_otap_create_cmd_symlink_delta (FILE        *stream,
                                otap_stat_t *a,
                                otap_stat_t *b)
{
    int err;
    char path_a[PATH_BUFFER_LENGTH];
    char path_b[PATH_BUFFER_LENGTH];

    char *spath_a = otap_stat_path (a);
    char *spath_b = otap_stat_path (b);

    ssize_t len_a = readlink (spath_a, path_a, sizeof(path_a)-1);
    ssize_t len_b = readlink (spath_b, path_b, sizeof(path_b)-1);

    free (spath_a);
    free (spath_b);

    if (len_a < 0 || len_b < 0)
        return OTAP_ERROR_UNABLE_TO_READ_SYMLINK;

    path_a[len_a] = path_b[len_b] = '\0';

    int pathcmp = strcmp (path_a, path_b);

    /* If both symlinks are equal, we quit */
    if ((b->mtime == a->mtime) && (pathcmp != 0))
        return 0;

    /* TODO: If only mtime changes, use a mtime update cmd */
    err = _otap_create_cmd_entity_delete(stream, a->name);
    if (err != 0)
        return err;

    return _otap_create_cmd_symlink_create (stream, b);
}

static int
_otap_create_cmd_special_create (FILE        *stream,
                                 otap_stat_t *nod)
{
    int err = _otap_create_fwrite_cmd(stream, OTAP_CMD_SPECIAL_CREATE);
    if (err != 0)
        return err;

    err = _otap_create_fwrite_string(stream, nod->name);
    if (err != 0)
        return err;

    err = _otap_create_fwrite_mtime (stream, nod->mtime);
    if (err != 0)
        return err;

    err = _otap_create_fwrite_mode (stream, nod->mode);
    if (err != 0)
        return err;
        
    err = _otap_create_fwrite_uid (stream, nod->uid);
    if (err != 0)
        return err;

    err = _otap_create_fwrite_gid (stream, nod->gid);
    if (err != 0)
        return err;

    return _otap_create_fwrite_dev (stream, nod->rdev);
}

static int
_otap_create (FILE        *stream,
              otap_stat_t *a,
              otap_stat_t *b,
              bool         top)
{
    if((a == NULL) && (b == NULL))
        otap_error(OTAP_ERROR_NULL_POINTER);

    int err;
    if(((b == NULL) || ((a != NULL) && (a->type != b->type)))
            && ((err = _otap_create_cmd_entity_delete(stream, a->name)) != 0))
        return err;

    if((a == NULL) || ((b != NULL) && (a->type != b->type)))
    {
        printf ("foo\n");
        switch(b->type)
        {
        case OTAP_STAT_TYPE_FILE:
            return _otap_create_cmd_file_create(stream, b);
        case OTAP_STAT_TYPE_DIR:
            return _otap_create_dir(stream, b);
        case OTAP_STAT_TYPE_SYMLINK:
            return _otap_create_cmd_symlink_create (stream, b);
        case OTAP_STAT_TYPE_CHRDEV:
        case OTAP_STAT_TYPE_BLKDEV:
        case OTAP_STAT_TYPE_FIFO:
        case OTAP_STAT_TYPE_SOCKET:
            return _otap_create_cmd_special_create (stream, b);
        default:
            otap_error(OTAP_ERROR_FEATURE_NOT_IMPLEMENTED);
            break;
        }
    }

    switch (b->type)
    {
    case OTAP_STAT_TYPE_FILE:
        return _otap_create_cmd_file_delta(stream, a, b);
    case OTAP_STAT_TYPE_SYMLINK:
        return _otap_create_cmd_symlink_delta(stream, a, b);
    case OTAP_STAT_TYPE_CHRDEV:
    case OTAP_STAT_TYPE_BLKDEV:
    case OTAP_STAT_TYPE_FIFO:
    case OTAP_STAT_TYPE_SOCKET:
        otap_error(OTAP_ERROR_FEATURE_NOT_IMPLEMENTED);
    case OTAP_STAT_TYPE_DIR:
        _otap_create_cmd_dir_delta (stream, a, b);
    default:
        break;
    }

    if(!top && ((err = _otap_create_cmd_dir_enter(stream, b->name)) != 0))
        return err;

    // Handle changes/additions.
    uintptr_t i;
    for(i = 0; i < b->size; i++)
    {
        otap_stat_t* _b = otap_stat_entry(b, i);
        if(_b == NULL)
            otap_error(OTAP_ERROR_UNABLE_TO_STAT_FILE);
        otap_stat_t* _a = otap_stat_entry_find(a, _b->name);
        fprintf (stderr, "%p - %p\n", _a, _b);
        fprintf (stderr, "%s - %s\n", _a->name, _b->name);
        err = _otap_create(stream, _a, _b, false);
        otap_stat_free(_a);
        otap_stat_free(_b);
        if(err != 0)
            return err;
    }

    // Handle deletions.
    for(i = 0; i < a->size; i++)
    {
        otap_stat_t* _a = otap_stat_entry(a, i);
        if(_a == NULL)
            otap_error(OTAP_ERROR_UNABLE_TO_STAT_FILE);
        otap_stat_t* _b = otap_stat_entry_find(b, _a->name);
        err = (_b != NULL ? 0 : _otap_create_cmd_entity_delete(stream, _a->name));
        otap_stat_free(_b);
        otap_stat_free(_a);
        if(err != 0)
            return err;
    }

    if(!top && ((err = _otap_create_cmd_dir_leave(stream, 1)) != 0))
        return err;
    return 0;
}

int
otap_create (FILE        *stream,
             otap_stat_t *a,
             otap_stat_t *b)
{
    if((stream == NULL) || (a == NULL) || (b == NULL))
        otap_error(OTAP_ERROR_NULL_POINTER);

    int err;
    if((err = _otap_create_cmd_ident(stream)) != 0)
        return err;

    if((err = _otap_create(stream, a, b, true)) != 0)
        return err;

    return _otap_create_cmd_update(stream);
}
