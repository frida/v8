// Copyright 2020 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_SANDBOX_EXTERNAL_POINTER_TABLE_H_
#define V8_SANDBOX_EXTERNAL_POINTER_TABLE_H_

#include "include/v8config.h"
#include "src/base/atomicops.h"
#include "src/base/memory.h"
#include "src/base/platform/mutex.h"
#include "src/common/globals.h"

#ifdef V8_COMPRESS_POINTERS

namespace v8 {
namespace internal {

class Isolate;

/**
 * A table storing pointers to objects outside the V8 heap.
 *
 * When V8_ENABLE_SANDBOX, its primary use is for pointing to objects outside
 * the sandbox, as described below.
 *
 * An external pointer table provides the basic mechanisms to ensure
 * memory-safe access to objects located outside the sandbox, but referenced
 * from within it. When an external pointer table is used, objects located
 * inside the sandbox reference outside objects through indices into the table.
 *
 * Type safety can be ensured by using type-specific tags for the external
 * pointers. These tags will be ORed into the unused top bits of the pointer
 * when storing them and will be ANDed away when loading the pointer later
 * again. If a pointer of the wrong type is accessed, some of the top bits will
 * remain in place, rendering the pointer inaccessible.
 *
 * Temporal memory safety is achieved through garbage collection of the table,
 * which ensures that every entry is either an invalid pointer or a valid
 * pointer pointing to a live object.
 *
 * Spatial memory safety can, if necessary, be ensured by storing the size of a
 * referenced object together with the object itself outside the sandbox, and
 * referencing both through a single entry in the table.
 *
 * The garbage collection algorithm for the table works as follows:
 *  - The top bit of every entry is reserved for the marking bit.
 *  - Every store to an entry automatically sets the marking bit when ORing
 *    with the tag. This avoids the need for write barriers.
 *  - Every load of an entry automatically removes the marking bit when ANDing
 *    with the inverted tag.
 *  - When the GC marking visitor finds a live object with an external pointer,
 *    it marks the corresponding entry as alive through Mark(), which sets the
 *    marking bit using an atomic CAS operation.
 *  - When marking is finished, Sweep() iterates of the table once while the
 *    mutator is stopped and builds a freelist from all dead entries while also
 *    removing the marking bit from any live entry.
 *
 * The freelist is a singly-linked list, using the lower 32 bits of each entry
 * to store the index of the next free entry. When the freelist is empty and a
 * new entry is allocated, the table grows in place and the freelist is
 * re-populated from the newly added entries.
 *
 * When V8_COMPRESS_POINTERS, external pointer tables are also used to ease
 * alignment requirements in heap object fields via indirection.
 */
class V8_EXPORT_PRIVATE ExternalPointerTable {
 public:
  // Size of an ExternalPointerTable, for layout computation in IsolateData.
  // Asserted to be equal to the actual size in external-pointer-table.cc.
  static int constexpr kSize = 3 * kSystemPointerSize;

  ExternalPointerTable() = default;

  // Initializes this external pointer table by reserving the backing memory
  // and initializing the freelist.
  inline void Init(Isolate* isolate);

  // Resets this external pointer table and deletes all associated memory.
  inline void TearDown();

  // Retrieves the entry referenced by the given handle.
  //
  // This method is atomic and can be called from background threads.
  inline Address Get(ExternalPointerHandle handle,
                     ExternalPointerTag tag) const;

  // Sets the entry referenced by the given handle.
  //
  // This method is atomic and can be called from background threads.
  inline void Set(ExternalPointerHandle handle, Address value,
                  ExternalPointerTag tag);

  // Exchanges the entry referenced by the given handle with the given value,
  // returning the previous value. The same tag is applied both to decode the
  // previous value and encode the given value.
  //
  // This method is atomic and can call be called from background threads.
  inline Address Exchange(ExternalPointerHandle handle, Address value,
                          ExternalPointerTag tag);

  // Allocates a new entry in the external pointer table. The caller must
  // initialize the entry afterwards through set(). In particular, the caller is
  // responsible for setting the mark bit of the new entry.
  // TODO(saelo) this can fail, in which case we should probably do GC + retry.
  //
  // This method is atomic and can be called from background threads.
  inline ExternalPointerHandle Allocate();

  // Marks the specified entry as alive.
  //
  // This method is atomic and can be called from background threads.
  inline void Mark(ExternalPointerHandle index);

  // Frees unmarked entries.
  //
  // This method must only be called while mutator threads are stopped as it is
  // not safe to allocate table entries while the table is being swept.
  //
  // Returns the number of live entries after sweeping.
  uint32_t Sweep(Isolate* isolate);

 private:
  // Required for Isolate::CheckIsolateLayout().
  friend class Isolate;

