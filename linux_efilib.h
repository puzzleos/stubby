/* This supplies the necessary defines for stubby's use of efi. */
#ifndef __STUBBY_LINUX_EFILIB_H
#define __STUBBY_LINUX_EFILIB_H

#include <stdint.h>
#include <wchar.h>

typedef uint64_t   UINT64;
typedef uint32_t   UINT32;
typedef uint16_t   UINT16;
typedef uint8_t    UINT8;
typedef uint64_t   UINTN;
typedef int64_t    INTN;

// This is the hack if you use this code, then:
//   sizeof(CHAR16) == 4
//
// It allows using the existing code that uses CHAR16
// with wprintf and friends.
//
// For test programs we need a implementations of Print
// and UnicodeSprint, but don't want to write them.  We use
// wprintf and friends.  The problem with that is libc expects a
// wchar_t of 4 bytes, but EFI applications compile with
// -fshort-wchar which means sizeof(Lthat sizeof(L"foo") == 2.
typedef wchar_t CHAR16;
typedef wchar_t WCHAR;

typedef UINT8	CHAR8;
typedef UINT8           BOOLEAN;
typedef UINTN           EFI_STATUS;

// this is specific based on what EFI_STATUS is.
#define EFIERR(a)           (0x8000000000000000 | a)
#ifndef IN
    #define IN
    #define OUT
    #define OPTIONAL
#endif

#define CONST const
#define VOID void
#define TRUE    ((BOOLEAN) 1)
#define FALSE   ((BOOLEAN) 0)

// efierr.h
#define EFIWARN(a)                            (a)
#define EFI_ERROR(a)              (((INTN) a) < 0)

#define EFI_SUCCESS                             0
#define EFI_LOAD_ERROR                  EFIERR(1)
#define EFI_INVALID_PARAMETER           EFIERR(2)
#define EFI_UNSUPPORTED                 EFIERR(3)
#define EFI_BAD_BUFFER_SIZE             EFIERR(4)
#define EFI_BUFFER_TOO_SMALL            EFIERR(5)
#define EFI_NOT_READY                   EFIERR(6)
#define EFI_DEVICE_ERROR                EFIERR(7)
#define EFI_WRITE_PROTECTED             EFIERR(8)
#define EFI_OUT_OF_RESOURCES            EFIERR(9)
#define EFI_VOLUME_CORRUPTED            EFIERR(10)
#define EFI_VOLUME_FULL                 EFIERR(11)
#define EFI_NO_MEDIA                    EFIERR(12)
#define EFI_MEDIA_CHANGED               EFIERR(13)
#define EFI_NOT_FOUND                   EFIERR(14)
#define EFI_ACCESS_DENIED               EFIERR(15)
#define EFI_NO_RESPONSE                 EFIERR(16)
#define EFI_NO_MAPPING                  EFIERR(17)
#define EFI_TIMEOUT                     EFIERR(18)
#define EFI_NOT_STARTED                 EFIERR(19)
#define EFI_ALREADY_STARTED             EFIERR(20)
#define EFI_ABORTED                     EFIERR(21)
#define EFI_ICMP_ERROR                  EFIERR(22)
#define EFI_TFTP_ERROR                  EFIERR(23)
#define EFI_PROTOCOL_ERROR              EFIERR(24)
#define EFI_INCOMPATIBLE_VERSION        EFIERR(25)
#define EFI_SECURITY_VIOLATION          EFIERR(26)
#define EFI_CRC_ERROR                   EFIERR(27)
#define EFI_END_OF_MEDIA                EFIERR(28)
#define EFI_END_OF_FILE                 EFIERR(31)
#define EFI_INVALID_LANGUAGE            EFIERR(32)
#define EFI_COMPROMISED_DATA            EFIERR(33)

#define EFI_WARN_UNKOWN_GLYPH           EFIWARN(1)
#define EFI_WARN_UNKNOWN_GLYPH          EFIWARN(1)
#define EFI_WARN_DELETE_FAILURE         EFIWARN(2)
#define EFI_WARN_WRITE_FAILURE          EFIWARN(3)
#define EFI_WARN_BUFFER_TOO_SMALL       EFIWARN(4)


UINTN  Print(IN CONST CHAR16 *fmt, ...);
UINTN  UnicodeSPrint(OUT CHAR16 *Str, IN UINTN StrSize, IN CONST CHAR16 *fmt, ...);
VOID   CopyMem(IN VOID *Dest, IN CONST VOID *Src, IN UINTN len);
VOID * AllocatePool(UINTN Size);
VOID   FreePool (IN VOID *p);
UINTN  strncmpa(IN CONST CHAR8 *s1, IN CONST CHAR8 *s2, IN UINTN len);
UINTN  strlena(IN CONST CHAR8 *s1);

VOID   StatusToString(OUT CHAR16 *Buffer, IN EFI_STATUS Status);
#endif
