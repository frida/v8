// Copyright 2013 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/base/platform/condition-variable.h"

#include "absl/time/time.h"
#include "src/base/platform/threading-backend.h"
#include "src/base/platform/time.h"

namespace v8 {
namespace base {

ConditionVariable::ConditionVariable()
    : impl_(GetThreadingBackend()->CreateConditionVariable()) {}

ConditionVariable::~ConditionVariable() = default;

void ConditionVariable::NotifyOne() { impl_->NotifyOne(); }

void ConditionVariable::NotifyAll() { impl_->NotifyAll(); }

void ConditionVariable::Wait(Mutex* mutex) {
  mutex->AssertHeldAndUnmark();
  impl_->Wait(mutex->impl_.get());
  mutex->AssertUnheldAndMark();
}

bool ConditionVariable::WaitFor(Mutex* mutex, const TimeDelta& rel_time) {
  mutex->AssertHeldAndUnmark();
  bool result = impl_->WaitFor(mutex->impl_.get(), rel_time.InMicroseconds());
  mutex->AssertUnheldAndMark();
  return result;
}

void NativeConditionVariable::NotifyOne() { native_handle_.Signal(); }

void NativeConditionVariable::NotifyAll() { native_handle_.SignalAll(); }

void NativeConditionVariable::Wait(MutexImpl* mutex) {
  native_handle_.Wait(&static_cast<NativeMutex*>(mutex)->native_handle_);
}

bool NativeConditionVariable::WaitFor(MutexImpl* mutex,
                                      int64_t delta_in_microseconds) {
  bool timed_out = native_handle_.WaitWithTimeout(
      &static_cast<NativeMutex*>(mutex)->native_handle_,
      absl::Microseconds(delta_in_microseconds));
  return !timed_out;
}

}  // namespace base
}  // namespace v8
