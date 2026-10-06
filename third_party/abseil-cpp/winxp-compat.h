#ifndef ABSL_WINXP_COMPAT_H
#define ABSL_WINXP_COMPAT_H

#include <windows.h>

#if _WIN32_WINNT < 0x0502
extern "C" {
NTSYSAPI WORD NTAPI RtlCaptureStackBackTrace(DWORD FramesToSkip,
    DWORD FramesToCapture, PVOID* BackTrace, PDWORD BackTraceHash);
}
#endif

#endif
