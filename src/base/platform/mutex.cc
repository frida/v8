// Copyright 2013 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/base/platform/mutex.h"

#include "src/base/platform/platform.h"
#include "src/base/platform/threading-backend.h"

namespace v8 {
namespace base {

Mutex::Mutex() : impl_(GetThreadingBackend()->CreatePlainMutex()) {
#ifdef DEBUG
  level_ = 0;
#endif
}

Mutex::~Mutex() { DCHECK_EQ(0, level_); }

void Mutex::Lock() {
  impl_->Lock();
  AssertUnheldAndMark();
}

void Mutex::Unlock() {
  AssertHeldAndUnmark();
  impl_->Unlock();
}

bool Mutex::TryLock() {
  if (!impl_->TryLock()) return false;
  AssertUnheldAndMark();
  return true;
}

RecursiveMutex::RecursiveMutex()
    : impl_(GetThreadingBackend()->CreateRecursiveMutex()) {
#ifdef DEBUG
  level_ = 0;
#endif
}

RecursiveMutex::~RecursiveMutex() { DCHECK_EQ(0, level_); }

void RecursiveMutex::Lock() {
  impl_->Lock();
#ifdef DEBUG
  DCHECK_LE(0, level_);
  level_++;
#endif
}

void RecursiveMutex::Unlock() {
#ifdef DEBUG
  DCHECK_LT(0, level_);
  level_--;
#endif
  impl_->Unlock();
}

bool RecursiveMutex::TryLock() {
  if (!impl_->TryLock()) return false;
#ifdef DEBUG
  DCHECK_LE(0, level_);
  level_++;
#endif
  return true;
}

void NativeMutex::Lock() ABSL_NO_THREAD_SAFETY_ANALYSIS {
  native_handle_.lock();
}

void NativeMutex::Unlock() ABSL_NO_THREAD_SAFETY_ANALYSIS {
  native_handle_.unlock();
}

bool NativeMutex::TryLock() ABSL_NO_THREAD_SAFETY_ANALYSIS {
  return native_handle_.try_lock();
}

NativeRecursiveMutex::~NativeRecursiveMutex() { DCHECK_EQ(0, level_); }

void NativeRecursiveMutex::Lock() {
  int own_id = v8::base::OS::GetCurrentThreadId();
  if (thread_id_ == own_id) {
    level_++;
    return;
  }
  mutex_.Lock();
  DCHECK_EQ(0, level_);
  thread_id_ = own_id;
  level_ = 1;
}

void NativeRecursiveMutex::Unlock() {
#ifdef DEBUG
  int own_id = v8::base::OS::GetCurrentThreadId();
  CHECK_EQ(thread_id_, own_id);
#endif
  if ((--level_) == 0) {
    thread_id_ = 0;
    mutex_.Unlock();
  }
}

bool NativeRecursiveMutex::TryLock() {
  int own_id = v8::base::OS::GetCurrentThreadId();
  if (thread_id_ == own_id) {
    level_++;
    return true;
  }
  if (mutex_.TryLock()) {
    DCHECK_EQ(0, level_);
    thread_id_ = own_id;
    level_ = 1;
    return true;
  }
  return false;
}

}  // namespace base
}  // namespace v8
