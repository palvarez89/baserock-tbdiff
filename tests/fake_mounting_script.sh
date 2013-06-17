#!/bin/sh
#
# Copyright (c) 2013 Codethink Ltd.
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

set -eu


usage() {
    echo usage: "$(basename $0) fake_mounting_point" >&2
    exit 1
}



if [ "$#" != 1 ]; then
    usage $0
fi


fake_mounting_point="$1"

cp -a "$mounting_script_test_dir"/* "$fake_mounting_point"

