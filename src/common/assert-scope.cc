// Copyright 2014 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/common/assert-scope.h"

#include "src/base/bit-field.h"
#include "src/base/lazy-instance.h"
#include "src/base/platform/platform.h"
#include "src/execution/isolate.h"
#include "src/utils/utils.h"

namespace v8 {
namespace internal {

namespace {

template <PerThreadAssertType kType>
using PerThreadDataBit = base::BitField<bool, kType, 1>;

base::LazyInstance<base::ThreadLocalInt>::type current_assert_data =
    LAZY_INSTANCE_INITIALIZER;

uint32_t GetPerThreadAssertData() {
  auto val = current_assert_data.Pointer();
  uint32_t data = val->Get();
  if (data == 0) {
    // Default all asserts to "allow".
    data = ~0;
    val->Set(data);
  }
  return data;
}

void SetPerThreadAssertData(uint32_t data) {
  current_assert_data.Pointer()->Set(data);
}

}  // namespace

template <PerThreadAssertType kType, bool kAllow>
PerThreadAssertScope<kType, kAllow>::PerThreadAssertScope() {
  old_data_ = GetPerThreadAssertData();
  SetPerThreadAssertData(
      PerThreadDataBit<kType>::update(old_data_.value(), kAllow));
}

template <PerThreadAssertType kType, bool kAllow>
PerThreadAssertScope<kType, kAllow>::~PerThreadAssertScope() {
  if (!old_data_.has_value()) return;
  Release();
}

template <PerThreadAssertType kType, bool kAllow>
void PerThreadAssertScope<kType, kAllow>::Release() {
  SetPerThreadAssertData(old_data_.value());
  old_data_.reset();
}

// static
template <PerThreadAssertType kType, bool kAllow>
bool PerThreadAssertScope<kType, kAllow>::IsAllowed() {
  return PerThreadDataBit<kType>::decode(GetPerThreadAssertData());
}

#define PER_ISOLATE_ASSERT_SCOPE_DEFINITION(ScopeType, field, enable)      \
  ScopeType::ScopeType(Isolate* isolate)                                   \
      : isolate_(isolate), old_data_(isolate->field()) {                   \
    DCHECK_NOT_NULL(isolate);                                              \
    isolate_->set_##field(enable);                                         \
  }                                                                        \
                                                                           \
  ScopeType::~ScopeType() { isolate_->set_##field(old_data_); }            \
                                                                           \
  /* static */                                                             \
  bool ScopeType::IsAllowed(Isolate* isolate) { return isolate->field(); } \
                                                                           \
  /* static */                                                             \
  void ScopeType::Open(Isolate* isolate, bool* was_execution_allowed) {    \
    DCHECK_NOT_NULL(isolate);                                              \
    DCHECK_NOT_NULL(was_execution_allowed);                                \
    *was_execution_allowed = isolate->field();                             \
    isolate->set_##field(enable);                                          \
  }                                                                        \
  /* static */                                                             \
  void ScopeType::Close(Isolate* isolate, bool was_execution_allowed) {    \
    DCHECK_NOT_NULL(isolate);                                              \
    isolate->set_##field(was_execution_allowed);                           \
  }

#define PER_ISOLATE_ASSERT_ENABLE_SCOPE_DEFINITION(EnableType, _, field, \
                                                   enable)               \
  PER_ISOLATE_ASSERT_SCOPE_DEFINITION(EnableType, field, enable)

#define PER_ISOLATE_ASSERT_DISABLE_SCOPE_DEFINITION(_, DisableType, field, \
                                                    enable)                \
  PER_ISOLATE_ASSERT_SCOPE_DEFINITION(DisableType, field, enable)

PER_ISOLATE_ASSERT_TYPE(PER_ISOLATE_ASSERT_ENABLE_SCOPE_DEFINITION, true)
PER_ISOLATE_ASSERT_TYPE(PER_ISOLATE_ASSERT_DISABLE_SCOPE_DEFINITION, false)

// -----------------------------------------------------------------------------
// Instantiations.

template class PerThreadAssertScope<HEAP_ALLOCATION_ASSERT, false>;
template class PerThreadAssertScope<HEAP_ALLOCATION_ASSERT, true>;
template class PerThreadAssertScope<SAFEPOINTS_ASSERT, false>;
template class PerThreadAssertScope<SAFEPOINTS_ASSERT, true>;
template class PerThreadAssertScope<HANDLE_ALLOCATION_ASSERT, false>;
template class PerThreadAssertScope<HANDLE_ALLOCATION_ASSERT, true>;
template class PerThreadAssertScope<HANDLE_DEREFERENCE_ASSERT, false>;
template class PerThreadAssertScope<HANDLE_DEREFERENCE_ASSERT, true>;
template class PerThreadAssertScope<CODE_DEPENDENCY_CHANGE_ASSERT, false>;
template class PerThreadAssertScope<CODE_DEPENDENCY_CHANGE_ASSERT, true>;
template class PerThreadAssertScope<CODE_ALLOCATION_ASSERT, false>;
template class PerThreadAssertScope<CODE_ALLOCATION_ASSERT, true>;
template class PerThreadAssertScope<GC_MOLE, false>;

}  // namespace internal
}  // namespace v8
