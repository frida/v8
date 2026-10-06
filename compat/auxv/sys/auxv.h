#ifndef V8_COMPAT_SYS_AUXV_H_
#define V8_COMPAT_SYS_AUXV_H_

#include <elf.h>
#include <stdio.h>

static inline unsigned long getauxval(unsigned long type) {
  unsigned long result = 0;
  FILE* fp = fopen("/proc/self/auxv", "r");
  if (fp == NULL) return 0;
  struct {
    unsigned long type;
    unsigned long value;
  } entry;
  while (fread(&entry, sizeof(entry), 1, fp) == 1) {
    if (entry.type == type) {
      result = entry.value;
      break;
    }
  }
  fclose(fp);
  return result;
}

#endif  // V8_COMPAT_SYS_AUXV_H_
