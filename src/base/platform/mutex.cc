// Copyright 2013 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/base/platform/platform.h"
#include "src/base/platform/threading-backend.h"

#include <errno.h>

#include <atomic>

#include "src/base/platform/condition-variable.h"

#if DEBUG
#include <unordered_set>
#endif  // DEBUG

#if V8_OS_WIN
#include <windows.h>
#endif

namespace v8 {
namespace base {

#if DEBUG
namespace {
// Used for asserts to guarantee we are not re-locking a mutex on the same
// thread. If this thread has only one held shared mutex (common case), we use
// {single_held_shared_mutex}. If it has more than one we allocate a set for it.
// Said set has to manually be constructed and destroyed.
LazyInstance<ThreadLocalPointer<SharedMutex>>::type
    single_held_shared_mutex = LAZY_INSTANCE_INITIALIZER;
using TSet = std::unordered_set<base::SharedMutex*>;
LazyInstance<ThreadLocalPointer<TSet>>::type
    held_shared_mutexes = LAZY_INSTANCE_INITIALIZER;

// Returns true iff {shared_mutex} is not a held mutex.
bool SharedMutexNotHeld(SharedMutex* shared_mutex) {
  DCHECK_NOT_NULL(shared_mutex);
  if (single_held_shared_mutex.Pointer()->Get() == shared_mutex) {
    return false;
  }
  auto shared = held_shared_mutexes.Pointer()->Get();
  return (!shared ||
          shared->count(shared_mutex) == 0);
}

// Tries to hold {shared_mutex}. Returns true iff it hadn't been held prior to
// this function call.
bool TryHoldSharedMutex(SharedMutex* shared_mutex) {
  DCHECK_NOT_NULL(shared_mutex);
  auto single_val = single_held_shared_mutex.Pointer();
  auto shared_val = held_shared_mutexes.Pointer();
  auto single_mutex = single_val->Get();
  if (single_mutex) {
    if (shared_mutex == single_mutex) {
      return false;
    }
    DCHECK_NULL(shared_val->Get());
    shared_val->Set(new TSet({single_mutex, shared_mutex}));
    single_val->Set(nullptr);
    return true;
  }
  auto m = shared_val->Get();
  if (m) {
    return m->insert(shared_mutex).second;
  }
  single_val->Set(shared_mutex);
  return true;
}

// Tries to release {shared_mutex}. Returns true iff it had been held prior to
// this function call.
bool TryReleaseSharedMutex(SharedMutex* shared_mutex) {
  DCHECK_NOT_NULL(shared_mutex);
  auto single_val = single_held_shared_mutex.Pointer();
  if (single_val->Get() == shared_mutex) {
    single_val->Set(nullptr);
    return true;
  }
  auto shared_vals = held_shared_mutexes.Pointer()->Get();
  if (shared_vals && shared_vals->erase(shared_mutex)) {
    if (shared_vals->empty()) {
      delete shared_vals;
      held_shared_mutexes.Pointer()->Set(nullptr);
    }
    return true;
  }
  return false;
}
}  // namespace
#endif  // DEBUG


Mutex::Mutex() : impl_(GetThreadingBackend()->CreatePlainMutex()) {
#ifdef DEBUG
  level_ = 0;
#endif
}


Mutex::~Mutex() {
  DCHECK_EQ(0, level_);
}


void Mutex::Lock() {
  impl_->Lock();
  AssertUnheldAndMark();
}


void Mutex::Unlock() {
  AssertHeldAndUnmark();
  impl_->Unlock();
}


bool Mutex::TryLock() {
  if (!impl_->TryLock()) {
    return false;
  }
  AssertUnheldAndMark();
  return true;
}


RecursiveMutex::RecursiveMutex()
  : impl_(GetThreadingBackend()->CreateRecursiveMutex()) {
#ifdef DEBUG
  level_ = 0;
#endif
}


RecursiveMutex::~RecursiveMutex() {
  DCHECK_EQ(0, level_);
}


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
  if (!impl_->TryLock()) {
    return false;
  }
#ifdef DEBUG
  DCHECK_LE(0, level_);
  level_++;
#endif
  return true;
}


SharedMutex::SharedMutex() : impl_(GetThreadingBackend()->CreateSharedMutex()) {
}

SharedMutex::~SharedMutex() {
}

void SharedMutex::LockShared() {
  DCHECK(TryHoldSharedMutex(this));
  impl_->LockShared();
}

void SharedMutex::LockExclusive() {
  DCHECK(TryHoldSharedMutex(this));
  impl_->LockExclusive();
}

void SharedMutex::UnlockShared() {
  DCHECK(TryReleaseSharedMutex(this));
  impl_->UnlockShared();
}

void SharedMutex::UnlockExclusive() {
  DCHECK(TryReleaseSharedMutex(this));
  impl_->UnlockExclusive();
}

bool SharedMutex::TryLockShared() {
  DCHECK(SharedMutexNotHeld(this));
  bool result = impl_->TryLockShared();
  if (result) DCHECK(TryHoldSharedMutex(this));
  return result;
}

bool SharedMutex::TryLockExclusive() {
  DCHECK(SharedMutexNotHeld(this));
  bool result = impl_->TryLockExclusive();
  if (result) DCHECK(TryHoldSharedMutex(this));
  return result;
}


void NativeSharedMutex::LockShared() {
  native_handle_.lock_shared();
}

void NativeSharedMutex::LockExclusive() {
  native_handle_.lock();
}

void NativeSharedMutex::UnlockShared() {
  native_handle_.unlock_shared();
}

void NativeSharedMutex::UnlockExclusive() {
  native_handle_.unlock();
}

bool NativeSharedMutex::TryLockShared() {
  return native_handle_.try_lock_shared();
}

bool NativeSharedMutex::TryLockExclusive() {
  return native_handle_.try_lock();
}


#if V8_OS_POSIX

static V8_INLINE void InitializeNativeHandle(pthread_mutex_t* mutex) {
  int result;
#if defined(DEBUG)
  // Use an error checking mutex in debug mode.
  pthread_mutexattr_t attr;
  result = pthread_mutexattr_init(&attr);
  DCHECK_EQ(0, result);
  result = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
  DCHECK_EQ(0, result);
  result = pthread_mutex_init(mutex, &attr);
  DCHECK_EQ(0, result);
  result = pthread_mutexattr_destroy(&attr);
#else
  // Use a fast mutex (default attributes).
  result = pthread_mutex_init(mutex, nullptr);
#endif  // defined(DEBUG)
  DCHECK_EQ(0, result);
  USE(result);
}


static V8_INLINE void InitializeRecursiveNativeHandle(pthread_mutex_t* mutex) {
  pthread_mutexattr_t attr;
  int result = pthread_mutexattr_init(&attr);
  DCHECK_EQ(0, result);
  result = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  DCHECK_EQ(0, result);
  result = pthread_mutex_init(mutex, &attr);
  DCHECK_EQ(0, result);
  result = pthread_mutexattr_destroy(&attr);
  DCHECK_EQ(0, result);
  USE(result);
}


static V8_INLINE void DestroyNativeHandle(pthread_mutex_t* mutex) {
  int result = pthread_mutex_destroy(mutex);
  DCHECK_EQ(0, result);
  USE(result);
}


static V8_INLINE void LockNativeHandle(pthread_mutex_t* mutex) {
  int result = pthread_mutex_lock(mutex);
  DCHECK_EQ(0, result);
  USE(result);
}


static V8_INLINE void UnlockNativeHandle(pthread_mutex_t* mutex) {
  int result = pthread_mutex_unlock(mutex);
  DCHECK_EQ(0, result);
  USE(result);
}


static V8_INLINE bool TryLockNativeHandle(pthread_mutex_t* mutex) {
  int result = pthread_mutex_trylock(mutex);
  if (result == EBUSY) {
    return false;
  }
  DCHECK_EQ(0, result);
  return true;
}


NativeMutex::NativeMutex() {
  InitializeNativeHandle(&native_handle_);
}


NativeMutex::~NativeMutex() {
  DestroyNativeHandle(&native_handle_);
}


void NativeMutex::Lock() {
  LockNativeHandle(&native_handle_);
}


void NativeMutex::Unlock() {
  UnlockNativeHandle(&native_handle_);
}


bool NativeMutex::TryLock() {
  return TryLockNativeHandle(&native_handle_);
}


NativeRecursiveMutex::NativeRecursiveMutex() {
  InitializeRecursiveNativeHandle(&native_handle_);
}


NativeRecursiveMutex::~NativeRecursiveMutex() {
  DestroyNativeHandle(&native_handle_);
}


void NativeRecursiveMutex::Lock() {
  LockNativeHandle(&native_handle_);
}


void NativeRecursiveMutex::Unlock() {
  UnlockNativeHandle(&native_handle_);
}


bool NativeRecursiveMutex::TryLock() {
  return TryLockNativeHandle(&native_handle_);
}

#elif V8_OS_WIN

NativeMutex::NativeMutex() {
  InitializeCriticalSection(V8ToWindowsType(&native_handle_));
}


NativeMutex::~NativeMutex() {
  DeleteCriticalSection(V8ToWindowsType(&native_handle_));
}


void NativeMutex::Lock() {
  EnterCriticalSection(V8ToWindowsType(&native_handle_));
}


void NativeMutex::Unlock() {
  LeaveCriticalSection(V8ToWindowsType(&native_handle_));
}


bool NativeMutex::TryLock() {
  if (!TryEnterCriticalSection(V8ToWindowsType(&native_handle_))) {
    return false;
  }
  return true;
}


NativeRecursiveMutex::NativeRecursiveMutex() {
  InitializeCriticalSection(V8ToWindowsType(&native_handle_));
}


NativeRecursiveMutex::~NativeRecursiveMutex() {
  DeleteCriticalSection(V8ToWindowsType(&native_handle_));
}


void NativeRecursiveMutex::Lock() {
  EnterCriticalSection(V8ToWindowsType(&native_handle_));
}


void NativeRecursiveMutex::Unlock() {
  LeaveCriticalSection(V8ToWindowsType(&native_handle_));
}


bool NativeRecursiveMutex::TryLock() {
  if (!TryEnterCriticalSection(V8ToWindowsType(&native_handle_))) {
    return false;
  }
  return true;
}

#elif V8_OS_STARBOARD

NativeMutex::NativeMutex() { SbMutexCreate(&native_handle_); }

NativeMutex::~NativeMutex() { SbMutexDestroy(&native_handle_); }

void NativeMutex::Lock() { SbMutexAcquire(&native_handle_); }

void NativeMutex::Unlock() { SbMutexRelease(&native_handle_); }

RecursiveMutex::RecursiveMutex() {}

RecursiveMutex::~RecursiveMutex() {}

void RecursiveMutex::Lock() { native_handle_.Acquire(); }

void RecursiveMutex::Unlock() { native_handle_.Release(); }

bool RecursiveMutex::TryLock() { return native_handle_.AcquireTry(); }

#endif  // V8_OS_STARBOARD

}  // namespace base
}  // namespace v8
