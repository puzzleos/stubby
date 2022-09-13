#ifndef __STUBBY_STRA_H
#define __STUBBY_STRA_H

#include "stubby_efi.h"

CHAR8 *strstra(const CHAR8 *input, const CHAR8 *find);
BOOLEAN remove_leading_efi_name(CHAR8 *cmdline, UINTN *len_p);

#endif
