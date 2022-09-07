#include "stra.h"
#include "stubby_efi.h"

// strstr for ascii string
CHAR8 *strstra(const CHAR8 *input, const CHAR8 *find) {
	const CHAR8 *p, *q;
	do {
		for (p = input, q = find; *q != '\0' && *p == *q; p++, q++) {}
		if (*q == '\0') {
			return (CHAR8*)input;
		}
	} while (*(input++) != '\0');
	return NULL;
}


/* UEFI shell populates LoadOptions and LoadOptionsSize in some odd ways.
 * One of which is that argv[0] (the name of the efi being executed) is
 * present in the LoadOptions.
 *
 * This function simply removes the first token of the command line
 * if that token ends with '.efi' or '.EFI'.
 *
 * parse_load_options in rhboot/shim/load-options.c has a good explanation
 * of this as well as more complete code, but under different license.
 *
 * return FALSE if there is an error.
 */
BOOLEAN remove_leading_efi_name(CHAR8 *cmdline, UINTN *len_p) {
	CHAR8 *p, *space;
	UINTN len = *len_p, offset = 0, newlen = 0, i = 0;

	space = strstra(cmdline, (CHAR8*)" ");
	if (space == NULL)
		return TRUE;

	p = strstra(cmdline, (CHAR8*)".EFI ");
	if (p == NULL) {
		p = strstra(cmdline, (CHAR8*)".efi ");
		if (p == NULL) {
			return TRUE;
		}
	}

	if (p > space)
		return TRUE;

	offset = ((space + 1) - cmdline);
	newlen = len - offset;
	if (newlen < 0) {
		return FALSE;
	}

	for (i = 0; i < newlen; i++) {
		cmdline[i] = cmdline[i + offset];
	}

	cmdline[newlen] = '\0';
	*len_p = newlen;

	return TRUE;
}
