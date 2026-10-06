#ifndef ABSL_LINUX_COMPAT_H
#define ABSL_LINUX_COMPAT_H

#include <fcntl.h>
#include <sched.h>
#include <linux/futex.h>

#ifndef FUTEX_PRIVATE_FLAG
#define FUTEX_PRIVATE_FLAG 128
#endif

#ifndef O_CLOEXEC
#define O_CLOEXEC 02000000
#endif

#ifndef F_DUPFD_CLOEXEC
#define F_DUPFD_CLOEXEC 1030
#endif

#ifdef __GLIBC__
#if !__GLIBC_PREREQ(2, 6)
static inline int sched_getcpu(void) { return -1; }
#endif
#endif

#endif
