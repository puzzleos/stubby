#ifndef __STUBBY_LINUX_EFILIB_H
#define __STUBBY_LINUX_EFILIB_H

#include <efi.h>
#include <wchar.h>
UINTN  Print(IN CONST wchar_t *fmt, ...);
VOID   CopyMem(IN VOID *Dest, IN CONST VOID *Src, IN UINTN len);
VOID * AllocatePool(UINTN Size);
VOID   FreePool (IN VOID *p);
UINTN  strncmpa(IN CONST CHAR8 *s1, IN CONST CHAR8 *s2, IN UINTN len);
UINTN  strlena(IN CONST CHAR8 *s1);
#endif
