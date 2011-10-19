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

#include "libtbd_xattrs.h"
#include "tbdiff.h"
#include <string.h>
#include <stdlib.h>

#include <attr/xattr.h>
#include <errno.h>

int tbd_xattrs_names(char const *path, tbd_xattrs_names_t *names)
{
	char *attrnames = NULL;
	/* get size of names list */
	ssize_t size = llistxattr(path, NULL, 0);
	if (size < 0) {
		if (errno == ENOSYS || errno == ENOTSUP) {
			return TBD_ERROR(TBD_ERROR_XATTRS_NOT_SUPPORTED);
		} else {
			return TBD_ERROR(TBD_ERROR_FAILURE);
		}
	}

	if (size == 0) {
		names->begin = NULL;
		names->end = NULL;
		return TBD_ERROR_SUCCESS;
	}

	while (1) {
		{ /* allocate memory for list */
			/* allocate 1 more for NUL terminator */
			char *allocres = realloc(attrnames, size + 1);
			if (allocres == NULL) {
				free(attrnames);
				return TBD_ERROR(TBD_ERROR_OUT_OF_MEMORY);
			}
			attrnames = allocres;
		}

		{ /* try to read names list */
			ssize_t listres = llistxattr(path, attrnames, size);
			if (listres >= 0) {
				/* succeeded, save size */
				size = listres;
				break;
			}
			if (errno != ERANGE) {
				/* other error, can't fix */
				free(attrnames);
				return TBD_ERROR(TBD_ERROR_FAILURE);
			}
			/* not big enough, enlarge and try again */ 
			size *= 2;
			errno = 0;
		}
	}
	/* ensure NUL terminated */
	attrnames[size] = '\0';
	names->begin = attrnames;
	names->end = attrnames + size;
	return TBD_ERROR_SUCCESS;
}

void tbd_xattrs_names_free(tbd_xattrs_names_t *names)
{
	free((void *)names->begin);
}

int tbd_xattrs_names_each(tbd_xattrs_names_t const *names,
                          int (*f)(char const *name, void *ud), void *ud)
{
	char const *name;
	int err = TBD_ERROR_SUCCESS;
	for (name = names->begin; name != names->end;
	     name = strchr(name, '\0') + 1) {
		if ((err = f(name, ud)) != TBD_ERROR_SUCCESS) {
			return err;
		}
	}
	return err;
}

static int names_sum(char const *name, void *ud) {
	if (name == NULL || ud == NULL)
		return TBD_ERROR(TBD_ERROR_NULL_POINTER);
	(*((uint16_t*)ud))++;
	return TBD_ERROR_SUCCESS;
}
int tbd_xattrs_names_count(tbd_xattrs_names_t const *names, int *count) {
	int _count = 0;
	int err;
	if ((err = tbd_xattrs_names_each(names, &names_sum, &_count)) ==
	    TBD_ERROR_SUCCESS) {
		*count = _count;
	}
	return err;
}

static int name_remove(char const *name, void *ud) {
	char const *path = ud;
	if (lremovexattr(path, name) < 0) {
		switch (errno) {
		case ENOATTR:
			return TBD_ERROR(TBD_ERROR_XATTRS_MISSING_ATTR);
		case ENOTSUP:
			return TBD_ERROR(TBD_ERROR_XATTRS_NOT_SUPPORTED);
		default:
			return TBD_ERROR(TBD_ERROR_FAILURE);
		}
	}
	return TBD_ERROR_SUCCESS;
}
int tbd_xattrs_removeall(char const *path)
{
	int err = TBD_ERROR_SUCCESS;
	tbd_xattrs_names_t list;

	/* get the list of attributes */
	if ((err = tbd_xattrs_names(path, &list)) != TBD_ERROR_SUCCESS) {
		return err;
	}

	err = tbd_xattrs_names_each(&list, &name_remove, (void*)path);

	tbd_xattrs_names_free(&list);
	return err;
}

int tbd_xattrs_get(char const *path, char const* name, void **buf,
                   size_t *bufsize, size_t *valsize)
{
	ssize_t toalloc = *bufsize;
	if (toalloc == 0 || *buf == NULL) {
		toalloc = lgetxattr(path, name, NULL, 0);
		if (toalloc < 0) {
			if (errno == ENOSYS || errno == ENOTSUP) {
				return TBD_ERROR(TBD_ERROR_XATTRS_NOT_SUPPORTED);
			} else {
				return TBD_ERROR(TBD_ERROR_FAILURE);
			}
		}
		{
			void *allocres = malloc(toalloc);
			if (allocres == NULL) {
				return TBD_ERROR(TBD_ERROR_OUT_OF_MEMORY);
			}
			*buf = allocres;
			*bufsize = toalloc;
		}
		
	}
	while (1) {
		{ /* try to get value */
			ssize_t getres = lgetxattr(path, name, *buf, *bufsize);
			if (getres >= 0) {
				/* succeeded, save size */
				*valsize = getres;
				break;
			}
			if (errno != ERANGE) {
				/* other error, can't fix */
				return TBD_ERROR(TBD_ERROR_FAILURE);
			}
			/* not big enough, enlarge and try again */ 
			toalloc *= 2;
			errno = 0;
		}

		{ /* allocate memory for list */
			void *allocres = realloc(*buf, toalloc);
			if (allocres == NULL) {
				return TBD_ERROR(TBD_ERROR_OUT_OF_MEMORY);
			}
			*buf = allocres;
			*bufsize = toalloc;
		}
	}
	return TBD_ERROR_SUCCESS;
}

typedef struct {
	char const *path;
	tbd_xattrs_pairs_callback_t f;
	void *pairs_ud;
	void *data;
	size_t data_size;
} tbd_xattrs_pairs_params_t;
static int call_with_data(char const *name, void *ud)
{
	tbd_xattrs_pairs_params_t *params;
	params = ud;
	size_t value_size;
	int err;
	if ((err = tbd_xattrs_get(params->path, name, &(params->data),
	                          &(params->data_size), &value_size)) !=
	    TBD_ERROR_SUCCESS) {
		return err;
	}
	return params->f(name, params->data, value_size, params->pairs_ud);
}
int tbd_xattrs_pairs(tbd_xattrs_names_t const *names, char const *path,
                     tbd_xattrs_pairs_callback_t f, void *ud)
{
	tbd_xattrs_pairs_params_t params = {
		path, f, ud, NULL, 0,
	};
	int err = tbd_xattrs_names_each(names, &call_with_data, &params);
	free(params.data);
	return err;
}
