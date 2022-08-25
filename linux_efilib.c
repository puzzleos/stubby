/* This supplies "linux" implementations of the EFI functions that
 * stubby uses. It allows us to write tests (test-*.c) and run them in linux.
 */
#include "linux_efilib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <wchar.h>

wchar_t* _fixup_fmt(const wchar_t *fmt) {
	// Efilib's Print and libc wprintf differ on the format specifiers
	// for wchar_t (wide) and char (ascii) strings:
	//   lib-name | wchar spec  | char spec |
	//   efilib   | %ls, %s     | %a        |
	//   libc     | %ls         | %s        |
	//
	// To simplify things:
	//  * restrict callers to only '%ls' or '%a'
	//  * change %a (ascii) to %s
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

	return fixedfmt;
}

UINTN Print(IN CONST CHAR16 *fmt, ...) {
	wchar_t *fixedfmt = _fixup_fmt(fmt);

	int x;
	va_list args;
	va_start(args, fmt);
	x = vwprintf(fixedfmt, args);
	va_end(args);

	free(fixedfmt);
	return x;
}

UINTN UnicodeSPrint(OUT CHAR16 *Str, IN UINTN StrSize, IN CONST CHAR16 *fmt, ...) {
	wchar_t *fixedfmt = _fixup_fmt(fmt);

	int x;
	va_list args;
	va_start(args, fmt);
	x = vswprintf(Str, StrSize, fixedfmt, args);
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

// from efilib/lib/error.c
struct {
    EFI_STATUS      Code;
    WCHAR	    *Desc;
} ErrorCodeTable[] = {
	{  EFI_SUCCESS,                L"Success"},
	{  EFI_LOAD_ERROR,             L"Load Error"},
	{  EFI_INVALID_PARAMETER,      L"Invalid Parameter"},
	{  EFI_UNSUPPORTED,            L"Unsupported"},
	{  EFI_BAD_BUFFER_SIZE,        L"Bad Buffer Size"},
	{  EFI_BUFFER_TOO_SMALL,       L"Buffer Too Small"},
	{  EFI_NOT_READY,              L"Not Ready"},
	{  EFI_DEVICE_ERROR,           L"Device Error"},
	{  EFI_WRITE_PROTECTED,        L"Write Protected"},
	{  EFI_OUT_OF_RESOURCES,       L"Out of Resources"},
	{  EFI_VOLUME_CORRUPTED,       L"Volume Corrupt"},
	{  EFI_VOLUME_FULL,            L"Volume Full"},
	{  EFI_NO_MEDIA,               L"No Media"},
	{  EFI_MEDIA_CHANGED,          L"Media changed"},
	{  EFI_NOT_FOUND,              L"Not Found"},
	{  EFI_ACCESS_DENIED,          L"Access Denied"},
	{  EFI_NO_RESPONSE,            L"No Response"},
	{  EFI_NO_MAPPING,             L"No mapping"},
	{  EFI_TIMEOUT,                L"Time out"},
	{  EFI_NOT_STARTED,            L"Not started"},
	{  EFI_ALREADY_STARTED,        L"Already started"},
	{  EFI_ABORTED,                L"Aborted"},
	{  EFI_ICMP_ERROR,             L"ICMP Error"},
	{  EFI_TFTP_ERROR,             L"TFTP Error"},
	{  EFI_PROTOCOL_ERROR,         L"Protocol Error"},
	{  EFI_INCOMPATIBLE_VERSION,   L"Incompatible Version"},
	{  EFI_SECURITY_VIOLATION,     L"Security Policy Violation"},
	{  EFI_CRC_ERROR,              L"CRC Error"},
	{  EFI_END_OF_MEDIA,           L"End of Media"},
	{  EFI_END_OF_FILE,            L"End of File"},
	{  EFI_INVALID_LANGUAGE,       L"Invalid Languages"},
	{  EFI_COMPROMISED_DATA,       L"Compromised Data"},

	// warnings
	{  EFI_WARN_UNKNOWN_GLYPH,     L"Warning Unknown Glyph"},
	{  EFI_WARN_DELETE_FAILURE,    L"Warning Delete Failure"},
	{  EFI_WARN_WRITE_FAILURE,     L"Warning Write Failure"},
	{  EFI_WARN_BUFFER_TOO_SMALL,  L"Warning Buffer Too Small"},
	{  0, NULL}
} ;

VOID StatusToString(OUT CHAR16 *Buffer, IN EFI_STATUS Status) {
    UINTN           Index;

    for (Index = 0; ErrorCodeTable[Index].Desc; Index +=1) {
        if (ErrorCodeTable[Index].Code == Status) {
	    wcscpy (Buffer, ErrorCodeTable[Index].Desc);
            return;
        }
    }

    UnicodeSPrint (Buffer, 0, L"%X", Status);
}
