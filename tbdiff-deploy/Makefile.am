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
# vi:set ts=8 sw=8 noet ai nocindent:

bin_PROGRAMS =								\
	tbdiff-deploy

tbdiff_deploy_SOURCES =							\
	main.c

tbdiff_deploy_CFLAGS =							\
	-I$(top_srcdir)							\
	$(PLATFORM_CFLAGS)						\
	$(PLATFORM_CPPFLAGS)

tbdiff_deploy_LDFLAGS =							\
	-no-undefined							\
	$(PLATFORM_LDFLAGS)

tbdiff_deploy_DEPENDENCIES =						\
	$(top_builddir)/tbdiff/libtbdiff-$(TBDIFF_VERSION_API).la

tbdiff_deploy_LDADD =							\
	$(top_builddir)/tbdiff/libtbdiff-$(TBDIFF_VERSION_API).la
