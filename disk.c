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

#include "stubby_efi.h"
#include "util.h"

EFI_STATUS disk_get_part_uuid(EFI_HANDLE *handle, CHAR16 uuid[static 37])
{
	EFI_DEVICE_PATH *device_path;

	/* export the device path this image is started from */
	device_path = DevicePathFromHandle(handle);
	if (device_path) {
		_cleanup_freepool_ EFI_DEVICE_PATH *paths = NULL;
		EFI_DEVICE_PATH *path;

		paths = UnpackDevicePath(device_path);
		for (path = paths;
		     !IsDevicePathEnd(path); path = NextDevicePathNode(path)) {
			HARDDRIVE_DEVICE_PATH *drive;

			if (DevicePathType(path) != MEDIA_DEVICE_PATH)
				continue;
			if (DevicePathSubType(path) != MEDIA_HARDDRIVE_DP)
				continue;
			drive = (HARDDRIVE_DEVICE_PATH *)path;
			if (drive->SignatureType != SIGNATURE_TYPE_GUID)
				continue;

			GuidToString(uuid, (EFI_GUID *)&drive->Signature);
			return EFI_SUCCESS;
		}
	}

	return EFI_NOT_FOUND;
}
