#!/bin/sh

#
# stubby smash script
#
# Copyright (c) 2020 Cisco Systems, Inc. <pmoore2@cisco.com>
#

#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

STUBBY="stubby.efi"

usage() {
	echo "usage: stubby-smash.1.sh <kernel> <initrd> <cmdline> <output>"
	echo ""
	echo "  Combine the <kernel>, <initrd>, and <cmdline> files into a"
	echo "  single bootable EFI application in <output>."
	echo ""
}

error() {
	echo "error:" "$@" 1>&2
	exit 1
}

deps() {
	while [ $# -ne 0 ]; do
		command -v "$1" >/dev/null 2>&1 ||
			error "please ensure \"$1\" is installed"
		shift
	done
}

assert_file() {
	[ -f "$1" ] || error "$2 '$1' is not a file"
	[ -r "$1" ] || error "$2 '$1' file is not readable"
}

# main
#

if [ "$1" = "-h" -o "$1" = "--help" ]; then
	usage
	exit 0
fi
if [ $# -ne 4 ]; then
	usage 1>&2;
	error "got $# arguments expected 4"
fi

arg_kernel=$1
arg_initrd=$2
arg_cmdline=$3
arg_output=$4

# dependency checks
deps objcopy

# sanity checks
assert_file "$STUBBY" "stubby efi file"
assert_file "$arg_kernel" "kernel argument"
assert_file "$arg_initrd" "initrd argument"
assert_file "$arg_cmdline" "cmdline argument"

# output check
[ ! -e "$arg_output" ] ||
	error "output file '$arg_output' already exists"

exec objcopy \
	"--add-section=.cmdline=$arg_cmdline" \
		"--change-section-vma=.cmdline=0x30000" \
	"--add-section=.linux=$arg_kernel" \
		"--change-section-vma=.linux=0x2000000" \
	"--add-section=.initrd=$arg_initrd" \
		"--change-section-vma=.initrd=0x3000000" \
	"$STUBBY" "$arg_output"
