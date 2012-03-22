# vi:set ts=8 sw=8 noet ai nocindent:
# -
# Copyright (c) 2011-2012 Codethink Ltd.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License Version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

lib_LTLIBRARIES =							\
	libtbdiff-1.la

libtbdiff_headers =							\
	tbdiff-common.h							\
	tbdiff-io.h							\
	tbdiff-stat.h							\
	tbdiff-xattrs.h							\
	tbdiff-private.h						\
	tbdiff.h

libtbdiff_sources =							\
	tbdiff-apply.c							\
	tbdiff-create.c							\
	tbdiff-io.c							\
	tbdiff-stat.c							\
	tbdiff-xattrs.c

libtbdiffincludedir = $(includedir)/tbdiff-$(TBDIFF_VERSION_API)/tbdiff

libtbdiffinclude_HEADERS =						\
	$(libtbdiff_headers)

libtbdiff_1_la_SOURCES =						\
	$(libtbdiff_sources)						\
	$(libtbdiff_headers)

libtbdiff_1_la_CFLAGS =							\
	-DDATADIR=\"$(datadir)\"					\
	-DTBDIFF_COMPILATION						\
	-DTBDIFF_VERSION_API=\"$(TBDIFF_VERSION_API)\"			\
	-I$(top_srcdir)							\
	$(PLATFORM_CFLAGS)						\
	$(PLATFORM_CPPFLAGS)

libtbdiff_1_la_LDFLAGS =						\
	-no-undefined							\
	-export-dynamic							\
	-version-info $(TBDIFF_VERINFO)					\
	$(PLATFORM_LDFLAGS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = tbdiff-1.pc