  // An external pointer table grows in blocks of this size. This is also the
  // initial size of the table.
  //
  // The external pointer table is used both for sandboxing external pointers
  // and for easing alignment requirements when compressing pointers. When just
  // doing the latter, expect less usage.
  static const size_t kBlockSize =
      SandboxedExternalPointersAreEnabled() ? 64 * KB : 16 * KB;
  static const size_t kEntriesPerBlock = kBlockSize / kSystemPointerSize;

  // When the table is swept, it first sets the freelist head to this special
  // value to better catch any violation of the "don't-alloc-while-sweeping"
  // requirement (see Sweep()). This value is chosen so it points to the last
  // entry in the table, which should usually be inaccessible (PROT_NONE).
  static const uint32_t kTableIsCurrentlySweepingMarker =
      (kExternalPointerTableReservationSize / kSystemPointerSize) - 1;

  // Returns true if this external pointer table has been initialized.
  bool is_initialized() { return buffer_ != kNullAddress; }

  // Extends the table and adds newly created entries to the freelist. Returns
  // the new freelist head. When calling this method, mutex_ must be locked.
  //
  // TODO(saelo) this can fail, deal with that appropriately.
  uint32_t Grow();

  // Computes the address of the specified entry.
  inline Address entry_address(uint32_t index) const {
    return buffer_ + index * sizeof(Address);
  }

  // When LeakSanitizer is enabled, this method will write the untagged (raw)
  // pointer into the shadow table (located after the real table) at the given
  // index. This is necessary because LSan is unable to scan the pointers in
  // the main table due to the pointer tagging scheme (the values don't "look
  // like" pointers). So instead it can scan the pointers in the shadow table.
  inline void lsan_record_ptr(uint32_t index, Address value) {
#if defined(LEAK_SANITIZER)
    base::Memory<Address>(entry_address(index) +
                          kExternalPointerTableReservationSize) =
        value & ~kExternalPointerTagMask;
#endif  // LEAK_SANITIZER
  }

  // Loads the value at the given index. This method is non-atomic, only use it
  // when no other threads can currently access the table.
  inline Address load(uint32_t index) const {
    return base::Memory<Address>(entry_address(index));
  }

  // Stores the provided value at the given index. This method is non-atomic,
  // only use it when no other threads can currently access the table.
  inline void store(uint32_t index, Address value) {
    lsan_record_ptr(index, value);
    base::Memory<Address>(entry_address(index)) = value;
  }

  // Atomically loads the value at the given index.
  inline Address load_atomic(uint32_t index) const {
    auto addr = reinterpret_cast<base::Atomic64*>(entry_address(index));
    return base::Relaxed_Load(addr);
  }

  // Atomically stores the provided value at the given index.
  inline void store_atomic(uint32_t index, Address value) {
    lsan_record_ptr(index, value);
    auto addr = reinterpret_cast<base::Atomic64*>(entry_address(index));
    base::Relaxed_Store(addr, value);
  }

  // Atomically exchanges the value at the given index with the provided value.
  inline Address exchange_atomic(uint32_t index, Address value) {
    lsan_record_ptr(index, value);
    auto addr = reinterpret_cast<base::Atomic64*>(entry_address(index));
    return static_cast<Address>(base::Relaxed_AtomicExchange(addr, value));
  }

  static bool is_marked(Address entry) {
    return (entry & kExternalPointerMarkBit) == kExternalPointerMarkBit;
  }

  static Address set_mark_bit(Address entry) {
    return entry | kExternalPointerMarkBit;
  }

  static Address clear_mark_bit(Address entry) {
    return entry & ~kExternalPointerMarkBit;
  }

  static bool is_free(Address entry) {
    return (entry & kExternalPointerFreeEntryTag) ==
           kExternalPointerFreeEntryTag;
  }

  static Address make_freelist_entry(uint32_t current_freelist_head) {
    // The next freelist entry is stored in the lower 32 bits of the entry.
    Address entry = current_freelist_head;
    return entry | kExternalPointerFreeEntryTag;
  }

  // The buffer backing this table. This is const after initialization. Should
  // only be accessed using the load_x() and store_x() methods, which take care
  // of atomicicy if necessary.
  Address buffer_ = kNullAddress;

  // The current capacity of this table, which is the number of usable entries.
  uint32_t capacity_ = 0;

  // The index of the first entry on the freelist or zero if the list is empty.
  base::Atomic32 freelist_head_ = 0;

  // Lock protecting the slow path for entry allocation, in particular Grow().
  // As the size of this structure must be predictable (it's part of
  // IsolateData), it cannot directly contain a Mutex and so instead contains a
  // pointer to one.
  base::Mutex* mutex_ = nullptr;
};

}  // namespace internal
}  // namespace v8

#endif  // V8_COMPRESS_POINTERS

#endif  // V8_SANDBOX_EXTERNAL_POINTER_TABLE_H_
