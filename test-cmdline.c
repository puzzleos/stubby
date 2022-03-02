#include "kcmdline.h"
#include "linux_efilib.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

EFI_STATUS do_check_cmdline(const char* cmdline) {
   return check_cmdline((unsigned char*)cmdline, strlen(cmdline));
}

typedef struct {
	EFI_STATUS expected;
	char cmdline[128];
} TestData;


TestData tests[] = {
	{EFI_SUCCESS, "console=ttyS0"},
	{EFI_SUCCESS, ""},
	{EFI_SUCCESS, "   "},
	{EFI_SUCCESS, "quiet"},
	{EFI_SUCCESS, " root=atomix console=ttyS0"},
	{EFI_SUCCESS, " root=atomix console=/dev/ttyS0 "},
	{EFI_SUCCESS, "root=atomix console=/dev/ttyS0"},
	{EFI_SECURITY_VIOLATION, "root=atomix init=/bin/bash debug"},
	{EFI_SECURITY_VIOLATION, "init=/bin/bash"},
	{EFI_SECURITY_VIOLATION, "root=\tatomix"},
	// 'quiet' is allowed, but 'quieter' should not be.
	{EFI_SECURITY_VIOLATION, "quieter"},
};


char* efiStatusStr(const EFI_STATUS n) {
	if (n == EFI_SUCCESS)
		return "allow";
	if (n == EFI_SECURITY_VIOLATION)
		return "reject-S";
	if (n == EFI_OUT_OF_RESOURCES)
		return "reject-R";
	wprintf(L"FAIL: unknown number: '%ld'\n", n);
	exit(1);
}

int main()
{
	EFI_STATUS ret;
	TestData td;
	int fails = 0;
	int passes = 0;
	int num = sizeof(tests) / sizeof(tests[0]);
	char nbuf[3];
	wchar_t *fmt = L"%02s | %-4s | %-8s | %-8s |%s|\n";

	wprintf(fmt, "#", "rslt", "expect", "found", "cmdline");
	for (int i=0; i<num; i++) {
		td = tests[i];
		ret = do_check_cmdline(td.cmdline);
		sprintf(nbuf, "%02d", i+1);
		if (ret == td.expected) {
			wprintf(fmt, nbuf, "PASS",
					efiStatusStr(td.expected), efiStatusStr(ret),
					td.cmdline);
			passes++;
		} else {
			wprintf(fmt, nbuf, "FAIL",
					efiStatusStr(td.expected), efiStatusStr(ret),
					td.cmdline);
			fails++;
		}
	}

	wprintf(L"passed: %d. failed: %d\n", passes, fails);
	if (fails != 0) {
		return 1;
	}

	return 0;
}
