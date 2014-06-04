/*
 *    Copyright (C) 2011-2012 Codethink Ltd.
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

#ifndef _TBDIFF_XATTRS_H
#define _TBDIFF_XATTRS_H

#include <stddef.h>
#include <stdint.h>

/* structure for names data */
struct tbd_xattrs_names {
	char const *begin;
	char const *end;
};

/* gets a list of the names of the file referenced by path */
int  tbd_xattrs_names(char const *path, struct tbd_xattrs_names *names_out);

/* frees up the INTERNAL resources of the list, doesn't free the list itself */
void tbd_xattrs_names_free(struct tbd_xattrs_names *names);

/* calls f with every name in the list */
int  tbd_xattrs_names_each(struct tbd_xattrs_names const *names,
                                  int (*f)(char const *name, void *ud),
                                  void *ud);

/* gets how many different attributes there are in the list */
int tbd_xattrs_names_count(struct tbd_xattrs_names const *names, uint32_t *count);

/* puts the value of the attribute called name into *buf with size *bufsize
 * if *buf is NULL or *bufsize is 0 then memory will be allocated for it
 * if *buf was too small it will be reallocated
 * if it is successful, *buf will contain *valsize bytes of data
 */
int  tbd_xattrs_get(char const *path, char const* name, void **buf,
                           size_t *bufsize, size_t *valsize);

/* removes all attributes of the file referenced by path */
int  tbd_xattrs_removeall(char const *path);

/* calls f for every attribute:value pair in the list */
int  tbd_xattrs_pairs(struct tbd_xattrs_names const *names, char const *path,
                      int (*f)(char const *, void const *, size_t, void *),
                      void *ud);
#endif /* !__TBDIFF_XATTRS_H__ */
