// Copyright 2026 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_THIRD_PARTY_LIBUNICODE_V8_LIBUNICODE_H_
#define V8_THIRD_PARTY_LIBUNICODE_V8_LIBUNICODE_H_

#include "third_party/libunicode/v8-libunicode-prefix.h"

// libunicode.h expects cutils.h to have defined TRUE, but cutils.h defines too
// many generic macros to include here.
#ifndef TRUE
#define TRUE 1
#define V8_LIBUNICODE_UNDEF_TRUE
#endif

extern "C" {
#include "third_party/libunicode/libunicode.h"
}

#ifdef V8_LIBUNICODE_UNDEF_TRUE
#undef TRUE
#undef V8_LIBUNICODE_UNDEF_TRUE
#endif

#endif  // V8_THIRD_PARTY_LIBUNICODE_V8_LIBUNICODE_H_
