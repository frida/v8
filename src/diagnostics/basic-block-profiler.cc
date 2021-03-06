// Copyright 2014 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/diagnostics/basic-block-profiler.h"

#include <algorithm>
#include <numeric>
#include <sstream>

#include "src/base/lazy-instance.h"
#include "src/heap/heap-inl.h"
#include "src/objects/shared-function-info-inl.h"

namespace v8 {
namespace internal {

namespace {
base::LazyInstance<BasicBlockProfiler>::type profiler_instance =
    LAZY_INSTANCE_INITIALIZER;
}

BasicBlockProfilerData::BasicBlockProfilerData(size_t n_blocks)
    : block_ids_(n_blocks), counts_(n_blocks, 0) {}

void BasicBlockProfilerData::SetCode(const std::ostringstream& os) {
  code_ = os.str();
}

void BasicBlockProfilerData::SetFunctionName(std::unique_ptr<char[]> name) {
  function_name_ = name.get();
}

void BasicBlockProfilerData::SetSchedule(const std::ostringstream& os) {
  schedule_ = os.str();
}

void BasicBlockProfilerData::SetBlockId(size_t offset, int32_t id) {
  DCHECK(offset < n_blocks());
  block_ids_[offset] = id;
}

void BasicBlockProfilerData::SetHash(int hash) { hash_ = hash; }

void BasicBlockProfilerData::ResetCounts() {
  for (size_t i = 0; i < n_blocks(); ++i) {
    counts_[i] = 0;
  }
}

BasicBlockProfilerData* BasicBlockProfiler::NewData(size_t n_blocks) {
  base::MutexGuard lock(&data_list_mutex_);
  auto data = std::make_unique<BasicBlockProfilerData>(n_blocks);
  BasicBlockProfilerData* data_ptr = data.get();
  data_list_.push_back(std::move(data));
  return data_ptr;
}

namespace {
Handle<String> CopyStringToJSHeap(const std::string& source, Isolate* isolate) {
  return isolate->factory()->NewStringFromAsciiChecked(source.c_str(),
                                                       AllocationType::kOld);
}

// Size of entries in both block_ids and counts.
constexpr int kBasicBlockSlotSize = kInt32Size;
}  // namespace

BasicBlockProfilerData::BasicBlockProfilerData(
    Handle<OnHeapBasicBlockProfilerData> js_heap_data, Isolate* isolate) {
  function_name_ = js_heap_data->name().ToCString().get();
  schedule_ = js_heap_data->schedule().ToCString().get();
  code_ = js_heap_data->code().ToCString().get();
  Handle<ByteArray> counts(js_heap_data->counts(), isolate);
  for (int i = 0; i < counts->length() / kBasicBlockSlotSize; ++i) {
    counts_.push_back(counts->get_uint32(i));
  }
  Handle<ByteArray> block_ids(js_heap_data->block_ids(), isolate);
  for (int i = 0; i < block_ids->length() / kBasicBlockSlotSize; ++i) {
    block_ids_.push_back(block_ids->get_int(i));
  }
  CHECK_EQ(block_ids_.size(), counts_.size());
  hash_ = js_heap_data->hash();
}

BasicBlockProfilerData::BasicBlockProfilerData(
    OnHeapBasicBlockProfilerData js_heap_data) {
  function_name_ = js_heap_data.name().ToCString().get();
  schedule_ = js_heap_data.schedule().ToCString().get();
  code_ = js_heap_data.code().ToCString().get();
  ByteArray counts(js_heap_data.counts());
  for (int i = 0; i < counts.length() / kBasicBlockSlotSize; ++i) {
    counts_.push_back(counts.get_uint32(i));
  }
  ByteArray block_ids(js_heap_data.block_ids());
  for (int i = 0; i < block_ids.length() / kBasicBlockSlotSize; ++i) {
    block_ids_.push_back(block_ids.get_int(i));
  }
  CHECK_EQ(block_ids_.size(), counts_.size());
}

Handle<OnHeapBasicBlockProfilerData> BasicBlockProfilerData::CopyToJSHeap(
    Isolate* isolate) {
  int array_size_in_bytes = static_cast<int>(n_blocks() * kBasicBlockSlotSize);
  CHECK(array_size_in_bytes >= 0 &&
        static_cast<size_t>(array_size_in_bytes) / kBasicBlockSlotSize ==
            n_blocks());  // Overflow
  Handle<ByteArray> block_ids = isolate->factory()->NewByteArray(
      array_size_in_bytes, AllocationType::kOld);
  for (int i = 0; i < static_cast<int>(n_blocks()); ++i) {
    block_ids->set_int(i, block_ids_[i]);
  }
  Handle<ByteArray> counts = isolate->factory()->NewByteArray(
      array_size_in_bytes, AllocationType::kOld);
  for (int i = 0; i < static_cast<int>(n_blocks()); ++i) {
    counts->set_uint32(i, counts_[i]);
  }
  Handle<String> name = CopyStringToJSHeap(function_name_, isolate);
  Handle<String> schedule = CopyStringToJSHeap(schedule_, isolate);
  Handle<String> code = CopyStringToJSHeap(code_, isolate);

  return isolate->factory()->NewOnHeapBasicBlockProfilerData(
      block_ids, counts, name, schedule, code, hash_, AllocationType::kOld);
}

BasicBlockProfiler* BasicBlockProfiler::Get() {
  return profiler_instance.Pointer();
}

void BasicBlockProfiler::ResetCounts(Isolate* isolate) {
  for (const auto& data : data_list_) {
    data->ResetCounts();
  }
  HandleScope scope(isolate);
  Handle<ArrayList> list(isolate->heap()->basic_block_profiling_data(),
                         isolate);
  for (int i = 0; i < list->Length(); ++i) {
    Handle<ByteArray> counts(
        OnHeapBasicBlockProfilerData::cast(list->Get(i)).counts(), isolate);
    for (int j = 0; j < counts->length() / kBasicBlockSlotSize; ++j) {
      counts->set_uint32(j, 0);
    }
  }
}

bool BasicBlockProfiler::HasData(Isolate* isolate) {
  return data_list_.size() > 0 ||
         isolate->heap()->basic_block_profiling_data().Length() > 0;
}

void BasicBlockProfiler::Print(std::ostream& os, Isolate* isolate) {
  os << "---- Start Profiling Data ----" << std::endl;
  for (const auto& data : data_list_) {
    os << *data;
  }
  HandleScope scope(isolate);
  Handle<ArrayList> list(isolate->heap()->basic_block_profiling_data(),
                         isolate);
  std::unordered_set<std::string> builtin_names;
  for (int i = 0; i < list->Length(); ++i) {
    BasicBlockProfilerData data(
        handle(OnHeapBasicBlockProfilerData::cast(list->Get(i)), isolate),
        isolate);
    // Print data for builtins to both stdout and the log file, if logging is
    // enabled.
    os << data;
    data.Log(isolate);
    // Ensure that all builtin names are unique; otherwise profile-guided
    // optimization might get confused.
    CHECK(builtin_names.insert(data.function_name_).second);
  }
  os << "---- End Profiling Data ----" << std::endl;
}

std::vector<bool> BasicBlockProfiler::GetCoverageBitmap(Isolate* isolate) {
  DisallowHeapAllocation no_gc;
  ArrayList list(isolate->heap()->basic_block_profiling_data());
  std::vector<bool> out;
  int list_length = list.Length();
  for (int i = 0; i < list_length; ++i) {
    BasicBlockProfilerData data(
        OnHeapBasicBlockProfilerData::cast(list.Get(i)));
    for (size_t i = 0; i < data.n_blocks(); ++i) {
      out.push_back(data.counts_[i] > 0);
    }
  }
  return out;
}

void BasicBlockProfilerData::Log(Isolate* isolate) {
  bool any_nonzero_counter = false;
  for (size_t i = 0; i < n_blocks(); ++i) {
    if (counts_[i] > 0) {
      any_nonzero_counter = true;
      isolate->logger()->BasicBlockCounterEvent(function_name_.c_str(),
                                                block_ids_[i], counts_[i]);
    }
  }
  if (any_nonzero_counter) {
    isolate->logger()->BuiltinHashEvent(function_name_.c_str(), hash_);
  }
}

std::ostream& operator<<(std::ostream& os, const BasicBlockProfilerData& d) {
  int block_count_sum = std::accumulate(d.counts_.begin(), d.counts_.end(), 0);
  if (block_count_sum == 0) return os;
  const char* name = "unknown function";
  if (!d.function_name_.empty()) {
    name = d.function_name_.c_str();
  }
  if (!d.schedule_.empty()) {
    os << "schedule for " << name << " (B0 entered " << d.counts_[0]
       << " times)" << std::endl;
    os << d.schedule_.c_str() << std::endl;
  }
  os << "block counts for " << name << ":" << std::endl;
  std::vector<std::pair<size_t, uint32_t>> pairs;
  pairs.reserve(d.n_blocks());
  for (size_t i = 0; i < d.n_blocks(); ++i) {
    pairs.push_back(std::make_pair(i, d.counts_[i]));
  }
  std::sort(
      pairs.begin(), pairs.end(),
      [=](std::pair<size_t, uint32_t> left, std::pair<size_t, uint32_t> right) {
        if (right.second == left.second) return left.first < right.first;
        return right.second < left.second;
      });
  for (auto it : pairs) {
    if (it.second == 0) break;
    os << "block B" << it.first << " : " << it.second << std::endl;
  }
  os << std::endl;
  if (!d.code_.empty()) {
    os << d.code_.c_str() << std::endl;
  }
  return os;
}

}  // namespace internal
}  // namespace v8
