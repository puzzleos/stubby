#!/bin/sh

#
# stubby smash script
#
# Copyright (c) 2020,2021 Cisco Systems, Inc. <pmoore2@cisco.com>
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
TEMP_D=""

usage() {
	echo "usage: stubby-smash.2.sh -o <output>"
	echo "         -k <kernel> -i <initrd> -c <cmdline> -s <SBAT>"
	echo ""
	echo "  Combine the <kernel>, <initrd>, <cmdline> and <SBAT> files"
	echo "  into a single bootable EFI app in <output>."
	echo ""
	exit 1
}

cleanup() {
    if [ -d "${TEMP_D}" ]; then
        rm -Rf "${TEMP_D}"
    fi
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

# dependency checks
deps objcopy

# argument parsing
arg_kernel=""
arg_initrd=""
arg_cmdline=""
arg_sbat=""
arg_output=""
while getopts ":o:k:i:c:s:" opt; do
	case $opt in
	o)
		arg_output=$OPTARG
		;;
	k)
		arg_kernel=$OPTARG
		;;
	i)
		arg_initrd=$OPTARG
		;;
	c)
		arg_cmdline=$OPTARG
		;;
	s)
		arg_sbat=$OPTARG
		;;
	*)
		usage
	;;
	esac
done
shift $(( $OPTIND - 1 ))

# sanity checks
assert_file "$STUBBY" "stubby efi file"
assert_file "$arg_kernel" "kernel argument"
assert_file "$arg_initrd" "initrd argument"
assert_file "$arg_cmdline" "cmdline argument"
assert_file "$arg_sbat" "sbat argument"

# output check
[ -n "$arg_output" ] || error "Must provide output option (-o FILE.efi)"
[ ! -e "$arg_output" ] ||
	error "output file '$arg_output' already exists"

TEMP_D=$(mktemp -d "${TMPDIR:-/tmp}/${0##*/}.XXXXXX") ||
    error "failed to make a temp dir"
trap cleanup EXIT

# adjust cmdline to not include trailing newline
cmdline="${TEMP_D}/cmdline"
lines=$(wc -l < "$arg_cmdline")
if [ "$lines" -eq 0 ]; then
    # zero lines is "no trailing newline"
    cmdline="$arg_cmdline"
elif [ "$lines" -eq 1 ]; then
    # one line.. just a trailing newline, strip it.
    tr -d '\n' < "$arg_cmdline" > "$cmdline" ||
        error "Failed to remove newline from $arg_cmdline"
else
    error "$arg_cmdline had $lines lines. Expected 0 or 1."
fi


exec objcopy \
	"--add-section=.cmdline=$cmdline" \
		"--change-section-vma=.cmdline=0x30000" \
	"--add-section=.sbat=$arg_sbat" \
		"--change-section-vma=.sbat=0x50000" \
		"--set-section-alignment=.sbat=512" \
	"--add-section=.linux=$arg_kernel" \
		"--change-section-vma=.linux=0x1000000" \
	"--add-section=.initrd=$arg_initrd" \
		"--change-section-vma=.initrd=0x3000000" \
	"$STUBBY" "$arg_output"
