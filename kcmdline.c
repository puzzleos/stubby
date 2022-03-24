#include "kcmdline.h"
#ifdef LINUX_TEST
#include "linux_efilib.h"
#else
#include <efilib.h>
#endif

// If a provided command line has more tokens (words) than MAX_TOKENS
// then an error will be returned.
#define MAX_TOKENS 128

// These are the tokens that are allowed to be passed on EFI cmdline.
static const CHAR8 allowed[][32] = {
	"^console=",
	"^root=oci:",
	"root=atomix",
	"ro",
	"quiet",
	"verbose",
	"crashkernel=256M",
};


BOOLEAN is_allowed(const CHAR8 *input) {
	int len = 0;
	int input_len = strlena(input);
	const CHAR8 * token;
	int num_allowed = sizeof(allowed) / sizeof(allowed[0]);
	for (int i=0; i<num_allowed; i++) {
		token = allowed[i];
		len = strlena(token);
		if (token[0] == '^') {
			len = len - 1;
			token = &token[1];
		} else if (input_len != len) {
			continue;
		}
		if (strncmpa(token, input, len) == 0) {
			return TRUE;
		}
	}
	return FALSE;
}

// check cmdline to make sure it contains only allowed words.
// return EFI_SUCCESS on safe, EFI_SECURITY_VIOLATION on unsafe;
EFI_STATUS check_cmdline(CONST CHAR8 *cmdline, UINTN cmdline_len) {
	CHAR8 c = '\0';
	CHAR8 *buf = NULL;
	CHAR8 *tokens[MAX_TOKENS];
	EFI_STATUS status = EFI_SUCCESS;
	int i;
	int start = -1;
	int num_toks = 0;
	buf = AllocatePool(cmdline_len + 1);
	if (!buf) {
		return EFI_OUT_OF_RESOURCES;
	}

	CopyMem(buf, cmdline, cmdline_len);
	// make sure it is null terminated.
	buf[cmdline_len] = '\0';

	// walk buf, populating tokens
	for (i = 0; i < cmdline_len; i++) {
		c = buf[i];
		if (c < 0x20 || c > 0x7e) {
			Print(L"Bad character 0x%02hhx.\n", buf);
			status = EFI_SECURITY_VIOLATION;
			goto out;
		}
		if (i >= MAX_TOKENS) {
			Print(L"Too many tokens in cmdline.\n");
			status = EFI_SECURITY_VIOLATION;
			goto out;
		}

		if (c == ' ') {
			// end of a token
			buf[i] = '\0';
			if (start >= 0) {
				tokens[num_toks] = &buf[start];
				start = -1;
				num_toks++;
			}
		} else {
			if (start < 0) {
				start = i;
			}
		}
	}
	if (start >= 0) {
		tokens[num_toks] = &buf[start];
		num_toks++;
	}

	for (i=0; i < num_toks; i++) {
		if (!is_allowed(tokens[i])) {
			Print(L"token not allowed: %s\n", tokens[i]);
			return EFI_SECURITY_VIOLATION;
		}
	}

out:

	FreePool(buf);
	return status;
}
