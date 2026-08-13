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

# The initrd must begin after both the raw kernel file and the kernel's
# advertised in-memory footprint (setup_header.init_size).  Keep the section
# start on a 16 MiB boundary, matching the kernel alignment used by current
# x86 EFI images.  INITRD_VMA may be explicitly overridden by the environment
# for a fixed layout; that override is checked below.
LINUX_VMA=${LINUX_VMA:-0x1000000}
INITRD_ALIGNMENT=${INITRD_ALIGNMENT:-0x1000000}
INITRD_VMA=${INITRD_VMA:-}
LINUX_SETUP_MAGIC=1400005704 # 0x53726448, "HdrS"

usage() {
	echo "usage: stubby-smash.2.sh -o <output>"
	echo "         -k <kernel> -i <initrd> -c <cmdline> -s <SBAT>"
	echo ""
	echo "  Combine the <kernel>, <initrd>, <cmdline> and <SBAT> files"
	echo "  into a single bootable EFI app in <output>."
	echo ""
	exit 1
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

read_u32_le() {
	value=$(od -An -v -tu4 -j "$2" -N 4 "$1") || return 1
	set -- $value
	[ $# -eq 1 ] && [ -n "$1" ] || return 1
	printf '%s\n' "$1"
}

check_kernel_layout() {
	kernel_size=$(wc -c < "$arg_kernel") ||
		error "unable to determine kernel size"
	setup_magic=$(read_u32_le "$arg_kernel" 514) ||
		error "kernel argument '$arg_kernel' is too small to contain a Linux setup header"
	[ "$setup_magic" -eq "$LINUX_SETUP_MAGIC" ] ||
		error "kernel argument '$arg_kernel' does not contain a Linux HdrS setup header"
	init_size=$(read_u32_le "$arg_kernel" 608) ||
		error "kernel argument '$arg_kernel' is too small to contain init_size"

	linux_footprint=$kernel_size
	if [ "$init_size" -gt "$linux_footprint" ]; then
		linux_footprint=$init_size
	fi
	# test(1) does not consistently accept hexadecimal operands, while POSIX
	# shell arithmetic does.  Normalize before validating or using it below.
	initrd_alignment=$((INITRD_ALIGNMENT))
	[ "$initrd_alignment" -gt 0 ] 2>/dev/null ||
		error "INITRD_ALIGNMENT must be a positive integer"

	raw_linux_end=$((LINUX_VMA + kernel_size))
	linux_end=$((LINUX_VMA + linux_footprint))
	if [ -z "$INITRD_VMA" ]; then
		# Round the required end up; do not assume init_size is a multiple of
		# the kernel's advertised alignment.
		INITRD_VMA=$((((linux_end + initrd_alignment - 1) / initrd_alignment) * initrd_alignment))
	fi

	if [ "$INITRD_VMA" -lt "$raw_linux_end" ]; then
		error "initrd VMA 0x$(printf '%X' "$INITRD_VMA") overlaps raw .linux bytes ending at 0x$(printf '%X' "$raw_linux_end")"
	fi
	if [ "$INITRD_VMA" -lt "$linux_end" ]; then
		error "initrd VMA 0x$(printf '%X' "$INITRD_VMA") is below Linux init_size end 0x$(printf '%X' "$linux_end")"
	fi
	printf 'stubby-smash: .linux %s bytes, init_size %s bytes; .initrd RVA 0x%X\n' \
		"$kernel_size" "$init_size" "$INITRD_VMA"
}

# main
#

# dependency checks
deps objcopy od wc

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
check_kernel_layout

# output check
[ ! -e "$arg_output" ] ||
	error "output file '$arg_output' already exists"

exec objcopy \
	"--add-section=.cmdline=$arg_cmdline" \
		"--change-section-vma=.cmdline=0x30000" \
	"--add-section=.sbat=$arg_sbat" \
		"--change-section-vma=.sbat=0x50000" \
		"--set-section-alignment=.sbat=512" \
	"--add-section=.linux=$arg_kernel" \
		"--change-section-vma=.linux=$LINUX_VMA" \
	"--add-section=.initrd=$arg_initrd" \
		"--change-section-vma=.initrd=$INITRD_VMA" \
	"$STUBBY" "$arg_output"
