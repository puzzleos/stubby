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
	// efilib's 'Print' has '%s' or '%ls' for wide and '%a' for ascii.
	// libc needs '%ls' for wide and '%s' for ascii.
	// callers must use '%a' and '%ls'; we replace '%a' with '%s'.
	if (wcsstr(fmt, L"%s") != NULL) {
		wprintf(L"ERROR: cannot use '%%s' in fmt string: %ls\n", fmt);
		exit(1);
	}

	wchar_t last = L'\0';
	size_t fmtlen = wcslen(fmt);
	wchar_t *fixedfmt = malloc((fmtlen*sizeof(wchar_t))+sizeof(wchar_t));
	if (fixedfmt == NULL) {
		wprintf(L"Failed malloc\n");
		exit(1);
	}

	fixedfmt[fmtlen] = L'\0';
	for (int i=0; i < fmtlen; i++) {
		fixedfmt[i] = fmt[i];
		if (last == L'%' && fmt[i] == L'a') {
			fixedfmt[i] = L's';
		}
		last = fmt[i];
	}

	int x;
	va_list args;
	va_start(args, fmt);
	x = vwprintf(fixedfmt, args);
	va_end(args);

	free(fixedfmt);
	return x;
}

VOID CopyMem(IN VOID *Dest, IN CONST VOID *Src, IN UINTN len) {
	memcpy(Dest, Src, len);
}

VOID * AllocatePool(UINTN Size) {
	return malloc(Size);
}

VOID FreePool (IN VOID *p) {
	return free(p);
}

UINTN strncmpa(IN CONST CHAR8 *s1, IN CONST CHAR8 *s2, IN UINTN len) {
	return strncmp((char*)s1, (char*)s2, len);
}

UINTN strlena(IN CONST CHAR8 *s1) {
	return strlen((char*)s1);
}
#endif

