#include "efi.h"

#ifndef LINUX_TEST
#include "efilib.h"
#else
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <wchar.h>

UINTN Print(IN CONST wchar_t *fmt, ...) {
	int x;
	va_list args;
	va_start(args, fmt);
	x = vwprintf(fmt, args);
	va_end(args);
	wprintf(L"\n");
	return x;
}

VOID CopyMem(IN VOID *Dest, IN CONST VOID *Src, IN UINTN len) {
	memcpy(Dest, Src, len);
}

VOID * AllocatePool(UINTN Size) {
	return malloc(Size);
}

UINTN strncmpa(IN CONST CHAR8 *s1, IN CONST CHAR8 *s2, IN UINTN len) {
	return strncmp((char*)s1, (char*)s2, len);
}

UINTN strlena(IN CONST CHAR8 *s1) {
	return strlen((char*)s1);
}
#endif

