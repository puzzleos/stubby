#ifndef __STUBBY_CMDLINE_H
#define __STUBBY_CMDLINE_H

#include <efi.h>

EFI_STATUS check_cmdline(CONST CHAR8 *cmdline, UINTN cmdline_len);
#endif
