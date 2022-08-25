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
