# vi:set ts=8 sw=8 noet ai nocindent:
# -
# Copyright (c) 2011-2014 Codethink Ltd.
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

SUBDIRS =								\
	tbdiff								\
	tbdiff-create							\
	tbdiff-deploy							\
	tb-switch							\
	tb-update							\
	baserock-system-config-sync					\
	system-version-manager						\
	tests

.PHONY: ChangeLog

ChangeLog: Makefile
	(GIT_DIR=$(top_srcdir)/.git git log > .changelog.tmp \
	&& mv .changelog.tmp ChangeLog; rm -f .changelog.tmp) \
	|| (touch ChangeLog; echo 'Git directory not found: installing possibly empty changelog.' >&2)

dist-hook: ChangeLog

TESTS = yarns/run_yarns.sh

noinst_PROGRAMS = symtime sockbind
symtime_SOURCES = yarns/symtime.c
sockbind_SOURCES = yarns/sockbind.c
