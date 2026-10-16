#ifndef ABSL_APPLE_COMPAT_CFTIMEZONE_H
#define ABSL_APPLE_COMPAT_CFTIMEZONE_H

#include <stddef.h>

typedef const struct OpaqueCFString* CFStringRef;
typedef const struct OpaqueCFTimeZone* CFTimeZoneRef;
typedef const void* CFTypeRef;
typedef long CFIndex;
typedef unsigned int CFStringEncoding;

#define kCFStringEncodingUTF8 0x08000100

static inline CFTimeZoneRef CFTimeZoneCopyDefault(void) { return NULL; }
static inline CFStringRef CFTimeZoneGetName(CFTimeZoneRef tz) { return NULL; }
static inline CFIndex CFStringGetLength(CFStringRef s) { return 0; }
static inline CFIndex CFStringGetMaximumSizeForEncoding(CFIndex length, CFStringEncoding encoding) { return 0; }
static inline int CFStringGetCString(CFStringRef s, char* buffer, CFIndex size, CFStringEncoding encoding) { return 0; }
static inline void CFRelease(CFTypeRef cf) {}

#endif
