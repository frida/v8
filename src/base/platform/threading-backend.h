// Copyright 2017 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_BASE_PLATFORM_THREADING_BACKEND_H_
#define V8_BASE_PLATFORM_THREADING_BACKEND_H_

#include "include/v8-platform.h"
#include "src/base/base-export.h"

namespace v8 {
namespace base {

V8_BASE_EXPORT ThreadingBackend* GetThreadingBackend();
V8_BASE_EXPORT void SetThreadingBackend(ThreadingBackend* backend);

}  // namespace base
}  // namespace v8

#endif  // V8_BASE_PLATFORM_THREADING_BACKEND_H_
