/*
 * stubby
 *
 * Copyright (c) 2020 Cisco Systems, Inc. <pmoore2@cisco.com>
 *
 * This file was originally copied from systemd under the LGPL-2.1+ license
 * and that license has been preserved in this project.  The systemd source
 * repository can be found at https://github.com/systemd/systemd.
 *
 */

/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <efi.h>
#include <efilib.h>

#include "linux.h"
#include "util.h"

#ifdef __i386__
#define __regparm0__ __attribute__((regparm(0)))
#else
#define __regparm0__
#endif

/*
 * This is the Linux-specific EFI handover entry, rather than the ordinary
 * PE/COFF EFI entry point.  It uses the native Linux C ABI.
 */
typedef VOID(*handover_f)(EFI_HANDLE image, EFI_SYSTEM_TABLE *table,
			 struct boot_params *params) __regparm0__;
static VOID linux_efi_handover(EFI_HANDLE image, struct boot_params *params)
{
	handover_f handover;
	UINTN start = (UINTN)params->hdr.code32_start;
	UINTN handover_start = start;

#ifdef __x86_64__
	handover_start += 512;
	DPRINT(L"stubby: Linux handover arguments: image 0x%lx, table 0x%lx, boot params 0x%lx\n",
	      (UINTN)image, (UINTN)ST, (UINTN)params);
	DPRINT(L"stubby: Linux handover entry is 0x%lx\n",
	      handover_start + params->hdr.handover_offset);
	asm volatile ("cli");
	start += 512;
#else
	DPRINT(L"stubby: Linux handover entry is 0x%lx\n",
	      handover_start + params->hdr.handover_offset);
#endif
	handover = (handover_f)(start + params->hdr.handover_offset);
	handover(image, ST, params);
}

