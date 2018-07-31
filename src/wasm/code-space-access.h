// Copyright 2020 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_WASM_CODE_SPACE_ACCESS_H_
#define V8_WASM_CODE_SPACE_ACCESS_H_

#include "src/base/build_config.h"
#include "src/base/macros.h"
#include "src/common/globals.h"

namespace v8 {
namespace internal {

#if defined(V8_OS_MACOSX) && defined(V8_HOST_ARCH_ARM64)

// Ignoring this warning is considered better than relying on
// __builtin_available.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunguarded-availability-new"
inline void SwitchMemoryPermissionsToWritable() {
  pthread_jit_write_protect_np(0);
}
inline void SwitchMemoryPermissionsToExecutable() {
  pthread_jit_write_protect_np(1);
}
#pragma clang diagnostic pop

namespace wasm {

class CodeSpaceWriteScope {
 public:
  // TODO(jkummerow): Background threads could permanently stay in
  // writable mode; only the main thread has to switch back and forth.
  CodeSpaceWriteScope() {
    auto level_storage = code_space_write_nesting_level_.Pointer();
    int level = level_storage->Get();
    if (level == 0) {
      SwitchMemoryPermissionsToWritable();
    }
    level_storage->Set(level + 1);
  }
  ~CodeSpaceWriteScope() {
    auto level_storage = code_space_write_nesting_level_.Pointer();
    int level = level_storage->Get() - 1;
    if (level == 0) {
      SwitchMemoryPermissionsToExecutable();
    }
    level_storage->Set(level);
  }

 private:
  static base::LazyInstance<base::ThreadLocalInt>::type
      code_space_write_nesting_level_;
};

#define CODE_SPACE_WRITE_SCOPE CodeSpaceWriteScope _write_access_;

}  // namespace wasm

#else  // Not Mac-on-arm64.

// Nothing to do, we map code memory with rwx permissions.
inline void SwitchMemoryPermissionsToWritable() {}
inline void SwitchMemoryPermissionsToExecutable() {}

#define CODE_SPACE_WRITE_SCOPE

#endif  // V8_OS_MACOSX && V8_HOST_ARCH_ARM64

}  // namespace internal
}  // namespace v8

#endif  // V8_WASM_CODE_SPACE_ACCESS_H_
