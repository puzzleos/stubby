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

#include "disk.h"
#include "kcmdline.h"
#include "linux.h"
#include "pe.h"
#include "util.h"

/* magic string to find in the binary image */
static const char __attribute__((used)) magic[] =
	"#### LoaderInfo: stubby " GIT_VERSION " ####";

BOOLEAN use_shell_cmdline(UINTN len)
{
	EFI_STATUS err;
	UINTN i;

	// If cmdline file is missing then use the shell's
	if (len == 0)
		return TRUE;

	// Otherwise require StubbyIgnoreCmdlineSection UEFI var to be 1
	err = efivar_get_int(L"StubbyIgnoreCmdlineSection", &i);
	return err == EFI_SUCCESS && i == 1;
}

static const EFI_GUID global_guid = EFI_GLOBAL_VARIABLE;

EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *sys_table)
{
	EFI_LOADED_IMAGE *loaded_image;
	_cleanup_freepool_ CHAR8 *b = NULL;
	UINTN size;
	BOOLEAN secure = FALSE;
	CHAR8 *sections[] = {
		(CHAR8 *)".cmdline",
		(CHAR8 *)".linux",
		(CHAR8 *)".initrd",
		NULL
	};
	UINTN addrs[ELEMENTSOF(sections) - 1] = {};
	UINTN offs[ELEMENTSOF(sections) - 1] = {};
	UINTN szs[ELEMENTSOF(sections) - 1] = {};
	CHAR8 *cmdline = NULL;
	UINTN cmdline_len;
	CHAR16 uuid[37];
	EFI_STATUS err;

	InitializeLib(image, sys_table);

	err = uefi_call_wrapper(BS->OpenProtocol, 6,
				image, &LoadedImageProtocol,
				(VOID **)&loaded_image,
				image, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	if (EFI_ERROR(err)) {
		Print(L"Error getting a LoadedImageProtocol handle: %r\n", err);
		uefi_call_wrapper(BS->Stall, 1, 3 * 1000 * 1000);
		return err;
	}

	if (efivar_get_raw(&global_guid,
			   L"SecureBoot", &b, &size) == EFI_SUCCESS)
		if (*b > 0)
			secure = TRUE;

	err = pe_memory_locate_sections(loaded_image->ImageBase,
					sections, addrs, offs, szs);
	if (EFI_ERROR(err)) {
		Print(L"Unable to locate embedded .linux section: %r\n", err);
		uefi_call_wrapper(BS->Stall, 1, 3 * 1000 * 1000);
		return err;
	}

	if (szs[0] > 0)
		cmdline = (CHAR8 *)(loaded_image->ImageBase + addrs[0]);

	cmdline_len = szs[0];

	/* if we are not in secure boot mode, or none was provided, accept a
	 * custom command line and replace the built-in one */
	if ((!secure || cmdline_len == 0) &&
	    loaded_image->LoadOptionsSize > 0 &&
	    use_shell_cmdline(cmdline_len) &&
	    *(CHAR16 *)loaded_image->LoadOptions > 0x1F) {
		CHAR16 *options;
		CHAR8 *line;
		UINTN i;

		options = (CHAR16 *)loaded_image->LoadOptions;
		cmdline_len = (loaded_image->LoadOptionsSize /
			       sizeof(CHAR16)) * sizeof(CHAR8);
		line = AllocatePool(cmdline_len);
		for (i = 0; i < cmdline_len; i++)
			line[i] = options[i];
		cmdline = line;

		err = check_cmdline(cmdline, cmdline_len);
		if (EFI_ERROR(err)) {
			if (secure) {
				Print(L"Custom kernel command line rejected\n");
				return err;
			} else {
				Print(L"Custom kernel would be rejected in secure mode\n");
			}
		}
	}

	/* export the device path we are started from, if it's not set yet */
	if (efivar_get_raw(&loader_guid, L"LoaderDevicePartUUID", NULL,
			   NULL) != EFI_SUCCESS)
		if (disk_get_part_uuid(loaded_image->DeviceHandle,
				       uuid) == EFI_SUCCESS)
			efivar_set(L"LoaderDevicePartUUID", uuid, FALSE);

	/* if LoaderImageIdentifier is not set, assume the image with this stub
	 * was loaded directly from UEFI */
	if (efivar_get_raw(&loader_guid, L"LoaderImageIdentifier", NULL,
			   NULL) != EFI_SUCCESS) {
		_cleanup_freepool_ CHAR16 *s;

		s = DevicePathToStr(loaded_image->FilePath);
		efivar_set(L"LoaderImageIdentifier", s, FALSE);
	}

	/* if LoaderFirmwareInfo is not set, let's set it */
	if (efivar_get_raw(&loader_guid, L"LoaderFirmwareInfo", NULL,
			   NULL) != EFI_SUCCESS) {
		_cleanup_freepool_ CHAR16 *s;

		s = PoolPrint(L"%s %d.%02d\n",
			      ST->FirmwareVendor,
			      ST->FirmwareRevision >> 16,
			      ST->FirmwareRevision & 0xffff);
		efivar_set(L"LoaderFirmwareInfo", s, FALSE);
	}

	/* ditto for LoaderFirmwareType */
	if (efivar_get_raw(&loader_guid, L"LoaderFirmwareType", NULL,
			   NULL) != EFI_SUCCESS) {
		_cleanup_freepool_ CHAR16 *s;

		s = PoolPrint(L"UEFI %d.%02d\n", ST->Hdr.Revision >> 16,
			      ST->Hdr.Revision & 0xffff);
		efivar_set(L"LoaderFirmwareType", s, FALSE);
	}

	/* add StubInfo */
	if (efivar_get_raw(&loader_guid,
			   L"StubInfo", NULL, NULL) != EFI_SUCCESS)
		efivar_set(L"StubInfo", L"stubby " GIT_VERSION, FALSE);

	err = linux_exec(image, cmdline, cmdline_len,
			 (UINTN)loaded_image->ImageBase + addrs[1],
			 (UINTN)loaded_image->ImageBase + addrs[2], szs[2]);

	Print(L"Execution of embedded linux image failed: %r\n", err);
	uefi_call_wrapper(BS->Stall, 1, 3 * 1000 * 1000);
	return err;
}
