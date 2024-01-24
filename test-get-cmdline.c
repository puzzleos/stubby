#include "stubby_efi.h"
#include "kcmdline.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct {
	EFI_STATUS expstatus;
	BOOLEAN secure;
	char *builtin;
	char *runtime;
	char *expected;
} TestData;

#define ResultNotChecked "Result should not be checked"

TestData tests[] = {
	// all good secure
	{EFI_SUCCESS, true,
		"root=atomix STUBBY_RT_CLI1 more",
		"console=ttyS0",
		"root=atomix console=ttyS0 more"},
	// builtin, not runtime, no token
	{EFI_SUCCESS, true,
		"root=atomix console=ttyS0 verbose",
		"",
		"root=atomix console=ttyS0 verbose"},
	// no builtin, use runtime.
	{EFI_SUCCESS, true,
		"",
		"root=atomix verbose",
		"root=atomix verbose"},
	// no builtin no runtime means empty, do not boot
	{EFI_SECURITY_VIOLATION, true,
		"",
		"",
		""},
	// insecure, no builtin, bad runtime token allows runtime
	{EFI_SECURITY_VIOLATION, false,
		"",
		"root=atomix verbose rootkit=yes",
		"root=atomix verbose rootkit=yes"},
	// all good secure marker at beginning
	{EFI_SUCCESS, true,
		"STUBBY_RT_CLI1 root=atomix more",
		"console=ttyS0",
		"console=ttyS0 root=atomix more"},
	// all good secure marker at end
	{EFI_SUCCESS, true,
		"root=atomix more STUBBY_RT_CLI1",
		"console=ttyS0",
		"root=atomix more console=ttyS0"},
	// all good insecure
	{EFI_SUCCESS, false,
		"root=atomix STUBBY_RT_CLI1 more",
		"console=ttyS0",
		"root=atomix console=ttyS0 more"},
	// secure, good builtin but bad runtime token result is not important.
	{EFI_SECURITY_VIOLATION, true,
		"root=atomix STUBBY_RT_CLI1 more",
		"console=ttyS0 more",
		ResultNotChecked},
	// insecure, good builtin but bad runtime token.
	{EFI_SECURITY_VIOLATION, true,
		"root=atomix STUBBY_RT_CLI1 more2",
		"console=ttyS0 more1",
		ResultNotChecked},
	// marker not a full token
	{EFI_INVALID_PARAMETER, false,
		"root=atomix STUBBY_RT_CLI1=abcd more",
		"console=ttyS0",
		ResultNotChecked},
	// no marker in secureboot input
	{EFI_INVALID_PARAMETER, true,
		"root=atomix",
		"console=ttyS0",
		ResultNotChecked},
	// no marker in insecure - just append.
	{EFI_SUCCESS, false,
		"root=atomix",
		"console=ttyS0",
		"root=atomix console=ttyS0"},
	// namespace for marker found twice in builtin secure
	{EFI_INVALID_PARAMETER, true,
		"root=atomix STUBBY_RT debug STUBBY_RT_CLI1 ",
		"console=ttyS0",
		ResultNotChecked},
	// namespace for marker found twice in builtin insecure
	{EFI_INVALID_PARAMETER, false,
		"root=atomix STUBBY_RT debug STUBBY_RT_CLI1 ",
		"console=ttyS0",
		ResultNotChecked},
	// namespace appears in runtime
	{EFI_INVALID_PARAMETER, false,
		"root=atomix debug STUBBY_RT_CLI1",
		"console=ttyS0 STUBBY_RT",
		ResultNotChecked},
};


BOOLEAN do_get_cmdline(int testnum, TestData td) {
	EFI_STATUS status;
	CHAR16 *errmsg;
	CHAR8 *found;
	UINTN found_len;
	CHAR16 status_found[64];
	CHAR16 status_exp[64];

	// Print(L"builtin [%d] = '%a'\nruntime [%d] = '%a'\n", td.builtin, strlen(td.builtin), td.runtime, strlen(td.runtime));
	status = get_cmdline(
		td.secure, 
		(CHAR8 *)td.builtin, strlen(td.builtin),
		(CHAR8 *)td.runtime, strlen(td.runtime),
		&found, &found_len,
		&errmsg);

	StatusToString(status_found, status);
	StatusToString(status_exp, td.expstatus);

	if (status != td.expstatus) {
		Print(L"test %d: expected status '%ls' found '%ls'\n", testnum, status_found, status_exp);
		Print(L"errmsg = %ls\n", errmsg);
		return false;
	}

	// only care to check further for EFI_SUCCESS or EFI_SECURITY_VIOLATION
	if ((status == EFI_SECURITY_VIOLATION && td.secure) ||
			(status != EFI_SUCCESS && status != EFI_SECURITY_VIOLATION)) {
		if (errmsg) {
			free(errmsg);
		}
		return true;
	}

	// Print(L"%d/%d strcmp(%a, found)=%d\n", strlen(td.expected), found_len, td.expected, strcmp(td.expected, found));
	if (strlen(td.expected) != found_len || strcmp(td.expected, (char *)found) != 0) {
		Print(L"expected(%d): %a|\nfound   (%d): %a|\nerrmsg: %ls\n",
				strlen(td.expected), td.expected, found_len, found, errmsg);
		if (errmsg) {
			free(errmsg);
		}
		return false;
	}

	return true;
}

int main()
{
	int num = sizeof(tests) / sizeof(tests[0]);
	int passes = 0, fails = 0;

	for (int i=0; i<num; i++) {
		if (do_get_cmdline(i, tests[i])) {
			passes++;
		} else {
			fails++;
			Print(L"test %d: FAIL\n", i);
		}
	}

	if (fails != 0) {
		return 1;
	}

	Print(L"passed: %d. failed: %d\n", passes, fails);
	return 0;
}
