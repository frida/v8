// Copyright 2019 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/base/lazy-instance.h"

#include <deque>

#include "src/base/platform/mutex.h"

namespace v8 {
namespace base {

namespace {

Mutex* lazy_mutex{nullptr};
std::deque<LazyRuntime::DestructorFn>* lazy_destructors{nullptr};

}

// static
void LazyRuntime::SetUp() {
  lazy_mutex = new Mutex();
  lazy_destructors = new std::deque<DestructorFn>();
}

// static
void LazyRuntime::TearDown() {
  for (auto& destroy : *lazy_destructors) {
    destroy();
  }

  delete lazy_destructors;
  lazy_destructors = nullptr;
  delete lazy_mutex;
  lazy_mutex = nullptr;
}

// static
void LazyRuntime::RegisterDestructor(DestructorFn destructor) {
  MutexGuard guard(lazy_mutex);
  lazy_destructors->push_front(destructor);
}

}  // namespace base
}  // namespace v8
