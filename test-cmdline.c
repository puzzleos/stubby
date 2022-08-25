#include "kcmdline.h"
#include "linux_efilib.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

EFI_STATUS do_check_cmdline(const char* cmdline) {
	CHAR16 emsg[256];
	UINTN emsg_len = sizeof(emsg) / sizeof(emsg[0]);
	return check_cmdline((unsigned char*)cmdline, strlen(cmdline), emsg, emsg_len);
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


CHAR16* efiStatusStr(const EFI_STATUS n) {
	if (n == EFI_SUCCESS)
		return L"allow";
	if (n == EFI_SECURITY_VIOLATION)
		return L"reject-S";
	if (n == EFI_OUT_OF_RESOURCES)
		return L"reject-R";
	Print(L"FAIL: unknown number: '%ld'\n", n);
	exit(1);
}

int main()
{
	EFI_STATUS ret;
	TestData td;
	int fails = 0;
	int passes = 0;
	int num = sizeof(tests) / sizeof(tests[0]);
	CHAR16 nbuf[3];
	CHAR16 *fmt = L"%02ls | %-4ls | %-8ls | %-8ls |%a|\n";

	Print(fmt, L"#", L"rslt", L"expect", L"found", "cmdline");
	for (int i=0; i<num; i++) {
		td = tests[i];
		ret = do_check_cmdline(td.cmdline);
		UnicodeSPrint(nbuf, 3, L"%02d", i+1);
		if (ret == td.expected) {
			Print(fmt, nbuf, L"PASS",
					efiStatusStr(td.expected), efiStatusStr(ret),
					td.cmdline);
			passes++;
		} else {
			wprintf(fmt, nbuf, L"FAIL",
					efiStatusStr(td.expected), efiStatusStr(ret),
					td.cmdline);
			fails++;
		}
	}

	Print(L"passed: %d. failed: %d\n", passes, fails);
	if (fails != 0) {
		return 1;
	}

	return 0;
}
