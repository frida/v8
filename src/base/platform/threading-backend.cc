// Copyright 2017 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/base/platform/threading-backend.h"

#include "src/base/platform/condition-variable.h"
#include "src/base/platform/mutex.h"

namespace v8 {
namespace base {

class DefaultThreadingBackend final : public ThreadingBackend {
 public:
  MutexImpl* CreatePlainMutex() override {
    return new NativeMutex();
  }

  MutexImpl* CreateRecursiveMutex() override {
    return new NativeRecursiveMutex();
  }

  SharedMutexImpl* CreateSharedMutex() override {
    return new NativeSharedMutex();
  }

  ConditionVariableImpl* CreateConditionVariable() override {
    return new NativeConditionVariable();
  }
};

static DefaultThreadingBackend default_threading_backend_;
static ThreadingBackend* threading_backend_ = &default_threading_backend_;


ThreadingBackend* GetThreadingBackend() {
  return threading_backend_;
}

void SetThreadingBackend(ThreadingBackend* backend) {
  threading_backend_ = (backend != nullptr) ? backend : &default_threading_backend_;
}

}  // namespace base
}  // namespace v8