EFI_STATUS linux_exec(EFI_HANDLE image,
		      CHAR8 *cmdline, UINTN cmdline_len,
		      UINTN linux_addr, UINTN linux_size,
		      UINTN initrd_addr, UINTN initrd_size)
{
	struct boot_params *image_params;
	struct boot_params *boot_params;
	UINT8 setup_sectors;
	UINT32 handover_offset;
	UINTN i;
	EFI_PHYSICAL_ADDRESS addr;
	EFI_STATUS err;

	DPRINT(L"stubby: Linux image at 0x%lx (%lu bytes), initrd at 0x%lx (%lu bytes)\n",
	      linux_addr, linux_size, initrd_addr, initrd_size);
	if (linux_size < sizeof(*image_params)) {
		Print(L"stubby: .linux section is too small for Linux boot parameters (%lu < %lu)\n",
		      linux_size, (UINTN)sizeof(*image_params));
		return EFI_LOAD_ERROR;
	}
	image_params = (struct boot_params *) linux_addr;

	DPRINT(L"stubby: Linux header: boot flag 0x%x, magic 0x%x, version 0x%x, setup sectors %u, relocatable %u\n",
	      image_params->hdr.boot_flag, image_params->hdr.header,
	      image_params->hdr.version, image_params->hdr.setup_sects,
	      image_params->hdr.relocatable_kernel);
	DPRINT(L"stubby: Linux EFI capabilities: xloadflags 0x%04x, handover offset 0x%x\n",
	      image_params->hdr.xloadflags, image_params->hdr.handover_offset);
	if (image_params->hdr.boot_flag != 0xAA55) {
		Print(L"stubby: invalid Linux boot flag (expected 0xaa55)\n");
		return EFI_LOAD_ERROR;
	}
	if (image_params->hdr.header != SETUP_MAGIC) {
		Print(L"stubby: invalid Linux setup magic (expected HdrS)\n");
		return EFI_LOAD_ERROR;
	}
	if (image_params->hdr.version < 0x20b) {
		Print(L"stubby: Linux boot protocol 0x%x is older than required 0x20b\n",
		      image_params->hdr.version);
		return EFI_LOAD_ERROR;
	}
	if (image_params->hdr.handover_offset == 0) {
		Print(L"stubby: Linux kernel does not provide an EFI handover entry\n");
		return EFI_UNSUPPORTED;
	}
	handover_offset = image_params->hdr.handover_offset;
	/* xloadflags is present from boot protocol 2.12 onwards. */
	if (image_params->hdr.version >= 0x20c &&
	    !(image_params->hdr.xloadflags & XLF_EFI_HANDOVER_64)) {
		Print(L"stubby: Linux kernel does not advertise 64-bit EFI handover support\n");
		return EFI_UNSUPPORTED;
	}
	if (!image_params->hdr.relocatable_kernel) {
		Print(L"stubby: Linux kernel is not relocatable\n");
		return EFI_LOAD_ERROR;
	}
	if (linux_addr > 0xffffffff || initrd_addr > 0xffffffff ||
	    initrd_size > 0xffffffff) {
		Print(L"stubby: kernel or initrd exceeds the 32-bit Linux boot-parameter address range\n");
		return EFI_LOAD_ERROR;
	}

	boot_params = (struct boot_params *) 0xFFFFFFFF;
	err = uefi_call_wrapper(BS->AllocatePages, 4,
				AllocateMaxAddress,
				EfiLoaderData,
				EFI_SIZE_TO_PAGES(0x4000),
				(EFI_PHYSICAL_ADDRESS *) &boot_params);
	if (EFI_ERROR(err))
		Print(L"stubby: unable to allocate boot parameters: %r\n", err);
	if (EFI_ERROR(err))
		return err;
	DPRINT(L"stubby: boot parameters allocated at 0x%lx\n", (UINTN)boot_params);

	ZeroMem(boot_params, 0x4000);
	/*
	 * Copy the packed Linux setup-header wire format byte-for-byte, then
	 * validate the handover field before using it.
	 */
	for (i = 0; i < sizeof(struct setup_header); i++)
		((UINT8 *)&boot_params->hdr)[i] =
			((const UINT8 *)&image_params->hdr)[i];
	if (boot_params->hdr.handover_offset != handover_offset) {
		Print(L"stubby: setup-header copy changed handover offset from 0x%x to 0x%x\n",
		      handover_offset, boot_params->hdr.handover_offset);
		return EFI_LOAD_ERROR;
	}
	boot_params->hdr.type_of_loader = 0xff;
	setup_sectors = image_params->hdr.setup_sects > 0 ?
			image_params->hdr.setup_sects : 4;
	boot_params->hdr.code32_start = (UINT32)linux_addr + \
					(setup_sectors + 1) * 512;
	if ((UINTN)(setup_sectors + 1) * 512 >= linux_size ||
	    (UINTN)(setup_sectors + 2) * 512 +
	    boot_params->hdr.handover_offset >= linux_size) {
		Print(L"stubby: Linux setup or handover offset lies outside .linux section\n");
		return EFI_LOAD_ERROR;
	}
	DPRINT(L"stubby: kernel code start 0x%x, handover offset 0x%x\n",
	      boot_params->hdr.code32_start, boot_params->hdr.handover_offset);

	if (cmdline) {
		addr = 0xA0000;
		err = uefi_call_wrapper(BS->AllocatePages, 4,
					AllocateMaxAddress,
					EfiLoaderData,
					EFI_SIZE_TO_PAGES(cmdline_len + 1),
					&addr);
		if (EFI_ERROR(err))
			Print(L"stubby: unable to allocate command line buffer: %r\n", err);
		if (EFI_ERROR(err))
			return err;
		CopyMem((VOID *)(UINTN)addr, cmdline, cmdline_len);
		((CHAR8 *)(UINTN)addr)[cmdline_len] = 0;
		boot_params->hdr.cmd_line_ptr = (UINT32)addr;
		DPRINT(L"stubby: command line copied to 0x%lx (%lu bytes)\n",
		      (UINTN)addr, cmdline_len);
	}

	boot_params->hdr.ramdisk_image = (UINT32)initrd_addr;
	boot_params->hdr.ramdisk_size = (UINT32)initrd_size;
	DPRINT(L"stubby: boot parameters set: initrd 0x%x (%u bytes), cmdline 0x%x\n",
	      boot_params->hdr.ramdisk_image, boot_params->hdr.ramdisk_size,
	      boot_params->hdr.cmd_line_ptr);
	DPRINT(L"stubby: transferring control to Linux EFI handover\n");

	linux_efi_handover(image, boot_params);
	Print(L"stubby: Linux EFI handover unexpectedly returned\n");
	return EFI_LOAD_ERROR;
}
