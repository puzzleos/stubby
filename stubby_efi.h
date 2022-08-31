#ifndef __STUBBY_EFI_H
#define __STUBBY_EFI_H
#ifdef LINUX_TEST
#include "linux_efilib.h"
#else
#include <efi.h>
#include <efilib.h>
// older gnu-efi do not have UnicodeSPrint (Ubuntu 20.04 version 3.0.9-1)
#ifndef UnicodeSPrint
#define UnicodeSPrint(fmt, ...) SPrint(fmt, ##__VA_ARGS__)
#endif
#endif
#endif
