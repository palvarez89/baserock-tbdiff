# vi:set et ai sw=2 sts=2 ts=2: */
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

dnl ***************************
dnl *** Version information ***
dnl ***************************
m4_define([tbdiff_verinfo], [0:0:0])
m4_define([tbdiff_version_api_major], [1])
m4_define([tbdiff_version_major], [0])
m4_define([tbdiff_version_minor], [0])
m4_define([tbdiff_version_micro], [0])
m4_define([tbdiff_version], [tbdiff_version_major().tbdiff_version_minor().tbdiff_version_micro()])

dnl ************************************
dnl *** Minimum debugging by default ***
dnl ************************************
m4_define([tbdiff_debug_default], [minimum])

dnl ***************************
dnl *** Initialize autoconf ***
dnl ***************************
AC_COPYRIGHT([Copyright (c) 2011-2012 Codethink Ltd])
AC_INIT([tbdiff], [tbdiff_version], [baserock-dev@baserock.org])
AC_PREREQ([2.50])
AC_CANONICAL_TARGET()

dnl ***************************
dnl *** Initialize automake ***
dnl ***************************
AM_INIT_AUTOMAKE([1.8 dist-bzip2 tar-ustar])
AC_CONFIG_MACRO_DIR([m4])
AC_SUBST([ACLOCAL_AMFLAGS], ["$ACLOCAL_FLAGS -I m4"])
AM_CONFIG_HEADER([config.h])
AM_MAINTAINER_MODE()
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

dnl **************************
dnl *** Libtool versioning ***
dnl **************************
TBDIFF_VERINFO=tbdiff_verinfo()
AC_SUBST([TBDIFF_VERINFO])

dnl ****************************
dnl *** Subst tbdiff version ***
dnl ****************************
TBDIFF_VERSION_MAJOR=tbdiff_version_major()
TBDIFF_VERSION_MINOR=tbdiff_version_minor()
TBDIFF_VERSION_MICRO=tbdiff_version_micro()
TBDIFF_VERSION_API_MAJOR=tbdiff_version_api_major()
TBDIFF_VERSION_API=$TBDIFF_VERSION_API_MAJOR
AC_SUBST([TBDIFF_VERSION_MAJOR])
AC_SUBST([TBDIFF_VERSION_MINOR])
AC_SUBST([TBDIFF_VERSION_MICRO])
AC_SUBST([TBDIFF_VERSION_API_MAJOR])
AC_SUBST([TBDIFF_VERSION_API])

dnl *******************************
dnl *** Check for UNIX variants ***
dnl *******************************
AC_AIX()
AC_ISC_POSIX()
AC_MINIX()

dnl ********************************
dnl *** Check for basic programs ***
dnl ********************************
AM_PROG_AS()
AC_PROG_CC()
AM_PROG_CC_C_O()
AC_PROG_INSTALL()

dnl ***********************
dnl *** Prepare libtool ***
dnl ***********************
LT_PREREQ([2.2.6])
LT_INIT([disable-static])

dnl ***************************************
dnl *** Check for standard header files ***
dnl ***************************************
AC_HEADER_STDC()
AC_CHECK_HEADERS([assert.h attr/xattr.h dirent.h endian.h errno.h fcntl.h \
                  inttypes.h stdbool.h stddef.h stdint.h stdio.h stdlib.h \
                  string.h sys/stat.h sys/time.h sys/types.h time.h unistd.h \
                  utime.h])

dnl ************************************
dnl *** Check for standard functions ***
dnl ************************************
dnl AC_CHECK_FUNCS([])

dnl ************************************
dnl *** Check for required libraries ***
dnl ************************************
AC_CHECK_LIB([attr], [lsetxattr])

dnl ***********************************
dnl *** Check for required packages ***
dnl ***********************************
dnl PKG_CHECK_MODULES([...], [...])

AC_OUTPUT([
Makefile
tbdiff/Makefile
tbdiff/tbdiff-1.pc
tbdiff-create/Makefile
tbdiff-deploy/Makefile
tb-switch/Makefile
tb-update/Makefile
baserock-system-config-sync/Makefile
system-version-manager/Makefile
tests/Makefile
])

dnl ***************************
dnl *** Print configuration ***
dnl ***************************
echo
echo "Build Configuration:"
echo
echo "  * Debug:                                     $enable_debug"
echo
