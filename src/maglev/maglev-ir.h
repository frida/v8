// Copyright 2021 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_MAGLEV_MAGLEV_IR_H_
#define V8_MAGLEV_MAGLEV_IR_H_

#include "src/base/bit-field.h"
#include "src/base/logging.h"
#include "src/base/macros.h"
#include "src/base/small-vector.h"
#include "src/base/threaded-list.h"
#include "src/codegen/label.h"
#include "src/codegen/reglist.h"
#include "src/common/globals.h"
#include "src/common/operation.h"
#include "src/compiler/backend/instruction.h"
#include "src/compiler/heap-refs.h"
#include "src/deoptimizer/deoptimize-reason.h"
#include "src/interpreter/bytecode-flags.h"
#include "src/interpreter/bytecode-register.h"
#include "src/maglev/maglev-compilation-unit.h"
#include "src/objects/smi.h"
#include "src/roots/roots.h"
#include "src/utils/utils.h"
#include "src/zone/zone.h"

namespace v8 {
namespace internal {
namespace maglev {

class BasicBlock;
class ProcessingState;
class MaglevCodeGenState;
class MaglevCompilationUnit;
class MaglevGraphLabeller;
class MaglevVregAllocationState;
class CompactInterpreterFrameState;

// Nodes are either
// 1. side-effecting or value-holding SSA nodes in the body of basic blocks, or
// 2. Control nodes that store the control flow at the end of basic blocks, and
// form a separate node hierarchy to non-control nodes.
//
// The macro lists below must match the node class hierarchy.

#define GENERIC_OPERATIONS_NODE_LIST(V) \
  V(GenericAdd)                         \
  V(GenericSubtract)                    \
  V(GenericMultiply)                    \
  V(GenericDivide)                      \
  V(GenericModulus)                     \
  V(GenericExponentiate)                \
  V(GenericBitwiseAnd)                  \
  V(GenericBitwiseOr)                   \
  V(GenericBitwiseXor)                  \
  V(GenericShiftLeft)                   \
  V(GenericShiftRight)                  \
  V(GenericShiftRightLogical)           \
  V(GenericBitwiseNot)                  \
  V(GenericNegate)                      \
  V(GenericIncrement)                   \
  V(GenericDecrement)                   \
  V(GenericEqual)                       \
  V(GenericStrictEqual)                 \
  V(GenericLessThan)                    \
  V(GenericLessThanOrEqual)             \
  V(GenericGreaterThan)                 \
  V(GenericGreaterThanOrEqual)

#define INT32_OPERATIONS_NODE_LIST(V)  \
  V(Int32AddWithOverflow)              \
  V(Int32SubtractWithOverflow)         \
  V(Int32MultiplyWithOverflow)         \
  V(Int32DivideWithOverflow)           \
  /*V(Int32ModulusWithOverflow)*/      \
  /*V(Int32ExponentiateWithOverflow)*/ \
  V(Int32BitwiseAnd)                   \
  V(Int32BitwiseOr)                    \
  V(Int32BitwiseXor)                   \
  V(Int32ShiftLeft)                    \
  V(Int32ShiftRight)                   \
  V(Int32ShiftRightLogical)            \
  /*V(Int32BitwiseNot)     */          \
  /*V(Int32NegateWithOverflow) */      \
  /*V(Int32IncrementWithOverflow)*/    \
  /*V(Int32DecrementWithOverflow)*/    \
  V(Int32Equal)                        \
  V(Int32StrictEqual)                  \
  V(Int32LessThan)                     \
  V(Int32LessThanOrEqual)              \
  V(Int32GreaterThan)                  \
  V(Int32GreaterThanOrEqual)

#define FLOAT64_OPERATIONS_NODE_LIST(V) \
  V(Float64Add)                         \
  V(Float64Subtract)                    \
  V(Float64Multiply)                    \
  V(Float64Divide)                      \
  /*V(Float64Modulus)*/                 \
  /*V(Float64Exponentiate)*/            \
  /*V(Float64Negate) */                 \
  /*V(Float64Increment)*/               \
  /*V(Float64Decrement)*/               \
  V(Float64Equal)                       \
  V(Float64StrictEqual)                 \
  V(Float64LessThan)                    \
  V(Float64LessThanOrEqual)             \
  V(Float64GreaterThan)                 \
  V(Float64GreaterThanOrEqual)

#define CONSTANT_VALUE_NODE_LIST(V) \
  V(Constant)                       \
  V(Float64Constant)                \
  V(Int32Constant)                  \
  V(RootConstant)                   \
  V(SmiConstant)

#define VALUE_NODE_LIST(V)        \
  V(Call)                         \
  V(CallBuiltin)                  \
  V(CallRuntime)                  \
  V(CallWithSpread)               \
  V(Construct)                    \
  V(ConstructWithSpread)          \
  V(CreateEmptyArrayLiteral)      \
  V(CreateArrayLiteral)           \
  V(CreateShallowArrayLiteral)    \
  V(CreateObjectLiteral)          \
  V(CreateEmptyObjectLiteral)     \
  V(CreateShallowObjectLiteral)   \
  V(CreateFunctionContext)        \
  V(CreateClosure)                \
  V(FastCreateClosure)            \
  V(CreateRegExpLiteral)          \
  V(DeleteProperty)               \
  V(ForInPrepare)                 \
  V(ForInNext)                    \
  V(GetSecondReturnedValue)       \
  V(GetTemplateObject)            \
  V(InitialValue)                 \
  V(LoadTaggedField)              \
  V(LoadDoubleField)              \
  V(LoadGlobal)                   \
  V(LoadNamedGeneric)             \
  V(LoadNamedFromSuperGeneric)    \
  V(SetNamedGeneric)              \
  V(DefineNamedOwnGeneric)        \
  V(StoreInArrayLiteralGeneric)   \
  V(StoreGlobal)                  \
  V(GetKeyedGeneric)              \
  V(SetKeyedGeneric)              \
  V(DefineKeyedOwnGeneric)        \
  V(Phi)                          \
  V(RegisterInput)                \
  V(CheckedSmiTag)                \
  V(CheckedSmiUntag)              \
  V(CheckedInternalizedString)    \
  V(ChangeInt32ToFloat64)         \
  V(Float64Box)                   \
  V(CheckedFloat64Unbox)          \
  V(LogicalNot)                   \
  V(SetPendingMessage)            \
  V(ToBooleanLogicalNot)          \
  V(TaggedEqual)                  \
  V(TaggedNotEqual)               \
  V(TestInstanceOf)               \
  V(TestUndetectable)             \
  V(TestTypeOf)                   \
  V(ToName)                       \
  V(ToNumberOrNumeric)            \
  V(ToObject)                     \
  V(ToString)                     \
  CONSTANT_VALUE_NODE_LIST(V)     \
  INT32_OPERATIONS_NODE_LIST(V)   \
  FLOAT64_OPERATIONS_NODE_LIST(V) \
  GENERIC_OPERATIONS_NODE_LIST(V)

#define GAP_MOVE_NODE_LIST(V) \
  V(ConstantGapMove)          \
  V(GapMove)

#define NODE_LIST(V)                  \
  V(CheckMaps)                        \
  V(CheckSmi)                         \
  V(CheckNumber)                      \
  V(CheckHeapObject)                  \
  V(CheckSymbol)                      \
  V(CheckString)                      \
  V(CheckMapsWithMigration)           \
  V(StoreTaggedFieldNoWriteBarrier)   \
  V(StoreTaggedFieldWithWriteBarrier) \
  V(IncreaseInterruptBudget)          \
  V(ReduceInterruptBudget)            \
  V(ThrowReferenceErrorIfHole)        \
  V(ThrowSuperNotCalledIfHole)        \
  V(ThrowSuperAlreadyCalledIfNotHole) \
  V(ThrowIfNotSuperConstructor)       \
  GAP_MOVE_NODE_LIST(V)               \
  VALUE_NODE_LIST(V)

#define BRANCH_CONTROL_NODE_LIST(V) \
  V(BranchIfRootConstant)           \
  V(BranchIfToBooleanTrue)          \
  V(BranchIfReferenceCompare)       \
  V(BranchIfInt32Compare)           \
  V(BranchIfFloat64Compare)         \
  V(BranchIfUndefinedOrNull)        \
  V(BranchIfJSReceiver)

#define CONDITIONAL_CONTROL_NODE_LIST(V) \
  V(Switch)                              \
  BRANCH_CONTROL_NODE_LIST(V)

#define UNCONDITIONAL_CONTROL_NODE_LIST(V) \
  V(Jump)                                  \
  V(JumpLoop)                              \
  V(JumpToInlined)                         \
  V(JumpFromInlined)

#define CONTROL_NODE_LIST(V)       \
  V(Abort)                         \
  V(Return)                        \
  V(Deopt)                         \
  CONDITIONAL_CONTROL_NODE_LIST(V) \
  UNCONDITIONAL_CONTROL_NODE_LIST(V)

#define NODE_BASE_LIST(V) \
  NODE_LIST(V)            \
  CONTROL_NODE_LIST(V)

// Define the opcode enum.
#define DEF_OPCODES(type) k##type,
enum class Opcode : uint16_t { NODE_BASE_LIST(DEF_OPCODES) };
#undef DEF_OPCODES
#define PLUS_ONE(type) +1
static constexpr int kOpcodeCount = NODE_BASE_LIST(PLUS_ONE);
static constexpr Opcode kFirstOpcode = static_cast<Opcode>(0);
static constexpr Opcode kLastOpcode = static_cast<Opcode>(kOpcodeCount - 1);
#undef PLUS_ONE

const char* OpcodeToString(Opcode opcode);
inline std::ostream& operator<<(std::ostream& os, Opcode opcode) {
  return os << OpcodeToString(opcode);
}

#define V(Name) Opcode::k##Name,
static constexpr Opcode kFirstValueNodeOpcode =
    std::min({VALUE_NODE_LIST(V) kLastOpcode});
static constexpr Opcode kLastValueNodeOpcode =
    std::max({VALUE_NODE_LIST(V) kFirstOpcode});
static constexpr Opcode kFirstConstantNodeOpcode =
    std::min({CONSTANT_VALUE_NODE_LIST(V) kLastOpcode});
static constexpr Opcode kLastConstantNodeOpcode =
    std::max({CONSTANT_VALUE_NODE_LIST(V) kFirstOpcode});
static constexpr Opcode kFirstGapMoveNodeOpcode =
    std::min({GAP_MOVE_NODE_LIST(V) kLastOpcode});
static constexpr Opcode kLastGapMoveNodeOpcode =
    std::max({GAP_MOVE_NODE_LIST(V) kFirstOpcode});

static constexpr Opcode kFirstNodeOpcode = std::min({NODE_LIST(V) kLastOpcode});
static constexpr Opcode kLastNodeOpcode = std::max({NODE_LIST(V) kFirstOpcode});

static constexpr Opcode kFirstBranchControlNodeOpcode =
    std::min({BRANCH_CONTROL_NODE_LIST(V) kLastOpcode});
static constexpr Opcode kLastBranchControlNodeOpcode =
    std::max({BRANCH_CONTROL_NODE_LIST(V) kFirstOpcode});

static constexpr Opcode kFirstConditionalControlNodeOpcode =
    std::min({CONDITIONAL_CONTROL_NODE_LIST(V) kLastOpcode});
static constexpr Opcode kLastConditionalControlNodeOpcode =
    std::max({CONDITIONAL_CONTROL_NODE_LIST(V) kFirstOpcode});

static constexpr Opcode kLastUnconditionalControlNodeOpcode =
    std::max({UNCONDITIONAL_CONTROL_NODE_LIST(V) kFirstOpcode});
static constexpr Opcode kFirstUnconditionalControlNodeOpcode =
    std::min({UNCONDITIONAL_CONTROL_NODE_LIST(V) kLastOpcode});

static constexpr Opcode kFirstControlNodeOpcode =
    std::min({CONTROL_NODE_LIST(V) kLastOpcode});
static constexpr Opcode kLastControlNodeOpcode =
    std::max({CONTROL_NODE_LIST(V) kFirstOpcode});
#undef V

constexpr bool IsValueNode(Opcode opcode) {
  return kFirstValueNodeOpcode <= opcode && opcode <= kLastValueNodeOpcode;
}
constexpr bool IsConstantNode(Opcode opcode) {
  return kFirstConstantNodeOpcode <= opcode &&
         opcode <= kLastConstantNodeOpcode;
}
constexpr bool IsGapMoveNode(Opcode opcode) {
  return kFirstGapMoveNodeOpcode <= opcode && opcode <= kLastGapMoveNodeOpcode;
}
constexpr bool IsBranchControlNode(Opcode opcode) {
  return kFirstBranchControlNodeOpcode <= opcode &&
         opcode <= kLastBranchControlNodeOpcode;
}
constexpr bool IsConditionalControlNode(Opcode opcode) {
  return kFirstConditionalControlNodeOpcode <= opcode &&
         opcode <= kLastConditionalControlNodeOpcode;
}
constexpr bool IsUnconditionalControlNode(Opcode opcode) {
  return kFirstUnconditionalControlNodeOpcode <= opcode &&
         opcode <= kLastUnconditionalControlNodeOpcode;
}

// Forward-declare NodeBase sub-hierarchies.
class Node;
class ControlNode;
class ConditionalControlNode;
class BranchControlNode;
class UnconditionalControlNode;
class ValueNode;

enum class ValueRepresentation : uint8_t { kTagged, kInt32, kFloat64 };

#define DEF_FORWARD_DECLARATION(type, ...) class type;
NODE_BASE_LIST(DEF_FORWARD_DECLARATION)
#undef DEF_FORWARD_DECLARATION

using NodeIdT = uint32_t;
static constexpr uint32_t kInvalidNodeId = 0;
static constexpr uint32_t kFirstValidNodeId = 1;

class OpProperties {
 public:
  constexpr bool is_call() const { return kIsCallBit::decode(bitfield_); }
  constexpr bool can_eager_deopt() const {
    return kCanEagerDeoptBit::decode(bitfield_);
  }
  constexpr bool can_lazy_deopt() const {
    return kCanLazyDeoptBit::decode(bitfield_);
  }
  constexpr bool can_read() const { return kCanReadBit::decode(bitfield_); }
  constexpr bool can_write() const { return kCanWriteBit::decode(bitfield_); }
  constexpr bool non_memory_side_effects() const {
    return kNonMemorySideEffectsBit::decode(bitfield_);
  }
  constexpr ValueRepresentation value_representation() const {
    return kValueRepresentationBits::decode(bitfield_);
  }
  constexpr bool is_conversion() const {
    return kIsConversionBit::decode(bitfield_);
  }
  constexpr bool needs_register_snapshot() const {
    return kNeedsRegisterSnapshotBit::decode(bitfield_);
  }
  constexpr bool is_pure() const {
    return (bitfield_ & kPureMask) == kPureValue;
  }
  constexpr bool is_required_when_unused() const {
    return can_write() || non_memory_side_effects();
  }

  constexpr OpProperties operator|(const OpProperties& that) {
    return OpProperties(bitfield_ | that.bitfield_);
  }

  static constexpr OpProperties Pure() { return OpProperties(kPureValue); }
  static constexpr OpProperties Call() {
    return OpProperties(kIsCallBit::encode(true));
  }
  static constexpr OpProperties EagerDeopt() {
    return OpProperties(kCanEagerDeoptBit::encode(true));
  }
  static constexpr OpProperties LazyDeopt() {
    return OpProperties(kCanLazyDeoptBit::encode(true));
  }
  static constexpr OpProperties Reading() {
    return OpProperties(kCanReadBit::encode(true));
  }
  static constexpr OpProperties Writing() {
    return OpProperties(kCanWriteBit::encode(true));
  }
  static constexpr OpProperties NonMemorySideEffects() {
    return OpProperties(kNonMemorySideEffectsBit::encode(true));
  }
  static constexpr OpProperties TaggedValue() {
    return OpProperties(
        kValueRepresentationBits::encode(ValueRepresentation::kTagged));
  }
  static constexpr OpProperties Int32() {
    return OpProperties(
        kValueRepresentationBits::encode(ValueRepresentation::kInt32));
  }
  static constexpr OpProperties Float64() {
    return OpProperties(
        kValueRepresentationBits::encode(ValueRepresentation::kFloat64));
  }
  static constexpr OpProperties ConversionNode() {
    return OpProperties(kIsConversionBit::encode(true));
  }
  static constexpr OpProperties NeedsRegisterSnapshot() {
    return OpProperties(kNeedsRegisterSnapshotBit::encode(true));
  }
  static constexpr OpProperties JSCall() {
    return Call() | NonMemorySideEffects() | LazyDeopt();
  }
  static constexpr OpProperties AnySideEffects() {
    return Reading() | Writing() | NonMemorySideEffects();
  }
  static constexpr OpProperties DeferredCall() {
    // Operations with a deferred call need a snapshot of register state,
    // because they need to be able to push registers to save them, and annotate
    // the safepoint with information about which registers are tagged.
    return NeedsRegisterSnapshot();
  }

  constexpr explicit OpProperties(uint32_t bitfield) : bitfield_(bitfield) {}
  operator uint32_t() const { return bitfield_; }

 private:
  using kIsCallBit = base::BitField<bool, 0, 1>;
  using kCanEagerDeoptBit = kIsCallBit::Next<bool, 1>;
  using kCanLazyDeoptBit = kCanEagerDeoptBit::Next<bool, 1>;
  using kCanReadBit = kCanLazyDeoptBit::Next<bool, 1>;
  using kCanWriteBit = kCanReadBit::Next<bool, 1>;
  using kNonMemorySideEffectsBit = kCanWriteBit::Next<bool, 1>;
  using kValueRepresentationBits =
      kNonMemorySideEffectsBit::Next<ValueRepresentation, 2>;
  using kIsConversionBit = kValueRepresentationBits::Next<bool, 1>;
  using kNeedsRegisterSnapshotBit = kIsConversionBit::Next<bool, 1>;

  static const uint32_t kPureMask = kCanReadBit::kMask | kCanWriteBit::kMask |
                                    kNonMemorySideEffectsBit::kMask;
  static const uint32_t kPureValue = kCanReadBit::encode(false) |
                                     kCanWriteBit::encode(false) |
                                     kNonMemorySideEffectsBit::encode(false);

  const uint32_t bitfield_;

 public:
  static const size_t kSize = kNeedsRegisterSnapshotBit::kLastUsedBit + 1;
};

class ValueLocation {
 public:
  ValueLocation() = default;

  template <typename... Args>
  void SetUnallocated(Args&&... args) {
    DCHECK(operand_.IsInvalid());
    operand_ = compiler::UnallocatedOperand(args...);
  }

  template <typename... Args>
  void SetAllocated(Args&&... args) {
    DCHECK(operand_.IsUnallocated());
    operand_ = compiler::AllocatedOperand(args...);
  }

  // Only to be used on inputs that inherit allocation.
  void InjectLocation(compiler::InstructionOperand location) {
    operand_ = location;
  }

  template <typename... Args>
  void SetConstant(Args&&... args) {
    DCHECK(operand_.IsUnallocated());
    operand_ = compiler::ConstantOperand(args...);
  }

  Register AssignedGeneralRegister() const {
    DCHECK(!IsDoubleRegister());
    return compiler::AllocatedOperand::cast(operand_).GetRegister();
  }

  DoubleRegister AssignedDoubleRegister() const {
    DCHECK(IsDoubleRegister());
    return compiler::AllocatedOperand::cast(operand_).GetDoubleRegister();
  }

  bool IsDoubleRegister() const { return operand_.IsDoubleRegister(); }

  const compiler::InstructionOperand& operand() const { return operand_; }
  const compiler::InstructionOperand& operand() { return operand_; }

 private:
  compiler::InstructionOperand operand_;
};

class InputLocation : public ValueLocation {
 public:
  NodeIdT next_use_id() const { return next_use_id_; }
  // Used in ValueNode::mark_use
  NodeIdT* get_next_use_id_address() { return &next_use_id_; }

 private:
  NodeIdT next_use_id_ = kInvalidNodeId;
};

class Input : public InputLocation {
 public:
  explicit Input(ValueNode* node) : node_(node) {}
  ValueNode* node() const { return node_; }

 private:
  ValueNode* node_;
};

class CheckpointedInterpreterState {
 public:
  CheckpointedInterpreterState() = default;
  CheckpointedInterpreterState(BytecodeOffset bytecode_position,
                               const CompactInterpreterFrameState* state,
                               const CheckpointedInterpreterState* parent)
      : bytecode_position(bytecode_position),
        register_frame(state),
        parent(parent) {}

  BytecodeOffset bytecode_position = BytecodeOffset::None();
  const CompactInterpreterFrameState* register_frame = nullptr;
  const CheckpointedInterpreterState* parent = nullptr;
};

class DeoptInfo {
 protected:
  DeoptInfo(Zone* zone, const MaglevCompilationUnit& compilation_unit,
            CheckpointedInterpreterState checkpoint);

 public:
  const MaglevCompilationUnit& unit;
  CheckpointedInterpreterState state;
  InputLocation* input_locations = nullptr;
  Label deopt_entry_label;
  int translation_index = -1;
};

struct RegisterSnapshot {
  RegList live_registers;
  RegList live_tagged_registers;
  DoubleRegList live_double_registers;
};

class EagerDeoptInfo : public DeoptInfo {
 public:
  EagerDeoptInfo(Zone* zone, const MaglevCompilationUnit& compilation_unit,
                 CheckpointedInterpreterState checkpoint)
      : DeoptInfo(zone, compilation_unit, checkpoint) {}
  DeoptimizeReason reason = DeoptimizeReason::kUnknown;
};

class LazyDeoptInfo : public DeoptInfo {
 public:
  LazyDeoptInfo(Zone* zone, const MaglevCompilationUnit& compilation_unit,
                CheckpointedInterpreterState checkpoint)
      : DeoptInfo(zone, compilation_unit, checkpoint) {}

  int deopting_call_return_pc = -1;
  interpreter::Register result_location =
      interpreter::Register::invalid_value();
  int result_size = 1;
};

// Dummy type for the initial raw allocation.
struct NodeWithInlineInputs {};

namespace detail {
// Helper for getting the static opcode of a Node subclass. This is in a
// "detail" namespace rather than in NodeBase because we can't template
// specialize outside of namespace scopes before C++17.
template <class T>
struct opcode_of_helper;

#define DEF_OPCODE_OF(Name)                          \
  template <>                                        \
  struct opcode_of_helper<Name> {                    \
    static constexpr Opcode value = Opcode::k##Name; \
  };
NODE_BASE_LIST(DEF_OPCODE_OF)
#undef DEF_OPCODE_OF

template <typename T>
T* ObjectPtrBeforeAddress(void* address) {
  char* address_as_char_ptr = reinterpret_cast<char*>(address);
  char* object_ptr_as_char_ptr = address_as_char_ptr - sizeof(T);
  return reinterpret_cast<T*>(object_ptr_as_char_ptr);
}

template <typename T>
const T* ObjectPtrBeforeAddress(const void* address) {
  const char* address_as_char_ptr = reinterpret_cast<const char*>(address);
  const char* object_ptr_as_char_ptr = address_as_char_ptr - sizeof(T);
  return reinterpret_cast<const T*>(object_ptr_as_char_ptr);
}

}  // namespace detail

class NodeBase : public ZoneObject {
 private:
  // Bitfield specification.
  using OpcodeField = base::BitField64<Opcode, 0, 16>;
  static_assert(OpcodeField::is_valid(kLastOpcode));
  using OpPropertiesField =
      OpcodeField::Next<OpProperties, OpProperties::kSize>;
  using NumTemporariesNeededField = OpPropertiesField::Next<uint8_t, 2>;
  // Align input count to 32-bit.
  using UnusedField = NumTemporariesNeededField::Next<uint8_t, 4>;
  using InputCountField = UnusedField::Next<size_t, 17>;
  static_assert(InputCountField::kShift == 32);

 protected:
  // Subclasses may use the remaining bitfield bits.
  template <class T, int size>
  using NextBitField = InputCountField::Next<T, size>;

  template <class T>
  static constexpr Opcode opcode_of = detail::opcode_of_helper<T>::value;

  static constexpr int kMaxInputs = InputCountField::kMax;

 public:
  template <class Derived, typename... Args>
  static Derived* New(Zone* zone, std::initializer_list<ValueNode*> inputs,
                      Args&&... args) {
    Derived* node =
        Allocate<Derived>(zone, inputs.size(), std::forward<Args>(args)...);

    int i = 0;
    for (ValueNode* input : inputs) {
      DCHECK_NOT_NULL(input);
      node->set_input(i++, input);
    }

    return node;
  }

  template <class Derived, typename... Args>
  static Derived* New(Zone* zone, const MaglevCompilationUnit& compilation_unit,
                      CheckpointedInterpreterState checkpoint, Args&&... args) {
    Derived* node = New<Derived>(zone, std::forward<Args>(args)...);
    if constexpr (Derived::kProperties.can_eager_deopt()) {
      new (node->eager_deopt_info())
          EagerDeoptInfo(zone, compilation_unit, checkpoint);
    } else {
      static_assert(Derived::kProperties.can_lazy_deopt());
      new (node->lazy_deopt_info())
          LazyDeoptInfo(zone, compilation_unit, checkpoint);
    }
    return node;
  }

  // Inputs must be initialized manually.
  template <class Derived, typename... Args>
  static Derived* New(Zone* zone, size_t input_count, Args&&... args) {
    Derived* node =
        Allocate<Derived>(zone, input_count, std::forward<Args>(args)...);
    return node;
  }

  // Overwritten by subclasses.
  static constexpr OpProperties kProperties =
      OpProperties::Pure() | OpProperties::TaggedValue();

  constexpr Opcode opcode() const { return OpcodeField::decode(bitfield_); }
  constexpr OpProperties properties() const {
    return OpPropertiesField::decode(bitfield_);
  }

  template <class T>
  constexpr bool Is() const;

  template <class T>
  constexpr T* Cast() {
    DCHECK(Is<T>());
    return static_cast<T*>(this);
  }
  template <class T>
  constexpr const T* Cast() const {
    DCHECK(Is<T>());
    return static_cast<const T*>(this);
  }
  template <class T>
  constexpr T* TryCast() {
    return Is<T>() ? static_cast<T*>(this) : nullptr;
  }

  constexpr bool has_inputs() const { return input_count() > 0; }
  constexpr int input_count() const {
    static_assert(InputCountField::kMax <= kMaxInt);
    return static_cast<int>(InputCountField::decode(bitfield_));
  }

  Input& input(int index) {
    DCHECK_LT(index, input_count());
    return *(input_base() - index);
  }
  const Input& input(int index) const {
    DCHECK_LT(index, input_count());
    return *(input_base() - index);
  }

  // Input iterators, use like:
  //
  //  for (Input& input : *node) { ... }
  auto begin() { return std::make_reverse_iterator(&input(-1)); }
  auto end() { return std::make_reverse_iterator(&input(input_count() - 1)); }

  constexpr bool has_id() const { return id_ != kInvalidNodeId; }
  constexpr NodeIdT id() const {
    DCHECK_NE(id_, kInvalidNodeId);
    return id_;
  }
  void set_id(NodeIdT id) {
    DCHECK_EQ(id_, kInvalidNodeId);
    DCHECK_NE(id, kInvalidNodeId);
    id_ = id;
  }

  uint8_t num_temporaries_needed() const {
    return NumTemporariesNeededField::decode(bitfield_);
  }

  RegList temporaries() const { return temporaries_; }

  void assign_temporaries(RegList list) { temporaries_ = list; }

  void Print(std::ostream& os, MaglevGraphLabeller*) const;

  EagerDeoptInfo* eager_deopt_info() {
    DCHECK(properties().can_eager_deopt());
    DCHECK(!properties().can_lazy_deopt());
    return detail::ObjectPtrBeforeAddress<EagerDeoptInfo>(last_input_address());
  }

  const EagerDeoptInfo* eager_deopt_info() const {
    DCHECK(properties().can_eager_deopt());
    DCHECK(!properties().can_lazy_deopt());
    return detail::ObjectPtrBeforeAddress<EagerDeoptInfo>(last_input_address());
  }

  LazyDeoptInfo* lazy_deopt_info() {
    DCHECK(properties().can_lazy_deopt());
    DCHECK(!properties().can_eager_deopt());
    return detail::ObjectPtrBeforeAddress<LazyDeoptInfo>(last_input_address());
  }

  const LazyDeoptInfo* lazy_deopt_info() const {
    DCHECK(properties().can_lazy_deopt());
    DCHECK(!properties().can_eager_deopt());
    return detail::ObjectPtrBeforeAddress<LazyDeoptInfo>(last_input_address());
  }

  const RegisterSnapshot& register_snapshot() const {
    DCHECK(properties().needs_register_snapshot());
    if (properties().can_eager_deopt()) {
      return *detail::ObjectPtrBeforeAddress<RegisterSnapshot>(
          eager_deopt_info());
    } else if (properties().can_lazy_deopt()) {
      return *detail::ObjectPtrBeforeAddress<RegisterSnapshot>(
          lazy_deopt_info());
    } else {
      return *detail::ObjectPtrBeforeAddress<RegisterSnapshot>(
          last_input_address());
    }
  }

  void set_register_snapshot(RegisterSnapshot snapshot) {
    DCHECK(properties().needs_register_snapshot());
    if (properties().can_eager_deopt()) {
      *detail::ObjectPtrBeforeAddress<RegisterSnapshot>(eager_deopt_info()) =
          snapshot;
    } else if (properties().can_lazy_deopt()) {
      *detail::ObjectPtrBeforeAddress<RegisterSnapshot>(lazy_deopt_info()) =
          snapshot;
    } else {
      *detail::ObjectPtrBeforeAddress<RegisterSnapshot>(last_input_address()) =
          snapshot;
    }
  }

 protected:
  explicit NodeBase(uint64_t bitfield) : bitfield_(bitfield) {}

  Input* input_base() { return detail::ObjectPtrBeforeAddress<Input>(this); }
  const Input* input_base() const {
    return detail::ObjectPtrBeforeAddress<Input>(this);
  }
  Input* last_input_address() { return &input(input_count() - 1); }
  const Input* last_input_address() const { return &input(input_count() - 1); }

  void set_input(int index, ValueNode* node) {
    new (&input(index)) Input(node);
  }

  // For nodes that don't have data past the input, allow trimming the input
  // count. This is used by Phis to reduce inputs when merging in dead control
  // flow.
  void reduce_input_count() {
    DCHECK_EQ(opcode(), Opcode::kPhi);
    DCHECK(!properties().can_lazy_deopt());
    DCHECK(!properties().can_eager_deopt());
    bitfield_ = InputCountField::update(bitfield_, input_count() - 1);
  }

  // Specify that there need to be a certain number of registers free (i.e.
  // useable as scratch registers) on entry into this node.
  //
  // Does not include any registers requested by RequireSpecificTemporary.
  void set_temporaries_needed(uint8_t value) {
    DCHECK_EQ(num_temporaries_needed(), 0);
    bitfield_ = NumTemporariesNeededField::update(bitfield_, value);
  }

  // Require that a specific register is free (and therefore clobberable) by the
  // entry into this node.
  void RequireSpecificTemporary(Register reg) { temporaries_.set(reg); }

 private:
  template <class Derived, typename... Args>
  static Derived* Allocate(Zone* zone, size_t input_count, Args&&... args) {
    static_assert(
        !Derived::kProperties.can_eager_deopt() ||
            !Derived::kProperties.can_lazy_deopt(),
        "The current deopt info representation, at the end of inputs, requires "
        "that we cannot have both lazy and eager deopts on a node. If we ever "
        "need this, we have to update accessors to check node->properties() "
        "for which deopts are active.");
    constexpr size_t size_before_inputs = RoundUp<alignof(Input)>(
        (Derived::kProperties.needs_register_snapshot()
             ? sizeof(RegisterSnapshot)
             : 0) +
        (Derived::kProperties.can_eager_deopt() ? sizeof(EagerDeoptInfo) : 0) +
        (Derived::kProperties.can_lazy_deopt() ? sizeof(LazyDeoptInfo) : 0));

    static_assert(IsAligned(size_before_inputs, alignof(Input)));
    const size_t size_before_node =
        size_before_inputs + input_count * sizeof(Input);

    DCHECK(IsAligned(size_before_inputs, alignof(Derived)));
    const size_t size = size_before_node + sizeof(Derived);
    intptr_t raw_buffer =
        reinterpret_cast<intptr_t>(zone->Allocate<NodeWithInlineInputs>(size));
    void* node_buffer = reinterpret_cast<void*>(raw_buffer + size_before_node);
    uint64_t bitfield = OpcodeField::encode(opcode_of<Derived>) |
                        OpPropertiesField::encode(Derived::kProperties) |
                        InputCountField::encode(input_count);
    Derived* node =
        new (node_buffer) Derived(bitfield, std::forward<Args>(args)...);
    return node;
  }

  uint64_t bitfield_;
  NodeIdT id_ = kInvalidNodeId;
  RegList temporaries_;

  NodeBase() = delete;
  NodeBase(const NodeBase&) = delete;
  NodeBase(NodeBase&&) = delete;
  NodeBase& operator=(const NodeBase&) = delete;
  NodeBase& operator=(NodeBase&&) = delete;
};

template <class T>
constexpr bool NodeBase::Is() const {
  return opcode() == opcode_of<T>;
}

// Specialized sub-hierarchy type checks.
template <>
constexpr bool NodeBase::Is<ValueNode>() const {
  return IsValueNode(opcode());
}
template <>
constexpr bool NodeBase::Is<BranchControlNode>() const {
  return IsBranchControlNode(opcode());
}
template <>
constexpr bool NodeBase::Is<ConditionalControlNode>() const {
  return IsConditionalControlNode(opcode());
}
template <>
constexpr bool NodeBase::Is<UnconditionalControlNode>() const {
  return IsUnconditionalControlNode(opcode());
}

// The Node class hierarchy contains all non-control nodes.
class Node : public NodeBase {
 public:
  using List = base::ThreadedListWithUnsafeInsertions<Node>;

  inline ValueLocation& result();

  Node* NextNode() const { return next_; }

 protected:
  using NodeBase::NodeBase;

 private:
  Node** next() { return &next_; }
  Node* next_ = nullptr;

  friend List;
  friend base::ThreadedListTraits<Node>;
};

// All non-control nodes with a result.
class ValueNode : public Node {
 public:
  ValueLocation& result() { return result_; }
  const ValueLocation& result() const { return result_; }

  const compiler::InstructionOperand& hint() const {
    DCHECK_EQ(state_, kSpillOrHint);
    DCHECK(spill_or_hint_.IsInvalid() || spill_or_hint_.IsUnallocated());
    return spill_or_hint_;
  }

  bool is_loadable() const {
    DCHECK_EQ(state_, kSpillOrHint);
    return spill_or_hint_.IsConstant() || spill_or_hint_.IsAnyStackSlot();
  }

  bool is_spilled() const { return spill_or_hint_.IsAnyStackSlot(); }

  void SetNoSpillOrHint();
  void SetConstantLocation();

  /* For constants only. */
  void LoadToRegister(MaglevCodeGenState*, Register);
  void LoadToRegister(MaglevCodeGenState*, DoubleRegister);
  void DoLoadToRegister(MaglevCodeGenState*, Register);
  void DoLoadToRegister(MaglevCodeGenState*, DoubleRegister);
  Handle<Object> Reify(LocalIsolate* isolate);

  void Spill(compiler::AllocatedOperand operand) {
#ifdef DEBUG
    if (state_ == kLastUse) {
      state_ = kSpillOrHint;
    } else {
      DCHECK(!is_loadable());
    }
#endif  // DEBUG
    DCHECK(!IsConstantNode(opcode()));
    DCHECK(operand.IsAnyStackSlot());
    spill_or_hint_ = operand;
    DCHECK(spill_or_hint_.IsAnyStackSlot());
  }

  compiler::AllocatedOperand spill_slot() const {
    DCHECK(is_spilled());
    return compiler::AllocatedOperand::cast(loadable_slot());
  }

  compiler::InstructionOperand loadable_slot() const {
    DCHECK_EQ(state_, kSpillOrHint);
    DCHECK(is_loadable());
    return spill_or_hint_;
  }

  void mark_use(NodeIdT id, InputLocation* input_location) {
    DCHECK_EQ(state_, kLastUse);
    DCHECK_NE(id, kInvalidNodeId);
    DCHECK_LT(start_id(), id);
    DCHECK_IMPLIES(has_valid_live_range(), id >= end_id_);
    end_id_ = id;
    *last_uses_next_use_id_ = id;
    last_uses_next_use_id_ = input_location->get_next_use_id_address();
    DCHECK_EQ(*last_uses_next_use_id_, kInvalidNodeId);
  }

  struct LiveRange {
    NodeIdT start = kInvalidNodeId;
    NodeIdT end = kInvalidNodeId;
  };

  bool has_valid_live_range() const { return end_id_ != 0; }
  LiveRange live_range() const { return {start_id(), end_id_}; }
  NodeIdT next_use() const { return next_use_; }

  // The following metods should only be used during register allocation, to
  // mark the _current_ state of this Node according to the register allocator.
  void set_next_use(NodeIdT use) { next_use_ = use; }

  // A node is dead once it has no more upcoming uses.
  bool is_dead() const { return next_use_ == kInvalidNodeId; }

  constexpr bool use_double_register() const {
    return (properties().value_representation() ==
            ValueRepresentation::kFloat64);
  }

  constexpr MachineRepresentation GetMachineRepresentation() const {
    switch (properties().value_representation()) {
      case ValueRepresentation::kTagged:
        return MachineRepresentation::kTagged;
      case ValueRepresentation::kInt32:
        return MachineRepresentation::kWord32;
      case ValueRepresentation::kFloat64:
        return MachineRepresentation::kFloat64;
    }
  }

  void AddRegister(Register reg) {
    DCHECK(!use_double_register());
    registers_with_result_.set(reg);
  }
  void AddRegister(DoubleRegister reg) {
    DCHECK(use_double_register());
    double_registers_with_result_.set(reg);
  }

  void RemoveRegister(Register reg) {
    DCHECK(!use_double_register());
    registers_with_result_.clear(reg);
  }
  void RemoveRegister(DoubleRegister reg) {
    DCHECK(use_double_register());
    double_registers_with_result_.clear(reg);
  }

  template <typename T>
  inline RegListBase<T> ClearRegisters();

  int num_registers() const {
    if (use_double_register()) {
      return double_registers_with_result_.Count();
    }
    return registers_with_result_.Count();
  }
  bool has_register() const {
    if (use_double_register()) {
      return double_registers_with_result_ != kEmptyDoubleRegList;
    }
    return registers_with_result_ != kEmptyRegList;
  }
  bool is_in_register(Register reg) const {
    DCHECK(!use_double_register());
    return registers_with_result_.has(reg);
  }
  bool is_in_register(DoubleRegister reg) const {
    DCHECK(use_double_register());
    return double_registers_with_result_.has(reg);
  }

  template <typename T>
  RegListBase<T> result_registers() {
    if constexpr (std::is_same<T, DoubleRegister>::value) {
      DCHECK(use_double_register());
      return double_registers_with_result_;
    } else {
      DCHECK(!use_double_register());
      return registers_with_result_;
    }
  }

  compiler::InstructionOperand allocation() const {
    if (has_register()) {
      return compiler::AllocatedOperand(compiler::LocationOperand::REGISTER,
                                        GetMachineRepresentation(),
                                        FirstRegisterCode());
    }
    DCHECK(is_loadable());
    return spill_or_hint_;
  }

 protected:
  explicit ValueNode(uint64_t bitfield)
      : Node(bitfield),
        last_uses_next_use_id_(&next_use_)
#ifdef DEBUG
        ,
        state_(kLastUse)
#endif  // DEBUG
  {
    if (use_double_register()) {
      double_registers_with_result_ = kEmptyDoubleRegList;
    } else {
      registers_with_result_ = kEmptyRegList;
    }
  }

  int FirstRegisterCode() const {
    if (use_double_register()) {
      return double_registers_with_result_.first().code();
    }
    return registers_with_result_.first().code();
  }

  // Rename for better pairing with `end_id`.
  NodeIdT start_id() const { return id(); }

  NodeIdT end_id_ = kInvalidNodeId;
  NodeIdT next_use_ = kInvalidNodeId;
  ValueLocation result_;
  union {
    RegList registers_with_result_;
    DoubleRegList double_registers_with_result_;
  };
  union {
    // Pointer to the current last use's next_use_id field. Most of the time
    // this will be a pointer to an Input's next_use_id_ field, but it's
    // initialized to this node's next_use_ to track the first use.
    NodeIdT* last_uses_next_use_id_;
    compiler::InstructionOperand spill_or_hint_;
  };
#ifdef DEBUG
  // TODO(leszeks): Consider spilling into kSpill and kHint.
  enum { kLastUse, kSpillOrHint } state_;
#endif  // DEBUG
};

template <>
inline RegList ValueNode::ClearRegisters() {
  DCHECK(!use_double_register());
  return std::exchange(registers_with_result_, kEmptyRegList);
}

template <>
inline DoubleRegList ValueNode::ClearRegisters() {
  DCHECK(use_double_register());
  return std::exchange(double_registers_with_result_, kEmptyDoubleRegList);
}

ValueLocation& Node::result() {
  DCHECK(Is<ValueNode>());
  return Cast<ValueNode>()->result();
}

template <class Derived>
class NodeT : public Node {
  static_assert(!IsValueNode(opcode_of<Derived>));

 public:
  constexpr Opcode opcode() const { return opcode_of<Derived>; }
  const OpProperties& properties() const { return Derived::kProperties; }

 protected:
  explicit NodeT(uint64_t bitfield) : Node(bitfield) {
    DCHECK_EQ(NodeBase::opcode(), opcode_of<Derived>);
  }
};

template <size_t InputCount, class Derived>
class FixedInputNodeT : public NodeT<Derived> {
  static constexpr size_t kInputCount = InputCount;

 public:
  // Shadowing for static knowledge.
  constexpr bool has_inputs() const { return input_count() > 0; }
  constexpr uint16_t input_count() const { return kInputCount; }
  auto end() {
    return std::make_reverse_iterator(&this->input(input_count() - 1));
  }

 protected:
  explicit FixedInputNodeT(uint64_t bitfield) : NodeT<Derived>(bitfield) {
    DCHECK_EQ(NodeBase::input_count(), kInputCount);
  }
};

template <class Derived>
class ValueNodeT : public ValueNode {
  static_assert(IsValueNode(opcode_of<Derived>));

 public:
  constexpr Opcode opcode() const { return opcode_of<Derived>; }
  const OpProperties& properties() const { return Derived::kProperties; }

 protected:
  explicit ValueNodeT(uint64_t bitfield) : ValueNode(bitfield) {
    DCHECK_EQ(NodeBase::opcode(), opcode_of<Derived>);
  }
};

template <size_t InputCount, class Derived>
class FixedInputValueNodeT : public ValueNodeT<Derived> {
  static constexpr size_t kInputCount = InputCount;

 public:
  // Shadowing for static knowledge.
  constexpr bool has_inputs() const { return input_count() > 0; }
  constexpr uint16_t input_count() const { return kInputCount; }
  auto end() {
    return std::make_reverse_iterator(&this->input(input_count() - 1));
  }

 protected:
  explicit FixedInputValueNodeT(uint64_t bitfield)
      : ValueNodeT<Derived>(bitfield) {
    DCHECK_EQ(NodeBase::input_count(), kInputCount);
  }
};

template <class Derived, Operation kOperation>
class UnaryWithFeedbackNode : public FixedInputValueNodeT<1, Derived> {
  using Base = FixedInputValueNodeT<1, Derived>;

 public:
  // The implementation currently calls runtime.
  static constexpr OpProperties kProperties = OpProperties::JSCall();

  static constexpr int kOperandIndex = 0;
  Input& operand_input() { return Node::input(kOperandIndex); }
  compiler::FeedbackSource feedback() const { return feedback_; }

 protected:
  explicit UnaryWithFeedbackNode(uint64_t bitfield,
                                 const compiler::FeedbackSource& feedback)
      : Base(bitfield), feedback_(feedback) {}

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}

  const compiler::FeedbackSource feedback_;
};

template <class Derived, Operation kOperation>
class BinaryWithFeedbackNode : public FixedInputValueNodeT<2, Derived> {
  using Base = FixedInputValueNodeT<2, Derived>;

 public:
  // The implementation currently calls runtime.
  static constexpr OpProperties kProperties = OpProperties::JSCall();

  static constexpr int kLeftIndex = 0;
  static constexpr int kRightIndex = 1;
  Input& left_input() { return Node::input(kLeftIndex); }
  Input& right_input() { return Node::input(kRightIndex); }
  compiler::FeedbackSource feedback() const { return feedback_; }

 protected:
  BinaryWithFeedbackNode(uint64_t bitfield,
                         const compiler::FeedbackSource& feedback)
      : Base(bitfield), feedback_(feedback) {}

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}

  const compiler::FeedbackSource feedback_;
};

#define DEF_OPERATION_NODE(Name, Super, OpName)                       \
  class Name : public Super<Name, Operation::k##OpName> {             \
    using Base = Super<Name, Operation::k##OpName>;                   \
                                                                      \
   public:                                                            \
    Name(uint64_t bitfield, const compiler::FeedbackSource& feedback) \
        : Base(bitfield, feedback) {}                                 \
    void AllocateVreg(MaglevVregAllocationState*);                    \
    void GenerateCode(MaglevCodeGenState*, const ProcessingState&);   \
    void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}    \
  };

#define DEF_UNARY_WITH_FEEDBACK_NODE(Name) \
  DEF_OPERATION_NODE(Generic##Name, UnaryWithFeedbackNode, Name)
#define DEF_BINARY_WITH_FEEDBACK_NODE(Name) \
  DEF_OPERATION_NODE(Generic##Name, BinaryWithFeedbackNode, Name)
UNARY_OPERATION_LIST(DEF_UNARY_WITH_FEEDBACK_NODE)
ARITHMETIC_OPERATION_LIST(DEF_BINARY_WITH_FEEDBACK_NODE)
COMPARISON_OPERATION_LIST(DEF_BINARY_WITH_FEEDBACK_NODE)
#undef DEF_UNARY_WITH_FEEDBACK_NODE
#undef DEF_BINARY_WITH_FEEDBACK_NODE

#undef DEF_OPERATION_NODE

template <class Derived, Operation kOperation>
class Int32BinaryWithOverflowNode : public FixedInputValueNodeT<2, Derived> {
  using Base = FixedInputValueNodeT<2, Derived>;

 public:
  static constexpr OpProperties kProperties =
      OpProperties::EagerDeopt() | OpProperties::Int32();

  static constexpr int kLeftIndex = 0;
  static constexpr int kRightIndex = 1;
  Input& left_input() { return Node::input(kLeftIndex); }
  Input& right_input() { return Node::input(kRightIndex); }

 protected:
  explicit Int32BinaryWithOverflowNode(uint64_t bitfield) : Base(bitfield) {}

  // void AllocateVreg(MaglevVregAllocationState*, const ProcessingState&);
  // void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}
};

#define DEF_OPERATION_NODE(Name, Super, OpName)                     \
  class Name : public Super<Name, Operation::k##OpName> {           \
    using Base = Super<Name, Operation::k##OpName>;                 \
                                                                    \
   public:                                                          \
    explicit Name(uint64_t bitfield) : Base(bitfield) {}            \
    void AllocateVreg(MaglevVregAllocationState*);                  \
    void GenerateCode(MaglevCodeGenState*, const ProcessingState&); \
    void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}  \
  };

#define DEF_INT32_BINARY_WITH_OVERFLOW_NODE(Name)                            \
  DEF_OPERATION_NODE(Int32##Name##WithOverflow, Int32BinaryWithOverflowNode, \
                     Name)
DEF_INT32_BINARY_WITH_OVERFLOW_NODE(Add)
DEF_INT32_BINARY_WITH_OVERFLOW_NODE(Subtract)
DEF_INT32_BINARY_WITH_OVERFLOW_NODE(Multiply)
DEF_INT32_BINARY_WITH_OVERFLOW_NODE(Divide)
// DEF_INT32_BINARY_WITH_OVERFLOW_NODE(Modulus)
// DEF_INT32_BINARY_WITH_OVERFLOW_NODE(Exponentiate)
#undef DEF_INT32_BINARY_WITH_OVERFLOW_NODE

template <class Derived, Operation kOperation>
class Int32BinaryNode : public FixedInputValueNodeT<2, Derived> {
  using Base = FixedInputValueNodeT<2, Derived>;

 public:
  static constexpr OpProperties kProperties =
      OpProperties::EagerDeopt() | OpProperties::Int32();

  static constexpr int kLeftIndex = 0;
  static constexpr int kRightIndex = 1;
  Input& left_input() { return Node::input(kLeftIndex); }
  Input& right_input() { return Node::input(kRightIndex); }

 protected:
  explicit Int32BinaryNode(uint64_t bitfield) : Base(bitfield) {}

  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}
};

#define DEF_OPERATION_NODE(Name, Super, OpName)                     \
  class Name : public Super<Name, Operation::k##OpName> {           \
    using Base = Super<Name, Operation::k##OpName>;                 \
                                                                    \
   public:                                                          \
    explicit Name(uint64_t bitfield) : Base(bitfield) {}            \
    void AllocateVreg(MaglevVregAllocationState*);                  \
    void GenerateCode(MaglevCodeGenState*, const ProcessingState&); \
    void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}  \
  };

#define DEF_INT32_BINARY_NODE(Name) \
  DEF_OPERATION_NODE(Int32##Name, Int32BinaryNode, Name)
DEF_INT32_BINARY_NODE(BitwiseAnd)
DEF_INT32_BINARY_NODE(BitwiseOr)
DEF_INT32_BINARY_NODE(BitwiseXor)
DEF_INT32_BINARY_NODE(ShiftLeft)
DEF_INT32_BINARY_NODE(ShiftRight)
DEF_INT32_BINARY_NODE(ShiftRightLogical)
#undef DEF_INT32_BINARY_NODE
// DEF_INT32_UNARY_WITH_OVERFLOW_NODE(Negate)
// DEF_INT32_UNARY_WITH_OVERFLOW_NODE(Increment)
// DEF_INT32_UNARY_WITH_OVERFLOW_NODE(Decrement)

template <class Derived, Operation kOperation>
class Int32CompareNode : public FixedInputValueNodeT<2, Derived> {
  using Base = FixedInputValueNodeT<2, Derived>;

 public:
  static constexpr int kLeftIndex = 0;
  static constexpr int kRightIndex = 1;
  Input& left_input() { return Node::input(kLeftIndex); }
  Input& right_input() { return Node::input(kRightIndex); }

 protected:
  explicit Int32CompareNode(uint64_t bitfield) : Base(bitfield) {}

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}
};

#define DEF_OPERATION_NODE(Name, Super, OpName)                     \
  class Name : public Super<Name, Operation::k##OpName> {           \
    using Base = Super<Name, Operation::k##OpName>;                 \
                                                                    \
   public:                                                          \
    explicit Name(uint64_t bitfield) : Base(bitfield) {}            \
    void AllocateVreg(MaglevVregAllocationState*);                  \
    void GenerateCode(MaglevCodeGenState*, const ProcessingState&); \
    void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}  \
  };

#define DEF_INT32_COMPARE_NODE(Name) \
  DEF_OPERATION_NODE(Int32##Name, Int32CompareNode, Name)
DEF_INT32_COMPARE_NODE(Equal)
DEF_INT32_COMPARE_NODE(StrictEqual)
DEF_INT32_COMPARE_NODE(LessThan)
DEF_INT32_COMPARE_NODE(LessThanOrEqual)
DEF_INT32_COMPARE_NODE(GreaterThan)
DEF_INT32_COMPARE_NODE(GreaterThanOrEqual)
#undef DEF_INT32_COMPARE_NODE

#undef DEF_OPERATION_NODE

template <class Derived, Operation kOperation>
class Float64BinaryNode : public FixedInputValueNodeT<2, Derived> {
  using Base = FixedInputValueNodeT<2, Derived>;

 public:
  static constexpr OpProperties kProperties = OpProperties::Float64();

  static constexpr int kLeftIndex = 0;
  static constexpr int kRightIndex = 1;
  Input& left_input() { return Node::input(kLeftIndex); }
  Input& right_input() { return Node::input(kRightIndex); }

 protected:
  explicit Float64BinaryNode(uint64_t bitfield) : Base(bitfield) {}

  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}
};

#define DEF_OPERATION_NODE(Name, Super, OpName)                     \
  class Name : public Super<Name, Operation::k##OpName> {           \
    using Base = Super<Name, Operation::k##OpName>;                 \
                                                                    \
   public:                                                          \
    explicit Name(uint64_t bitfield) : Base(bitfield) {}            \
    void AllocateVreg(MaglevVregAllocationState*);                  \
    void GenerateCode(MaglevCodeGenState*, const ProcessingState&); \
    void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}  \
  };

#define DEF_FLOAT64_BINARY_NODE(Name) \
  DEF_OPERATION_NODE(Float64##Name, Float64BinaryNode, Name)
DEF_FLOAT64_BINARY_NODE(Add)
DEF_FLOAT64_BINARY_NODE(Subtract)
DEF_FLOAT64_BINARY_NODE(Multiply)
DEF_FLOAT64_BINARY_NODE(Divide)
// DEF_FLOAT64_BINARY_NODE(Modulus)
// DEF_FLOAT64_BINARY_NODE(Exponentiate)
// DEF_FLOAT64_BINARY_NODE(Equal)
// DEF_FLOAT64_BINARY_NODE(StrictEqual)
// DEF_FLOAT64_BINARY_NODE(LessThan)
// DEF_FLOAT64_BINARY_NODE(LessThanOrEqual)
// DEF_FLOAT64_BINARY_NODE(GreaterThan)
// DEF_FLOAT64_BINARY_NODE(GreaterThanOrEqual)
#undef DEF_FLOAT64_BINARY_NODE

template <class Derived, Operation kOperation>
class Float64CompareNode : public FixedInputValueNodeT<2, Derived> {
  using Base = FixedInputValueNodeT<2, Derived>;

 public:
  static constexpr int kLeftIndex = 0;
  static constexpr int kRightIndex = 1;
  Input& left_input() { return Node::input(kLeftIndex); }
  Input& right_input() { return Node::input(kRightIndex); }

 protected:
  explicit Float64CompareNode(uint64_t bitfield) : Base(bitfield) {}

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}
};

#define DEF_OPERATION_NODE(Name, Super, OpName)                     \
  class Name : public Super<Name, Operation::k##OpName> {           \
    using Base = Super<Name, Operation::k##OpName>;                 \
                                                                    \
   public:                                                          \
    explicit Name(uint64_t bitfield) : Base(bitfield) {}            \
    void AllocateVreg(MaglevVregAllocationState*);                  \
    void GenerateCode(MaglevCodeGenState*, const ProcessingState&); \
    void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}  \
  };

#define DEF_FLOAT64_COMPARE_NODE(Name) \
  DEF_OPERATION_NODE(Float64##Name, Float64CompareNode, Name)
DEF_FLOAT64_COMPARE_NODE(Equal)
DEF_FLOAT64_COMPARE_NODE(StrictEqual)
DEF_FLOAT64_COMPARE_NODE(LessThan)
DEF_FLOAT64_COMPARE_NODE(LessThanOrEqual)
DEF_FLOAT64_COMPARE_NODE(GreaterThan)
DEF_FLOAT64_COMPARE_NODE(GreaterThanOrEqual)
#undef DEF_FLOAT64_COMPARE_NODE

#undef DEF_OPERATION_NODE

class CheckedSmiTag : public FixedInputValueNodeT<1, CheckedSmiTag> {
  using Base = FixedInputValueNodeT<1, CheckedSmiTag>;

 public:
  explicit CheckedSmiTag(uint64_t bitfield) : Base(bitfield) {}

  static constexpr OpProperties kProperties =
      OpProperties::EagerDeopt() | OpProperties::ConversionNode();

  Input& input() { return Node::input(0); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}
};

class CheckedSmiUntag : public FixedInputValueNodeT<1, CheckedSmiUntag> {
  using Base = FixedInputValueNodeT<1, CheckedSmiUntag>;

 public:
  explicit CheckedSmiUntag(uint64_t bitfield) : Base(bitfield) {}

  static constexpr OpProperties kProperties = OpProperties::EagerDeopt() |
                                              OpProperties::Int32() |
                                              OpProperties::ConversionNode();

  Input& input() { return Node::input(0); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}
};

class Int32Constant : public FixedInputValueNodeT<0, Int32Constant> {
  using Base = FixedInputValueNodeT<0, Int32Constant>;

 public:
  using OutputRegister = Register;

  explicit Int32Constant(uint64_t bitfield, int32_t value)
      : Base(bitfield), value_(value) {}

  static constexpr OpProperties kProperties = OpProperties::Int32();

  int32_t value() const { return value_; }

  bool ToBoolean(LocalIsolate* local_isolate) const { return value_ != 0; }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;

  void DoLoadToRegister(MaglevCodeGenState*, OutputRegister);
  Handle<Object> DoReify(LocalIsolate* isolate);

 private:
  const int32_t value_;
};

class Float64Constant : public FixedInputValueNodeT<0, Float64Constant> {
  using Base = FixedInputValueNodeT<0, Float64Constant>;

 public:
  using OutputRegister = DoubleRegister;

  explicit Float64Constant(uint64_t bitfield, double value)
      : Base(bitfield), value_(value) {}

  static constexpr OpProperties kProperties = OpProperties::Float64();

  double value() const { return value_; }

  bool ToBoolean(LocalIsolate* local_isolate) const {
    return value_ != 0.0 && !std::isnan(value_);
  }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;

  void DoLoadToRegister(MaglevCodeGenState*, OutputRegister);
  Handle<Object> DoReify(LocalIsolate* isolate);

 private:
  const double value_;
};

class Float64Box : public FixedInputValueNodeT<1, Float64Box> {
  using Base = FixedInputValueNodeT<1, Float64Box>;

 public:
  explicit Float64Box(uint64_t bitfield) : Base(bitfield) {}

  static constexpr OpProperties kProperties =
      OpProperties::DeferredCall() | OpProperties::ConversionNode();

  Input& input() { return Node::input(0); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}
};

class ChangeInt32ToFloat64
    : public FixedInputValueNodeT<1, ChangeInt32ToFloat64> {
  using Base = FixedInputValueNodeT<1, ChangeInt32ToFloat64>;

 public:
  explicit ChangeInt32ToFloat64(uint64_t bitfield) : Base(bitfield) {}

  static constexpr OpProperties kProperties =
      OpProperties::Float64() | OpProperties::ConversionNode();

  Input& input() { return Node::input(0); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}
};

class CheckedFloat64Unbox
    : public FixedInputValueNodeT<1, CheckedFloat64Unbox> {
  using Base = FixedInputValueNodeT<1, CheckedFloat64Unbox>;

 public:
  explicit CheckedFloat64Unbox(uint64_t bitfield) : Base(bitfield) {}

  static constexpr OpProperties kProperties = OpProperties::EagerDeopt() |
                                              OpProperties::Float64() |
                                              OpProperties::ConversionNode();

  Input& input() { return Node::input(0); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}
};

class LogicalNot : public FixedInputValueNodeT<1, LogicalNot> {
  using Base = FixedInputValueNodeT<1, LogicalNot>;

 public:
  explicit LogicalNot(uint64_t bitfield) : Base(bitfield) {}

  Input& value() { return Node::input(0); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}
};

class SetPendingMessage : public FixedInputValueNodeT<1, SetPendingMessage> {
  using Base = FixedInputValueNodeT<1, SetPendingMessage>;

 public:
  explicit SetPendingMessage(uint64_t bitfield) : Base(bitfield) {}

  Input& value() { return Node::input(0); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}
};

class ToBooleanLogicalNot
    : public FixedInputValueNodeT<1, ToBooleanLogicalNot> {
  using Base = FixedInputValueNodeT<1, ToBooleanLogicalNot>;

 public:
  explicit ToBooleanLogicalNot(uint64_t bitfield) : Base(bitfield) {}

  Input& value() { return Node::input(0); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}
};

class TaggedEqual : public FixedInputValueNodeT<2, TaggedEqual> {
  using Base = FixedInputValueNodeT<2, TaggedEqual>;

 public:
  explicit TaggedEqual(uint64_t bitfield) : Base(bitfield) {}

  Input& lhs() { return Node::input(0); }
  Input& rhs() { return Node::input(1); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}
};

class TaggedNotEqual : public FixedInputValueNodeT<2, TaggedNotEqual> {
  using Base = FixedInputValueNodeT<2, TaggedNotEqual>;

 public:
  explicit TaggedNotEqual(uint64_t bitfield) : Base(bitfield) {}

  Input& lhs() { return Node::input(0); }
  Input& rhs() { return Node::input(1); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}
};

class TestInstanceOf : public FixedInputValueNodeT<3, TestInstanceOf> {
  using Base = FixedInputValueNodeT<3, TestInstanceOf>;

 public:
  explicit TestInstanceOf(uint64_t bitfield) : Base(bitfield) {}

  // The implementation currently calls runtime.
  static constexpr OpProperties kProperties = OpProperties::JSCall();

  Input& context() { return input(0); }
  Input& object() { return input(1); }
  Input& callable() { return input(2); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}
};

class TestUndetectable : public FixedInputValueNodeT<1, TestUndetectable> {
  using Base = FixedInputValueNodeT<1, TestUndetectable>;

 public:
  explicit TestUndetectable(uint64_t bitfield) : Base(bitfield) {}

  Input& value() { return Node::input(0); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}
};

class TestTypeOf : public FixedInputValueNodeT<1, TestTypeOf> {
  using Base = FixedInputValueNodeT<1, TestTypeOf>;

 public:
  explicit TestTypeOf(uint64_t bitfield,
                      interpreter::TestTypeOfFlags::LiteralFlag literal)
      : Base(bitfield), literal_(literal) {}

  Input& value() { return Node::input(0); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}

 private:
  interpreter::TestTypeOfFlags::LiteralFlag literal_;
};

class ToName : public FixedInputValueNodeT<2, ToName> {
  using Base = FixedInputValueNodeT<2, ToName>;

 public:
  explicit ToName(uint64_t bitfield) : Base(bitfield) {}

  // The implementation currently calls runtime.
  static constexpr OpProperties kProperties = OpProperties::JSCall();

  Input& context() { return Node::input(0); }
  Input& value_input() { return Node::input(1); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}
};

class ToNumberOrNumeric : public FixedInputValueNodeT<2, ToNumberOrNumeric> {
  using Base = FixedInputValueNodeT<2, ToNumberOrNumeric>;

 public:
  explicit ToNumberOrNumeric(uint64_t bitfield, Object::Conversion mode)
      : Base(bitfield), mode_(mode) {}

  // The implementation currently calls runtime.
  static constexpr OpProperties kProperties = OpProperties::JSCall();

  Input& context() { return Node::input(0); }
  Input& value_input() { return Node::input(1); }
  Object::Conversion mode() const { return mode_; }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}

 private:
  const Object::Conversion mode_;
};

class DeleteProperty : public FixedInputValueNodeT<3, DeleteProperty> {
  using Base = FixedInputValueNodeT<3, DeleteProperty>;

 public:
  explicit DeleteProperty(uint64_t bitfield, LanguageMode mode)
      : Base(bitfield), mode_(mode) {}

  // The implementation currently calls runtime.
  static constexpr OpProperties kProperties = OpProperties::JSCall();

  Input& context() { return Node::input(0); }
  Input& object() { return Node::input(1); }
  Input& key() { return Node::input(2); }

  LanguageMode mode() const { return mode_; }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;

 private:
  const LanguageMode mode_;
};

class ForInPrepare : public FixedInputValueNodeT<2, ForInPrepare> {
  using Base = FixedInputValueNodeT<2, ForInPrepare>;

 public:
  explicit ForInPrepare(uint64_t bitfield, compiler::FeedbackSource& feedback)
      : Base(bitfield), feedback_(feedback) {}

  static constexpr OpProperties kProperties = OpProperties::Call();

  compiler::FeedbackSource feedback() const { return feedback_; }

  Input& context() { return Node::input(0); }
  Input& enumerator() { return Node::input(1); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}

 private:
  const compiler::FeedbackSource feedback_;
};

class ForInNext : public FixedInputValueNodeT<5, ForInNext> {
  using Base = FixedInputValueNodeT<5, ForInNext>;

 public:
  explicit ForInNext(uint64_t bitfield, compiler::FeedbackSource& feedback)
      : Base(bitfield), feedback_(feedback) {}

  static constexpr OpProperties kProperties = OpProperties::JSCall();

  compiler::FeedbackSource feedback() const { return feedback_; }

  Input& context() { return Node::input(0); }
  Input& receiver() { return Node::input(1); }
  Input& cache_array() { return Node::input(2); }
  Input& cache_type() { return Node::input(3); }
  Input& cache_index() { return Node::input(4); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}

 private:
  const compiler::FeedbackSource feedback_;
};

class GetSecondReturnedValue
    : public FixedInputValueNodeT<0, GetSecondReturnedValue> {
  using Base = FixedInputValueNodeT<0, GetSecondReturnedValue>;

 public:
  explicit GetSecondReturnedValue(uint64_t bitfield) : Base(bitfield) {}

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}
};

class ToObject : public FixedInputValueNodeT<2, ToObject> {
  using Base = FixedInputValueNodeT<2, ToObject>;

 public:
  explicit ToObject(uint64_t bitfield) : Base(bitfield) {}

  // The implementation currently calls runtime.
  static constexpr OpProperties kProperties = OpProperties::JSCall();

  Input& context() { return Node::input(0); }
  Input& value_input() { return Node::input(1); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}
};

class ToString : public FixedInputValueNodeT<2, ToString> {
  using Base = FixedInputValueNodeT<2, ToString>;

 public:
  explicit ToString(uint64_t bitfield) : Base(bitfield) {}

  // The implementation currently calls runtime.
  static constexpr OpProperties kProperties = OpProperties::JSCall();

  Input& context() { return Node::input(0); }
  Input& value_input() { return Node::input(1); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}
};
class InitialValue : public FixedInputValueNodeT<0, InitialValue> {
  using Base = FixedInputValueNodeT<0, InitialValue>;

 public:
  explicit InitialValue(uint64_t bitfield, interpreter::Register source)
      : Base(bitfield), source_(source) {}

  interpreter::Register source() const { return source_; }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;

 private:
  const interpreter::Register source_;
};

class RegisterInput : public FixedInputValueNodeT<0, RegisterInput> {
  using Base = FixedInputValueNodeT<0, RegisterInput>;

 public:
  explicit RegisterInput(uint64_t bitfield, Register input)
      : Base(bitfield), input_(input) {}

  Register input() const { return input_; }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;

 private:
  const Register input_;
};

class SmiConstant : public FixedInputValueNodeT<0, SmiConstant> {
  using Base = FixedInputValueNodeT<0, SmiConstant>;

 public:
  using OutputRegister = Register;

  explicit SmiConstant(uint64_t bitfield, Smi value)
      : Base(bitfield), value_(value) {}

  Smi value() const { return value_; }

  bool ToBoolean(LocalIsolate* local_isolate) const {
    return value_ != Smi::FromInt(0);
  }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;

  void DoLoadToRegister(MaglevCodeGenState*, OutputRegister);
  Handle<Object> DoReify(LocalIsolate* isolate);

 private:
  const Smi value_;
};

class Constant : public FixedInputValueNodeT<0, Constant> {
  using Base = FixedInputValueNodeT<0, Constant>;

 public:
  using OutputRegister = Register;

  explicit Constant(uint64_t bitfield, const compiler::HeapObjectRef& object)
      : Base(bitfield), object_(object) {}

  bool ToBoolean(LocalIsolate* local_isolate) const {
    return object_.object()->BooleanValue(local_isolate);
  }

  bool IsTheHole() const { return object_.IsTheHole(); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;

  void DoLoadToRegister(MaglevCodeGenState*, OutputRegister);
  Handle<Object> DoReify(LocalIsolate* isolate);

 private:
  const compiler::HeapObjectRef object_;
};

class RootConstant : public FixedInputValueNodeT<0, RootConstant> {
  using Base = FixedInputValueNodeT<0, RootConstant>;

 public:
  using OutputRegister = Register;

  explicit RootConstant(uint64_t bitfield, RootIndex index)
      : Base(bitfield), index_(index) {}

  bool ToBoolean(LocalIsolate* local_isolate) const;

  RootIndex index() const { return index_; }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;

  void DoLoadToRegister(MaglevCodeGenState*, OutputRegister);
  Handle<Object> DoReify(LocalIsolate* isolate);

 private:
  const RootIndex index_;
};

class CreateEmptyArrayLiteral
    : public FixedInputValueNodeT<0, CreateEmptyArrayLiteral> {
  using Base = FixedInputValueNodeT<0, CreateEmptyArrayLiteral>;

 public:
  explicit CreateEmptyArrayLiteral(uint64_t bitfield,
                                   const compiler::FeedbackSource& feedback)
      : Base(bitfield), feedback_(feedback) {}

  compiler::FeedbackSource feedback() const { return feedback_; }

  // The implementation currently calls runtime.
  static constexpr OpProperties kProperties = OpProperties::Call();

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}

 private:
  const compiler::FeedbackSource feedback_;
};

class CreateArrayLiteral : public FixedInputValueNodeT<0, CreateArrayLiteral> {
  using Base = FixedInputValueNodeT<0, CreateArrayLiteral>;

 public:
  explicit CreateArrayLiteral(uint64_t bitfield,
                              const compiler::HeapObjectRef& constant_elements,
                              const compiler::FeedbackSource& feedback,
                              int flags)
      : Base(bitfield),
        constant_elements_(constant_elements),
        feedback_(feedback),
        flags_(flags) {}

  compiler::HeapObjectRef constant_elements() { return constant_elements_; }
  compiler::FeedbackSource feedback() const { return feedback_; }
  int flags() const { return flags_; }

  // The implementation currently calls runtime.
  static constexpr OpProperties kProperties = OpProperties::Call();

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}

 private:
  const compiler::HeapObjectRef constant_elements_;
  const compiler::FeedbackSource feedback_;
  const int flags_;
};

class CreateShallowArrayLiteral
    : public FixedInputValueNodeT<0, CreateShallowArrayLiteral> {
  using Base = FixedInputValueNodeT<0, CreateShallowArrayLiteral>;

 public:
  explicit CreateShallowArrayLiteral(
      uint64_t bitfield, const compiler::HeapObjectRef& constant_elements,
      const compiler::FeedbackSource& feedback, int flags)
      : Base(bitfield),
        constant_elements_(constant_elements),
        feedback_(feedback),
        flags_(flags) {}

  compiler::HeapObjectRef constant_elements() { return constant_elements_; }
  compiler::FeedbackSource feedback() const { return feedback_; }
  int flags() const { return flags_; }

  // The implementation currently calls runtime.
  static constexpr OpProperties kProperties = OpProperties::Call();

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}

 private:
  const compiler::HeapObjectRef constant_elements_;
  const compiler::FeedbackSource feedback_;
  const int flags_;
};

class CreateObjectLiteral
    : public FixedInputValueNodeT<0, CreateObjectLiteral> {
  using Base = FixedInputValueNodeT<0, CreateObjectLiteral>;

 public:
  explicit CreateObjectLiteral(
      uint64_t bitfield,
      const compiler::ObjectBoilerplateDescriptionRef& boilerplate_descriptor,
      const compiler::FeedbackSource& feedback, int flags)
      : Base(bitfield),
        boilerplate_descriptor_(boilerplate_descriptor),
        feedback_(feedback),
        flags_(flags) {}

  compiler::ObjectBoilerplateDescriptionRef boilerplate_descriptor() {
    return boilerplate_descriptor_;
  }
  compiler::FeedbackSource feedback() const { return feedback_; }
  int flags() const { return flags_; }

  // The implementation currently calls runtime.
  static constexpr OpProperties kProperties = OpProperties::Call();

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}

 private:
  const compiler::ObjectBoilerplateDescriptionRef boilerplate_descriptor_;
  const compiler::FeedbackSource feedback_;
  const int flags_;
};

class CreateEmptyObjectLiteral
    : public FixedInputValueNodeT<0, CreateEmptyObjectLiteral> {
  using Base = FixedInputValueNodeT<0, CreateEmptyObjectLiteral>;

 public:
  explicit CreateEmptyObjectLiteral(uint64_t bitfield, compiler::MapRef& map)
      : Base(bitfield), map_(map) {}

  static constexpr OpProperties kProperties = OpProperties::DeferredCall();

  compiler::MapRef map() { return map_; }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}

 private:
  const compiler::MapRef map_;
};

class CreateShallowObjectLiteral
    : public FixedInputValueNodeT<0, CreateShallowObjectLiteral> {
  using Base = FixedInputValueNodeT<0, CreateShallowObjectLiteral>;

 public:
  explicit CreateShallowObjectLiteral(
      uint64_t bitfield,
      const compiler::ObjectBoilerplateDescriptionRef& boilerplate_descriptor,
      const compiler::FeedbackSource& feedback, int flags)
      : Base(bitfield),
        boilerplate_descriptor_(boilerplate_descriptor),
        feedback_(feedback),
        flags_(flags) {}

  // TODO(victorgomes): We should not need a boilerplate descriptor in
  // CreateShallowObjectLiteral.
  compiler::ObjectBoilerplateDescriptionRef boilerplate_descriptor() {
    return boilerplate_descriptor_;
  }
  compiler::FeedbackSource feedback() const { return feedback_; }
  int flags() const { return flags_; }

  // The implementation currently calls runtime.
  static constexpr OpProperties kProperties = OpProperties::Call();

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}

 private:
  const compiler::ObjectBoilerplateDescriptionRef boilerplate_descriptor_;
  const compiler::FeedbackSource feedback_;
  const int flags_;
};

class CreateFunctionContext
    : public FixedInputValueNodeT<1, CreateFunctionContext> {
  using Base = FixedInputValueNodeT<1, CreateFunctionContext>;

 public:
  explicit CreateFunctionContext(uint64_t bitfield,
                                 compiler::ScopeInfoRef scope_info,
                                 uint32_t slot_count, ScopeType scope_type)
      : Base(bitfield),
        scope_info_(scope_info),
        slot_count_(slot_count),
        scope_type_(scope_type) {}

  compiler::ScopeInfoRef scope_info() const { return scope_info_; }
  uint32_t slot_count() const { return slot_count_; }
  ScopeType scope_type() const { return scope_type_; }

  Input& context() { return input(0); }

  // The implementation currently calls runtime.
  static constexpr OpProperties kProperties = OpProperties::Call();

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;

 private:
  const compiler::ScopeInfoRef scope_info_;
  const uint32_t slot_count_;
  ScopeType scope_type_;
};

class FastCreateClosure : public FixedInputValueNodeT<1, FastCreateClosure> {
  using Base = FixedInputValueNodeT<1, FastCreateClosure>;

 public:
  explicit FastCreateClosure(
      uint64_t bitfield, compiler::SharedFunctionInfoRef shared_function_info,
      compiler::FeedbackCellRef feedback_cell)
      : Base(bitfield),
        shared_function_info_(shared_function_info),
        feedback_cell_(feedback_cell) {}

  compiler::SharedFunctionInfoRef shared_function_info() const {
    return shared_function_info_;
  }
  compiler::FeedbackCellRef feedback_cell() const { return feedback_cell_; }

  Input& context() { return input(0); }

  // The implementation currently calls runtime.
  static constexpr OpProperties kProperties = OpProperties::Call();

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;

 private:
  const compiler::SharedFunctionInfoRef shared_function_info_;
  const compiler::FeedbackCellRef feedback_cell_;
};

class CreateRegExpLiteral
    : public FixedInputValueNodeT<0, CreateRegExpLiteral> {
  using Base = FixedInputValueNodeT<0, CreateRegExpLiteral>;

 public:
  explicit CreateRegExpLiteral(uint64_t bitfield,
                               const compiler::StringRef& pattern,
                               const compiler::FeedbackSource& feedback,
                               int flags)
      : Base(bitfield), pattern_(pattern), feedback_(feedback), flags_(flags) {}

  compiler::StringRef pattern() { return pattern_; }
  compiler::FeedbackSource feedback() const { return feedback_; }
  int flags() const { return flags_; }

  // The implementation currently calls runtime.
  static constexpr OpProperties kProperties = OpProperties::Call();

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}

 private:
  compiler::StringRef pattern_;
  const compiler::FeedbackSource feedback_;
  const int flags_;
};

class CreateClosure : public FixedInputValueNodeT<1, CreateClosure> {
  using Base = FixedInputValueNodeT<1, CreateClosure>;

 public:
  explicit CreateClosure(uint64_t bitfield,
                         compiler::SharedFunctionInfoRef shared_function_info,
                         compiler::FeedbackCellRef feedback_cell,
                         bool pretenured)
      : Base(bitfield),
        shared_function_info_(shared_function_info),
        feedback_cell_(feedback_cell),
        pretenured_(pretenured) {}

  compiler::SharedFunctionInfoRef shared_function_info() const {
    return shared_function_info_;
  }
  compiler::FeedbackCellRef feedback_cell() const { return feedback_cell_; }
  bool pretenured() const { return pretenured_; }

  Input& context() { return input(0); }

  // The implementation currently calls runtime.
  static constexpr OpProperties kProperties = OpProperties::Call();

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;

 private:
  const compiler::SharedFunctionInfoRef shared_function_info_;
  const compiler::FeedbackCellRef feedback_cell_;
  const bool pretenured_;
};

class CheckMaps : public FixedInputNodeT<1, CheckMaps> {
  using Base = FixedInputNodeT<1, CheckMaps>;

 public:
  explicit CheckMaps(uint64_t bitfield, const compiler::MapRef& map)
      : Base(bitfield), map_(map) {
    DCHECK(!map.is_migration_target());
  }

  static constexpr OpProperties kProperties = OpProperties::EagerDeopt();

  compiler::MapRef map() const { return map_; }

  static constexpr int kReceiverIndex = 0;
  Input& receiver_input() { return input(kReceiverIndex); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;

 private:
  const compiler::MapRef map_;
};
class CheckSmi : public FixedInputNodeT<1, CheckSmi> {
  using Base = FixedInputNodeT<1, CheckSmi>;

 public:
  explicit CheckSmi(uint64_t bitfield) : Base(bitfield) {}

  static constexpr OpProperties kProperties = OpProperties::EagerDeopt();

  static constexpr int kReceiverIndex = 0;
  Input& receiver_input() { return input(kReceiverIndex); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;
};

class CheckNumber : public FixedInputNodeT<1, CheckNumber> {
  using Base = FixedInputNodeT<1, CheckNumber>;

 public:
  explicit CheckNumber(uint64_t bitfield, Object::Conversion mode)
      : Base(bitfield), mode_(mode) {}

  static constexpr OpProperties kProperties = OpProperties::EagerDeopt();

  static constexpr int kReceiverIndex = 0;
  Input& receiver_input() { return input(kReceiverIndex); }
  Object::Conversion mode() const { return mode_; }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}

 private:
  const Object::Conversion mode_;
};

class CheckHeapObject : public FixedInputNodeT<1, CheckHeapObject> {
  using Base = FixedInputNodeT<1, CheckHeapObject>;

 public:
  explicit CheckHeapObject(uint64_t bitfield) : Base(bitfield) {}

  static constexpr OpProperties kProperties = OpProperties::EagerDeopt();

  static constexpr int kReceiverIndex = 0;
  Input& receiver_input() { return input(kReceiverIndex); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;
};

class CheckSymbol : public FixedInputNodeT<1, CheckSymbol> {
  using Base = FixedInputNodeT<1, CheckSymbol>;

 public:
  explicit CheckSymbol(uint64_t bitfield) : Base(bitfield) {}

  static constexpr OpProperties kProperties = OpProperties::EagerDeopt();

  static constexpr int kReceiverIndex = 0;
  Input& receiver_input() { return input(kReceiverIndex); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;
};

class CheckString : public FixedInputNodeT<1, CheckString> {
  using Base = FixedInputNodeT<1, CheckString>;

 public:
  explicit CheckString(uint64_t bitfield) : Base(bitfield) {}

  static constexpr OpProperties kProperties = OpProperties::EagerDeopt();

  static constexpr int kReceiverIndex = 0;
  Input& receiver_input() { return input(kReceiverIndex); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;
};

class CheckMapsWithMigration
    : public FixedInputNodeT<1, CheckMapsWithMigration> {
  using Base = FixedInputNodeT<1, CheckMapsWithMigration>;

 public:
  explicit CheckMapsWithMigration(uint64_t bitfield,
                                  const compiler::MapRef& map)
      : Base(bitfield), map_(map) {
    DCHECK(map.is_migration_target());
  }

  static constexpr OpProperties kProperties =
      OpProperties::EagerDeopt() | OpProperties::DeferredCall();

  compiler::MapRef map() const { return map_; }

  static constexpr int kReceiverIndex = 0;
  Input& receiver_input() { return input(kReceiverIndex); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;

 private:
  const compiler::MapRef map_;
};

class CheckedInternalizedString
    : public FixedInputValueNodeT<1, CheckedInternalizedString> {
  using Base = FixedInputValueNodeT<1, CheckedInternalizedString>;

 public:
  explicit CheckedInternalizedString(uint64_t bitfield) : Base(bitfield) {
    CHECK_EQ(properties().value_representation(), ValueRepresentation::kTagged);
  }

  static constexpr OpProperties kProperties = OpProperties::EagerDeopt() |
                                              OpProperties::TaggedValue() |
                                              OpProperties::ConversionNode();

  static constexpr int kObjectIndex = 0;
  Input& object_input() { return Node::input(kObjectIndex); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}
};

class GetTemplateObject : public FixedInputValueNodeT<1, GetTemplateObject> {
  using Base = FixedInputValueNodeT<1, GetTemplateObject>;

 public:
  explicit GetTemplateObject(
      uint64_t bitfield,
      const compiler::SharedFunctionInfoRef& shared_function_info,
      const compiler::FeedbackSource& feedback)
      : Base(bitfield),
        shared_function_info_(shared_function_info),
        feedback_(feedback) {}

  // The implementation currently calls runtime.
  static constexpr OpProperties kProperties = OpProperties::Call();

  Input& description() { return input(0); }

  compiler::SharedFunctionInfoRef shared_function_info() {
    return shared_function_info_;
  }
  compiler::FeedbackSource feedback() const { return feedback_; }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}

 private:
  compiler::SharedFunctionInfoRef shared_function_info_;
  const compiler::FeedbackSource feedback_;
};

class LoadTaggedField : public FixedInputValueNodeT<1, LoadTaggedField> {
  using Base = FixedInputValueNodeT<1, LoadTaggedField>;

 public:
  explicit LoadTaggedField(uint64_t bitfield, int offset)
      : Base(bitfield), offset_(offset) {}

  static constexpr OpProperties kProperties = OpProperties::Reading();

  int offset() const { return offset_; }

  static constexpr int kObjectIndex = 0;
  Input& object_input() { return input(kObjectIndex); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;

 private:
  const int offset_;
};

class LoadDoubleField : public FixedInputValueNodeT<1, LoadDoubleField> {
  using Base = FixedInputValueNodeT<1, LoadDoubleField>;

 public:
  explicit LoadDoubleField(uint64_t bitfield, int offset)
      : Base(bitfield), offset_(offset) {}

  static constexpr OpProperties kProperties =
      OpProperties::Reading() | OpProperties::Float64();

  int offset() const { return offset_; }

  static constexpr int kObjectIndex = 0;
  Input& object_input() { return input(kObjectIndex); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;

 private:
  const int offset_;
};

class StoreTaggedFieldNoWriteBarrier
    : public FixedInputNodeT<2, StoreTaggedFieldNoWriteBarrier> {
  using Base = FixedInputNodeT<2, StoreTaggedFieldNoWriteBarrier>;

 public:
  explicit StoreTaggedFieldNoWriteBarrier(uint64_t bitfield, int offset)
      : Base(bitfield), offset_(offset) {}

  static constexpr OpProperties kProperties = OpProperties::Writing();

  int offset() const { return offset_; }

  static constexpr int kObjectIndex = 0;
  static constexpr int kValueIndex = 1;
  Input& object_input() { return input(kObjectIndex); }
  Input& value_input() { return input(kValueIndex); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;

 private:
  const int offset_;
};

class StoreTaggedFieldWithWriteBarrier
    : public FixedInputNodeT<2, StoreTaggedFieldWithWriteBarrier> {
  using Base = FixedInputNodeT<2, StoreTaggedFieldWithWriteBarrier>;

 public:
  explicit StoreTaggedFieldWithWriteBarrier(uint64_t bitfield, int offset)
      : Base(bitfield), offset_(offset) {}

  static constexpr OpProperties kProperties =
      OpProperties::Writing() | OpProperties::DeferredCall();

  int offset() const { return offset_; }

  static constexpr int kObjectIndex = 0;
  static constexpr int kValueIndex = 1;
  Input& object_input() { return input(kObjectIndex); }
  Input& value_input() { return input(kValueIndex); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;

 private:
  const int offset_;
};

class LoadGlobal : public FixedInputValueNodeT<1, LoadGlobal> {
  using Base = FixedInputValueNodeT<1, LoadGlobal>;

 public:
  explicit LoadGlobal(uint64_t bitfield, const compiler::NameRef& name,
                      const compiler::FeedbackSource& feedback)
      : Base(bitfield), name_(name), feedback_(feedback) {}

  // The implementation currently calls runtime.
  static constexpr OpProperties kProperties = OpProperties::JSCall();

  const compiler::NameRef& name() const { return name_; }
  compiler::FeedbackSource feedback() const { return feedback_; }

  Input& context() { return input(0); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;

 private:
  const compiler::NameRef name_;
  const compiler::FeedbackSource feedback_;
};

class StoreGlobal : public FixedInputValueNodeT<2, StoreGlobal> {
  using Base = FixedInputValueNodeT<2, StoreGlobal>;

 public:
  explicit StoreGlobal(uint64_t bitfield, const compiler::NameRef& name,
                       const compiler::FeedbackSource& feedback)
      : Base(bitfield), name_(name), feedback_(feedback) {}

  // The implementation currently calls runtime.
  static constexpr OpProperties kProperties = OpProperties::JSCall();

  const compiler::NameRef& name() const { return name_; }
  compiler::FeedbackSource feedback() const { return feedback_; }

  Input& context() { return input(0); }
  Input& value() { return input(1); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;

 private:
  const compiler::NameRef name_;
  const compiler::FeedbackSource feedback_;
};

class LoadNamedGeneric : public FixedInputValueNodeT<2, LoadNamedGeneric> {
  using Base = FixedInputValueNodeT<2, LoadNamedGeneric>;

 public:
  explicit LoadNamedGeneric(uint64_t bitfield, const compiler::NameRef& name,
                            const compiler::FeedbackSource& feedback)
      : Base(bitfield), name_(name), feedback_(feedback) {}

  // The implementation currently calls runtime.
  static constexpr OpProperties kProperties = OpProperties::JSCall();

  compiler::NameRef name() const { return name_; }
  compiler::FeedbackSource feedback() const { return feedback_; }

  static constexpr int kContextIndex = 0;
  static constexpr int kObjectIndex = 1;
  Input& context() { return input(kContextIndex); }
  Input& object_input() { return input(kObjectIndex); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;

 private:
  const compiler::NameRef name_;
  const compiler::FeedbackSource feedback_;
};

class LoadNamedFromSuperGeneric
    : public FixedInputValueNodeT<3, LoadNamedFromSuperGeneric> {
  using Base = FixedInputValueNodeT<3, LoadNamedFromSuperGeneric>;

 public:
  explicit LoadNamedFromSuperGeneric(uint64_t bitfield,
                                     const compiler::NameRef& name,
                                     const compiler::FeedbackSource& feedback)
      : Base(bitfield), name_(name), feedback_(feedback) {}

  // The implementation currently calls runtime.
  static constexpr OpProperties kProperties = OpProperties::JSCall();

  compiler::NameRef name() const { return name_; }
  compiler::FeedbackSource feedback() const { return feedback_; }

  static constexpr int kContextIndex = 0;
  static constexpr int kReceiverIndex = 1;
  static constexpr int kLookupStartObjectIndex = 2;
  Input& context() { return input(kContextIndex); }
  Input& receiver() { return input(kReceiverIndex); }
  Input& lookup_start_object() { return input(kLookupStartObjectIndex); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;

 private:
  const compiler::NameRef name_;
  const compiler::FeedbackSource feedback_;
};

class SetNamedGeneric : public FixedInputValueNodeT<3, SetNamedGeneric> {
  using Base = FixedInputValueNodeT<3, SetNamedGeneric>;

 public:
  explicit SetNamedGeneric(uint64_t bitfield, const compiler::NameRef& name,
                           const compiler::FeedbackSource& feedback)
      : Base(bitfield), name_(name), feedback_(feedback) {}

  // The implementation currently calls runtime.
  static constexpr OpProperties kProperties = OpProperties::JSCall();

  compiler::NameRef name() const { return name_; }
  compiler::FeedbackSource feedback() const { return feedback_; }

  static constexpr int kContextIndex = 0;
  static constexpr int kObjectIndex = 1;
  static constexpr int kValueIndex = 2;
  Input& context() { return input(kContextIndex); }
  Input& object_input() { return input(kObjectIndex); }
  Input& value_input() { return input(kValueIndex); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;

 private:
  const compiler::NameRef name_;
  const compiler::FeedbackSource feedback_;
};

class DefineNamedOwnGeneric
    : public FixedInputValueNodeT<3, DefineNamedOwnGeneric> {
  using Base = FixedInputValueNodeT<3, DefineNamedOwnGeneric>;

 public:
  explicit DefineNamedOwnGeneric(uint64_t bitfield,
                                 const compiler::NameRef& name,
                                 const compiler::FeedbackSource& feedback)
      : Base(bitfield), name_(name), feedback_(feedback) {}

  // The implementation currently calls runtime.
  static constexpr OpProperties kProperties = OpProperties::JSCall();

  compiler::NameRef name() const { return name_; }
  compiler::FeedbackSource feedback() const { return feedback_; }

  static constexpr int kContextIndex = 0;
  static constexpr int kObjectIndex = 1;
  static constexpr int kValueIndex = 2;
  Input& context() { return input(kContextIndex); }
  Input& object_input() { return input(kObjectIndex); }
  Input& value_input() { return input(kValueIndex); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;

 private:
  const compiler::NameRef name_;
  const compiler::FeedbackSource feedback_;
};

class StoreInArrayLiteralGeneric
    : public FixedInputValueNodeT<4, StoreInArrayLiteralGeneric> {
  using Base = FixedInputValueNodeT<4, StoreInArrayLiteralGeneric>;

 public:
  explicit StoreInArrayLiteralGeneric(uint64_t bitfield,
                                      const compiler::FeedbackSource& feedback)
      : Base(bitfield), feedback_(feedback) {}

  // The implementation currently calls runtime.
  static constexpr OpProperties kProperties = OpProperties::JSCall();

  compiler::FeedbackSource feedback() const { return feedback_; }

  static constexpr int kContextIndex = 0;
  static constexpr int kObjectIndex = 1;
  static constexpr int kNameIndex = 2;
  static constexpr int kValueIndex = 3;
  Input& context() { return input(kContextIndex); }
  Input& object_input() { return input(kObjectIndex); }
  Input& name_input() { return input(kNameIndex); }
  Input& value_input() { return input(kValueIndex); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}

 private:
  const compiler::FeedbackSource feedback_;
};

class GetKeyedGeneric : public FixedInputValueNodeT<3, GetKeyedGeneric> {
  using Base = FixedInputValueNodeT<3, GetKeyedGeneric>;

 public:
  explicit GetKeyedGeneric(uint64_t bitfield,
                           const compiler::FeedbackSource& feedback)
      : Base(bitfield), feedback_(feedback) {}

  // The implementation currently calls runtime.
  static constexpr OpProperties kProperties = OpProperties::JSCall();

  compiler::FeedbackSource feedback() const { return feedback_; }

  static constexpr int kContextIndex = 0;
  static constexpr int kObjectIndex = 1;
  static constexpr int kKeyIndex = 2;
  Input& context() { return input(kContextIndex); }
  Input& object_input() { return input(kObjectIndex); }
  Input& key_input() { return input(kKeyIndex); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}

 private:
  const compiler::FeedbackSource feedback_;
};

class SetKeyedGeneric : public FixedInputValueNodeT<4, SetKeyedGeneric> {
  using Base = FixedInputValueNodeT<4, SetKeyedGeneric>;

 public:
  explicit SetKeyedGeneric(uint64_t bitfield,
                           const compiler::FeedbackSource& feedback)
      : Base(bitfield), feedback_(feedback) {}

  // The implementation currently calls runtime.
  static constexpr OpProperties kProperties = OpProperties::JSCall();

  compiler::FeedbackSource feedback() const { return feedback_; }

  static constexpr int kContextIndex = 0;
  static constexpr int kObjectIndex = 1;
  static constexpr int kKeyIndex = 2;
  static constexpr int kValueIndex = 3;
  Input& context() { return input(kContextIndex); }
  Input& object_input() { return input(kObjectIndex); }
  Input& key_input() { return input(kKeyIndex); }
  Input& value_input() { return input(kValueIndex); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}

 private:
  const compiler::FeedbackSource feedback_;
};

class DefineKeyedOwnGeneric
    : public FixedInputValueNodeT<4, DefineKeyedOwnGeneric> {
  using Base = FixedInputValueNodeT<4, DefineKeyedOwnGeneric>;

 public:
  explicit DefineKeyedOwnGeneric(uint64_t bitfield,
                                 const compiler::FeedbackSource& feedback)
      : Base(bitfield), feedback_(feedback) {}

  // The implementation currently calls runtime.
  static constexpr OpProperties kProperties = OpProperties::JSCall();

  compiler::FeedbackSource feedback() const { return feedback_; }

  static constexpr int kContextIndex = 0;
  static constexpr int kObjectIndex = 1;
  static constexpr int kKeyIndex = 2;
  static constexpr int kValueIndex = 3;
  Input& context() { return input(kContextIndex); }
  Input& object_input() { return input(kObjectIndex); }
  Input& key_input() { return input(kKeyIndex); }
  Input& value_input() { return input(kValueIndex); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}

 private:
  const compiler::FeedbackSource feedback_;
};

class GapMove : public FixedInputNodeT<0, GapMove> {
  using Base = FixedInputNodeT<0, GapMove>;

 public:
  GapMove(uint64_t bitfield, compiler::AllocatedOperand source,
          compiler::AllocatedOperand target)
      : Base(bitfield), source_(source), target_(target) {}

  compiler::AllocatedOperand source() const { return source_; }
  compiler::AllocatedOperand target() const { return target_; }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;

 private:
  compiler::AllocatedOperand source_;
  compiler::AllocatedOperand target_;
};

class ConstantGapMove : public FixedInputNodeT<0, ConstantGapMove> {
  using Base = FixedInputNodeT<0, ConstantGapMove>;

 public:
  ConstantGapMove(uint64_t bitfield, ValueNode* node,
                  compiler::AllocatedOperand target)
      : Base(bitfield), node_(node), target_(target) {}

  compiler::AllocatedOperand target() const { return target_; }
  ValueNode* node() const { return node_; }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;

 private:
  ValueNode* node_;
  compiler::InstructionOperand source_;
  compiler::AllocatedOperand target_;
};

// TODO(verwaest): It may make more sense to buffer phis in merged_states until
// we set up the interpreter frame state for code generation. At that point we
// can generate correctly-sized phis.
class Phi : public ValueNodeT<Phi> {
  using Base = ValueNodeT<Phi>;

 public:
  using List = base::ThreadedList<Phi>;

  // TODO(jgruber): More intuitive constructors, if possible.
  Phi(uint64_t bitfield, interpreter::Register owner, int merge_offset)
      : Base(bitfield), owner_(owner), merge_offset_(merge_offset) {}

  interpreter::Register owner() const { return owner_; }
  int merge_offset() const { return merge_offset_; }

  using Node::reduce_input_count;
  using Node::set_input;

  void AllocateVreg(MaglevVregAllocationState*);
  void AllocateVregInPostProcess(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;

 private:
  Phi** next() { return &next_; }

  const interpreter::Register owner_;
  Phi* next_ = nullptr;
  const int merge_offset_;
  friend List;
  friend base::ThreadedListTraits<Phi>;
};

class Call : public ValueNodeT<Call> {
  using Base = ValueNodeT<Call>;

 public:
  // We assume function and context as fixed inputs.
  static constexpr int kFunctionIndex = 0;
  static constexpr int kContextIndex = 1;
  static constexpr int kFixedInputCount = 2;

  // We need enough inputs to have these fixed inputs plus the maximum arguments
  // to a function call.
  static_assert(kMaxInputs >= kFixedInputCount + Code::kMaxArguments);

  // This ctor is used when for variable input counts.
  // Inputs must be initialized manually.
  Call(uint64_t bitfield, ConvertReceiverMode mode, ValueNode* function,
       ValueNode* context)
      : Base(bitfield), receiver_mode_(mode) {
    set_input(kFunctionIndex, function);
    set_input(kContextIndex, context);
  }

  static constexpr OpProperties kProperties = OpProperties::JSCall();

  Input& function() { return input(kFunctionIndex); }
  const Input& function() const { return input(kFunctionIndex); }
  Input& context() { return input(kContextIndex); }
  const Input& context() const { return input(kContextIndex); }
  int num_args() const { return input_count() - kFixedInputCount; }
  Input& arg(int i) { return input(i + kFixedInputCount); }
  void set_arg(int i, ValueNode* node) {
    set_input(i + kFixedInputCount, node);
  }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}

 private:
  ConvertReceiverMode receiver_mode_;
};

class Construct : public ValueNodeT<Construct> {
  using Base = ValueNodeT<Construct>;

 public:
  // We assume function and context as fixed inputs.
  static constexpr int kFunctionIndex = 0;
  static constexpr int kNewTargetIndex = 1;
  static constexpr int kContextIndex = 2;
  static constexpr int kFixedInputCount = 3;

  // We need enough inputs to have these fixed inputs plus the maximum arguments
  // to a function call.
  static_assert(kMaxInputs >= kFixedInputCount + Code::kMaxArguments);

  // This ctor is used when for variable input counts.
  // Inputs must be initialized manually.
  Construct(uint64_t bitfield, ValueNode* function, ValueNode* new_target,
            ValueNode* context)
      : Base(bitfield) {
    set_input(kFunctionIndex, function);
    set_input(kNewTargetIndex, new_target);
    set_input(kContextIndex, context);
  }

  static constexpr OpProperties kProperties = OpProperties::JSCall();

  Input& function() { return input(kFunctionIndex); }
  const Input& function() const { return input(kFunctionIndex); }
  Input& new_target() { return input(kNewTargetIndex); }
  const Input& new_target() const { return input(kNewTargetIndex); }
  Input& context() { return input(kContextIndex); }
  const Input& context() const { return input(kContextIndex); }
  int num_args() const { return input_count() - kFixedInputCount; }
  Input& arg(int i) { return input(i + kFixedInputCount); }
  void set_arg(int i, ValueNode* node) {
    set_input(i + kFixedInputCount, node);
  }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}
};

class CallBuiltin : public ValueNodeT<CallBuiltin> {
  using Base = ValueNodeT<CallBuiltin>;

 public:
  enum FeedbackSlotType { kTaggedIndex, kSmi };

  // This ctor is used when for variable input counts.
  // Inputs must be initialized manually.
  CallBuiltin(uint64_t bitfield, Builtin builtin)
      : Base(bitfield), builtin_(builtin) {
    DCHECK(
        !Builtins::CallInterfaceDescriptorFor(builtin).HasContextParameter());
  }

  // This ctor is used when for variable input counts.
  // Inputs must be initialized manually.
  CallBuiltin(uint64_t bitfield, Builtin builtin, ValueNode* context)
      : Base(bitfield), builtin_(builtin) {
    DCHECK(Builtins::CallInterfaceDescriptorFor(builtin).HasContextParameter());
    // We use the last valid input for the context.
    set_input(input_count() - 1, context);
  }

  // This is an overestimation, since some builtins might not call JS code.
  static constexpr OpProperties kProperties = OpProperties::JSCall();

  bool has_feedback() const { return feedback_.has_value(); }
  compiler::FeedbackSource feedback() const {
    DCHECK(has_feedback());
    return feedback_.value();
  }
  FeedbackSlotType slot_type() const {
    DCHECK(has_feedback());
    return slot_type_;
  }
  void set_feedback(compiler::FeedbackSource& feedback,
                    FeedbackSlotType slot_type) {
    feedback_ = feedback;
    slot_type_ = slot_type;
  }

  Builtin builtin() const { return builtin_; }

  int num_args(bool has_context) const {
    DCHECK_EQ(
        has_context,
        Builtins::CallInterfaceDescriptorFor(builtin_).HasContextParameter());
    int extra_input_count = has_context ? 1 : 0;
    return input_count() - extra_input_count;
  }
  void set_arg(int i, ValueNode* node) { set_input(i, node); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;

 private:
  Builtin builtin_;
  base::Optional<compiler::FeedbackSource> feedback_;
  FeedbackSlotType slot_type_ = kTaggedIndex;
};

class CallRuntime : public ValueNodeT<CallRuntime> {
  using Base = ValueNodeT<CallRuntime>;

 public:
  // We assume the context as fixed input.
  static constexpr int kContextIndex = 0;
  static constexpr int kFixedInputCount = 1;

  // This ctor is used when for variable input counts.
  // Inputs must be initialized manually.
  CallRuntime(uint64_t bitfield, Runtime::FunctionId function_id,
              ValueNode* context)
      : Base(bitfield), function_id_(function_id) {
    set_input(kContextIndex, context);
  }

  static constexpr OpProperties kProperties = OpProperties::JSCall();

  Runtime::FunctionId function_id() const { return function_id_; }

  Input& context() { return input(kContextIndex); }
  const Input& context() const { return input(kContextIndex); }
  int num_args() const { return input_count() - kFixedInputCount; }
  Input& arg(int i) { return input(i + kFixedInputCount); }
  void set_arg(int i, ValueNode* node) {
    set_input(i + kFixedInputCount, node);
  }

  int ReturnCount() {
    return Runtime::FunctionForId(function_id())->result_size;
  }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;

 private:
  Runtime::FunctionId function_id_;
};

class CallWithSpread : public ValueNodeT<CallWithSpread> {
  using Base = ValueNodeT<CallWithSpread>;

 public:
  // We assume function and context as fixed inputs.
  static constexpr int kFunctionIndex = 0;
  static constexpr int kContextIndex = 1;
  static constexpr int kFixedInputCount = 2;

  // This ctor is used when for variable input counts.
  // Inputs must be initialized manually.
  CallWithSpread(uint64_t bitfield, ValueNode* function, ValueNode* context)
      : Base(bitfield) {
    set_input(kFunctionIndex, function);
    set_input(kContextIndex, context);
  }

  static constexpr OpProperties kProperties = OpProperties::JSCall();

  Input& function() { return input(kFunctionIndex); }
  const Input& function() const { return input(kFunctionIndex); }
  Input& context() { return input(kContextIndex); }
  const Input& context() const { return input(kContextIndex); }
  int num_args() const { return input_count() - kFixedInputCount; }
  Input& arg(int i) { return input(i + kFixedInputCount); }
  void set_arg(int i, ValueNode* node) {
    set_input(i + kFixedInputCount, node);
  }
  Input& spread() {
    // Spread is the last argument/input.
    return input(input_count() - 1);
  }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}
};

class ConstructWithSpread : public ValueNodeT<ConstructWithSpread> {
  using Base = ValueNodeT<ConstructWithSpread>;

 public:
  // We assume function and context as fixed inputs.
  static constexpr int kFunctionIndex = 0;
  static constexpr int kNewTargetIndex = 1;
  static constexpr int kContextIndex = 2;
  static constexpr int kFixedInputCount = 3;

  // This ctor is used when for variable input counts.
  // Inputs must be initialized manually.
  ConstructWithSpread(uint64_t bitfield, ValueNode* function,
                      ValueNode* new_target, ValueNode* context)
      : Base(bitfield) {
    set_input(kFunctionIndex, function);
    set_input(kNewTargetIndex, new_target);
    set_input(kContextIndex, context);
  }

  static constexpr OpProperties kProperties = OpProperties::JSCall();

  Input& function() { return input(kFunctionIndex); }
  const Input& function() const { return input(kFunctionIndex); }
  Input& new_target() { return input(kNewTargetIndex); }
  const Input& new_target() const { return input(kNewTargetIndex); }
  Input& context() { return input(kContextIndex); }
  const Input& context() const { return input(kContextIndex); }
  int num_args() const { return input_count() - kFixedInputCount; }
  Input& arg(int i) { return input(i + kFixedInputCount); }
  void set_arg(int i, ValueNode* node) {
    set_input(i + kFixedInputCount, node);
  }
  Input& spread() {
    // Spread is the last argument/input.
    return input(input_count() - 1);
  }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}
};

class IncreaseInterruptBudget
    : public FixedInputNodeT<0, IncreaseInterruptBudget> {
  using Base = FixedInputNodeT<0, IncreaseInterruptBudget>;

 public:
  explicit IncreaseInterruptBudget(uint64_t bitfield, int amount)
      : Base(bitfield), amount_(amount) {
    DCHECK_GT(amount, 0);
  }

  int amount() const { return amount_; }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;

 private:
  const int amount_;
};

class ReduceInterruptBudget : public FixedInputNodeT<0, ReduceInterruptBudget> {
  using Base = FixedInputNodeT<0, ReduceInterruptBudget>;

 public:
  explicit ReduceInterruptBudget(uint64_t bitfield, int amount)
      : Base(bitfield), amount_(amount) {
    DCHECK_GT(amount, 0);
  }

  static constexpr OpProperties kProperties = OpProperties::DeferredCall();

  int amount() const { return amount_; }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;

 private:
  const int amount_;
};

class ThrowReferenceErrorIfHole
    : public FixedInputNodeT<1, ThrowReferenceErrorIfHole> {
  using Base = FixedInputNodeT<1, ThrowReferenceErrorIfHole>;

 public:
  explicit ThrowReferenceErrorIfHole(uint64_t bitfield,
                                     const compiler::NameRef& name)
      : Base(bitfield), name_(name) {}

  static constexpr OpProperties kProperties =
      OpProperties::LazyDeopt() | OpProperties::DeferredCall();

  const compiler::NameRef& name() const { return name_; }

  Input& value() { return Node::input(0); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}

 private:
  const compiler::NameRef name_;
};

class ThrowSuperNotCalledIfHole
    : public FixedInputNodeT<1, ThrowSuperNotCalledIfHole> {
  using Base = FixedInputNodeT<1, ThrowSuperNotCalledIfHole>;

 public:
  explicit ThrowSuperNotCalledIfHole(uint64_t bitfield) : Base(bitfield) {}

  static constexpr OpProperties kProperties =
      OpProperties::LazyDeopt() | OpProperties::DeferredCall();

  Input& value() { return Node::input(0); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}
};

class ThrowSuperAlreadyCalledIfNotHole
    : public FixedInputNodeT<1, ThrowSuperAlreadyCalledIfNotHole> {
  using Base = FixedInputNodeT<1, ThrowSuperAlreadyCalledIfNotHole>;

 public:
  explicit ThrowSuperAlreadyCalledIfNotHole(uint64_t bitfield)
      : Base(bitfield) {}

  static constexpr OpProperties kProperties =
      OpProperties::LazyDeopt() | OpProperties::DeferredCall();

  Input& value() { return Node::input(0); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}
};

class ThrowIfNotSuperConstructor
    : public FixedInputNodeT<2, ThrowIfNotSuperConstructor> {
  using Base = FixedInputNodeT<2, ThrowIfNotSuperConstructor>;

 public:
  explicit ThrowIfNotSuperConstructor(uint64_t bitfield) : Base(bitfield) {}

  static constexpr OpProperties kProperties =
      OpProperties::LazyDeopt() | OpProperties::DeferredCall();

  Input& constructor() { return Node::input(0); }
  Input& function() { return Node::input(1); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}
};

// Represents either a direct BasicBlock pointer, or an entry in a list of
// unresolved BasicBlockRefs which will be mutated (in place) at some point into
// direct BasicBlock pointers.
class BasicBlockRef {
  struct BasicBlockRefBuilder;

 public:
  BasicBlockRef() : next_ref_(nullptr) {
#ifdef DEBUG
    state_ = kRefList;
#endif
  }
  explicit BasicBlockRef(BasicBlock* block) : block_ptr_(block) {
#ifdef DEBUG
    state_ = kBlockPointer;
#endif
  }

  // Refs can't be copied or moved, since they are referenced by `this` pointer
  // in the ref list.
  BasicBlockRef(const BasicBlockRef&) = delete;
  BasicBlockRef(BasicBlockRef&&) = delete;
  BasicBlockRef& operator=(const BasicBlockRef&) = delete;
  BasicBlockRef& operator=(BasicBlockRef&&) = delete;

  // Construct a new ref-list mode BasicBlockRef and add it to the given ref
  // list.
  explicit BasicBlockRef(BasicBlockRef* ref_list_head) : BasicBlockRef() {
    BasicBlockRef* old_next_ptr = MoveToRefList(ref_list_head);
    USE(old_next_ptr);
    DCHECK_NULL(old_next_ptr);
  }

  // Change this ref to a direct basic block pointer, returning the old "next"
  // pointer of the current ref.
  BasicBlockRef* SetToBlockAndReturnNext(BasicBlock* block) {
    DCHECK_EQ(state_, kRefList);

    BasicBlockRef* old_next_ptr = next_ref_;
    block_ptr_ = block;
#ifdef DEBUG
    state_ = kBlockPointer;
#endif
    return old_next_ptr;
  }

  // Reset this ref list to null, returning the old ref list (i.e. the old
  // "next" pointer).
  BasicBlockRef* Reset() {
    DCHECK_EQ(state_, kRefList);

    BasicBlockRef* old_next_ptr = next_ref_;
    next_ref_ = nullptr;
    return old_next_ptr;
  }

  // Move this ref to the given ref list, returning the old "next" pointer of
  // the current ref.
  BasicBlockRef* MoveToRefList(BasicBlockRef* ref_list_head) {
    DCHECK_EQ(state_, kRefList);
    DCHECK_EQ(ref_list_head->state_, kRefList);

    BasicBlockRef* old_next_ptr = next_ref_;
    next_ref_ = ref_list_head->next_ref_;
    ref_list_head->next_ref_ = this;
    return old_next_ptr;
  }

  BasicBlock* block_ptr() const {
    DCHECK_EQ(state_, kBlockPointer);
    return block_ptr_;
  }

  BasicBlockRef* next_ref() const {
    DCHECK_EQ(state_, kRefList);
    return next_ref_;
  }

  bool has_ref() const {
    DCHECK_EQ(state_, kRefList);
    return next_ref_ != nullptr;
  }

 private:
  union {
    BasicBlock* block_ptr_;
    BasicBlockRef* next_ref_;
  };
#ifdef DEBUG
  enum { kBlockPointer, kRefList } state_;
#endif  // DEBUG
};

class ControlNode : public NodeBase {
 public:
  // A "hole" in control flow is a control node that unconditionally interrupts
  // linear control flow (either by jumping or by exiting).
  //
  // A "post-dominating" hole is a hole that is guaranteed to be be reached in
  // control flow after this node (i.e. it is a hole that is a post-dominator
  // of this node).
  ControlNode* next_post_dominating_hole() const {
    return next_post_dominating_hole_;
  }
  void set_next_post_dominating_hole(ControlNode* node) {
    DCHECK_IMPLIES(node != nullptr,
                   node->Is<UnconditionalControlNode>() || node->Is<Abort>() ||
                       node->Is<Return>() || node->Is<Deopt>());
    next_post_dominating_hole_ = node;
  }

 protected:
  using NodeBase::NodeBase;

 private:
  ControlNode* next_post_dominating_hole_ = nullptr;
};

class UnconditionalControlNode : public ControlNode {
 public:
  BasicBlock* target() const { return target_.block_ptr(); }
  int predecessor_id() const { return predecessor_id_; }
  void set_predecessor_id(int id) { predecessor_id_ = id; }

 protected:
  explicit UnconditionalControlNode(uint64_t bitfield,
                                    BasicBlockRef* target_refs)
      : ControlNode(bitfield), target_(target_refs) {}
  explicit UnconditionalControlNode(uint64_t bitfield, BasicBlock* target)
      : ControlNode(bitfield), target_(target) {}

 private:
  const BasicBlockRef target_;
  int predecessor_id_ = 0;
};

template <class Derived>
class UnconditionalControlNodeT : public UnconditionalControlNode {
  static_assert(IsUnconditionalControlNode(opcode_of<Derived>));
  static constexpr size_t kInputCount = 0;

 public:
  // Shadowing for static knowledge.
  constexpr Opcode opcode() const { return NodeBase::opcode_of<Derived>; }
  constexpr bool has_inputs() const { return input_count() > 0; }
  constexpr uint16_t input_count() const { return kInputCount; }
  auto end() {
    return std::make_reverse_iterator(&this->input(input_count() - 1));
  }

 protected:
  explicit UnconditionalControlNodeT(uint64_t bitfield,
                                     BasicBlockRef* target_refs)
      : UnconditionalControlNode(bitfield, target_refs) {
    DCHECK_EQ(NodeBase::opcode(), opcode_of<Derived>);
    DCHECK_EQ(NodeBase::input_count(), kInputCount);
  }
  explicit UnconditionalControlNodeT(uint64_t bitfield, BasicBlock* target)
      : UnconditionalControlNode(bitfield, target) {
    DCHECK_EQ(NodeBase::opcode(), opcode_of<Derived>);
    DCHECK_EQ(NodeBase::input_count(), kInputCount);
  }
};

class ConditionalControlNode : public ControlNode {
 public:
  ConditionalControlNode(uint64_t bitfield) : ControlNode(bitfield) {}
};

class BranchControlNode : public ConditionalControlNode {
 public:
  BranchControlNode(uint64_t bitfield, BasicBlockRef* if_true_refs,
                    BasicBlockRef* if_false_refs)
      : ConditionalControlNode(bitfield),
        if_true_(if_true_refs),
        if_false_(if_false_refs) {}

  BasicBlock* if_true() const { return if_true_.block_ptr(); }
  BasicBlock* if_false() const { return if_false_.block_ptr(); }

 private:
  BasicBlockRef if_true_;
  BasicBlockRef if_false_;
};

template <size_t InputCount, class Derived>
class BranchControlNodeT : public BranchControlNode {
  static_assert(IsBranchControlNode(opcode_of<Derived>));
  static constexpr size_t kInputCount = InputCount;

 public:
  // Shadowing for static knowledge.
  constexpr Opcode opcode() const { return NodeBase::opcode_of<Derived>; }
  constexpr bool has_inputs() const { return input_count() > 0; }
  constexpr uint16_t input_count() const { return kInputCount; }
  auto end() {
    return std::make_reverse_iterator(&this->input(input_count() - 1));
  }

 protected:
  explicit BranchControlNodeT(uint64_t bitfield, BasicBlockRef* if_true_refs,
                              BasicBlockRef* if_false_refs)
      : BranchControlNode(bitfield, if_true_refs, if_false_refs) {
    DCHECK_EQ(NodeBase::opcode(), opcode_of<Derived>);
    DCHECK_EQ(NodeBase::input_count(), kInputCount);
  }
};

class Jump : public UnconditionalControlNodeT<Jump> {
  using Base = UnconditionalControlNodeT<Jump>;

 public:
  Jump(uint64_t bitfield, BasicBlockRef* target_refs)
      : Base(bitfield, target_refs) {}

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}
};

class JumpLoop : public UnconditionalControlNodeT<JumpLoop> {
  using Base = UnconditionalControlNodeT<JumpLoop>;

 public:
  explicit JumpLoop(uint64_t bitfield, BasicBlock* target, int32_t loop_depth,
                    FeedbackSlot feedback_slot)
      : Base(bitfield, target),
        loop_depth_(loop_depth),
        feedback_slot_(feedback_slot) {}

  explicit JumpLoop(uint64_t bitfield, BasicBlockRef* ref, int32_t loop_depth,
                    FeedbackSlot feedback_slot)
      : Base(bitfield, ref),
        loop_depth_(loop_depth),
        feedback_slot_(feedback_slot) {}

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}

  base::Vector<Input> used_nodes() { return used_node_locations_; }
  void set_used_nodes(base::Vector<Input> locations) {
    used_node_locations_ = locations;
  }

 private:
  // For OSR.
  const int32_t loop_depth_;
  const FeedbackSlot feedback_slot_;
  base::Vector<Input> used_node_locations_;
};

class JumpToInlined : public UnconditionalControlNodeT<JumpToInlined> {
  using Base = UnconditionalControlNodeT<JumpToInlined>;

 public:
  explicit JumpToInlined(uint64_t bitfield, BasicBlockRef* target_refs,
                         MaglevCompilationUnit* unit)
      : Base(bitfield, target_refs), unit_(unit) {}

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;

  const MaglevCompilationUnit* unit() const { return unit_; }

 private:
  MaglevCompilationUnit* unit_;
};

class JumpFromInlined : public UnconditionalControlNodeT<JumpFromInlined> {
  using Base = UnconditionalControlNodeT<JumpFromInlined>;

 public:
  explicit JumpFromInlined(uint64_t bitfield, BasicBlockRef* target_refs)
      : Base(bitfield, target_refs) {}

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}
};

class Abort : public ControlNode {
 public:
  explicit Abort(uint64_t bitfield, AbortReason reason)
      : ControlNode(bitfield), reason_(reason) {
    DCHECK_EQ(NodeBase::opcode(), opcode_of<Abort>);
  }

  AbortReason reason() const { return reason_; }

  void AllocateVreg(MaglevVregAllocationState*) {}
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;

 private:
  const AbortReason reason_;
};

class Return : public ControlNode {
 public:
  explicit Return(uint64_t bitfield) : ControlNode(bitfield) {
    DCHECK_EQ(NodeBase::opcode(), opcode_of<Return>);
  }

  Input& value_input() { return input(0); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}
};

class Deopt : public ControlNode {
 public:
  explicit Deopt(uint64_t bitfield, DeoptimizeReason reason)
      : ControlNode(bitfield), reason_(reason) {
    DCHECK_EQ(NodeBase::opcode(), opcode_of<Deopt>);
  }

  static constexpr OpProperties kProperties = OpProperties::EagerDeopt();

  DeoptimizeReason reason() const { return reason_; }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;

 private:
  DeoptimizeReason reason_;
};

class Switch : public ConditionalControlNode {
 public:
  explicit Switch(uint64_t bitfield, int value_base, BasicBlockRef* targets,
                  int size, BasicBlockRef* fallthrough)
      : ConditionalControlNode(bitfield),
        value_base_(value_base),
        targets_(targets),
        size_(size),
        fallthrough_(fallthrough) {}

  int value_base() const { return value_base_; }
  const BasicBlockRef* targets() const { return targets_; }
  int size() const { return size_; }

  BasicBlock* fallthrough() const { return fallthrough_.block_ptr(); }

  Input& value() { return input(0); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}

 private:
  const int value_base_;
  const BasicBlockRef* targets_;
  const int size_;
  BasicBlockRef fallthrough_;
};

class BranchIfRootConstant
    : public BranchControlNodeT<1, BranchIfRootConstant> {
  using Base = BranchControlNodeT<1, BranchIfRootConstant>;

 public:
  explicit BranchIfRootConstant(uint64_t bitfield, BasicBlockRef* if_true_refs,
                                BasicBlockRef* if_false_refs,
                                RootIndex root_index)
      : Base(bitfield, if_true_refs, if_false_refs), root_index_(root_index) {}

  RootIndex root_index() { return root_index_; }
  Input& condition_input() { return input(0); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}

 private:
  RootIndex root_index_;
};

class BranchIfUndefinedOrNull
    : public BranchControlNodeT<1, BranchIfUndefinedOrNull> {
  using Base = BranchControlNodeT<1, BranchIfUndefinedOrNull>;

 public:
  explicit BranchIfUndefinedOrNull(uint64_t bitfield,
                                   BasicBlockRef* if_true_refs,
                                   BasicBlockRef* if_false_refs)
      : Base(bitfield, if_true_refs, if_false_refs) {}

  Input& condition_input() { return input(0); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}
};

class BranchIfJSReceiver : public BranchControlNodeT<1, BranchIfJSReceiver> {
  using Base = BranchControlNodeT<1, BranchIfJSReceiver>;

 public:
  explicit BranchIfJSReceiver(uint64_t bitfield, BasicBlockRef* if_true_refs,
                              BasicBlockRef* if_false_refs)
      : Base(bitfield, if_true_refs, if_false_refs) {}

  Input& condition_input() { return input(0); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}
};

class BranchIfToBooleanTrue
    : public BranchControlNodeT<1, BranchIfToBooleanTrue> {
  using Base = BranchControlNodeT<1, BranchIfToBooleanTrue>;

 public:
  explicit BranchIfToBooleanTrue(uint64_t bitfield, BasicBlockRef* if_true_refs,
                                 BasicBlockRef* if_false_refs)
      : Base(bitfield, if_true_refs, if_false_refs) {}

  static constexpr OpProperties kProperties = OpProperties::Call();

  Input& condition_input() { return input(0); }

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const {}
};

class BranchIfInt32Compare
    : public BranchControlNodeT<2, BranchIfInt32Compare> {
  using Base = BranchControlNodeT<2, BranchIfInt32Compare>;

 public:
  static constexpr int kLeftIndex = 0;
  static constexpr int kRightIndex = 1;
  Input& left_input() { return NodeBase::input(kLeftIndex); }
  Input& right_input() { return NodeBase::input(kRightIndex); }

  explicit BranchIfInt32Compare(uint64_t bitfield, Operation operation,
                                BasicBlockRef* if_true_refs,
                                BasicBlockRef* if_false_refs)
      : Base(bitfield, if_true_refs, if_false_refs), operation_(operation) {}

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;

 private:
  Operation operation_;
};

class BranchIfFloat64Compare
    : public BranchControlNodeT<2, BranchIfFloat64Compare> {
  using Base = BranchControlNodeT<2, BranchIfFloat64Compare>;

 public:
  static constexpr int kLeftIndex = 0;
  static constexpr int kRightIndex = 1;
  Input& left_input() { return NodeBase::input(kLeftIndex); }
  Input& right_input() { return NodeBase::input(kRightIndex); }

  explicit BranchIfFloat64Compare(uint64_t bitfield, Operation operation,
                                  BasicBlockRef* if_true_refs,
                                  BasicBlockRef* if_false_refs)
      : Base(bitfield, if_true_refs, if_false_refs), operation_(operation) {}

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;

 private:
  Operation operation_;
};

class BranchIfReferenceCompare
    : public BranchControlNodeT<2, BranchIfReferenceCompare> {
  using Base = BranchControlNodeT<2, BranchIfReferenceCompare>;

 public:
  static constexpr int kLeftIndex = 0;
  static constexpr int kRightIndex = 1;
  Input& left_input() { return NodeBase::input(kLeftIndex); }
  Input& right_input() { return NodeBase::input(kRightIndex); }

  explicit BranchIfReferenceCompare(uint64_t bitfield, Operation operation,
                                    BasicBlockRef* if_true_refs,
                                    BasicBlockRef* if_false_refs)
      : Base(bitfield, if_true_refs, if_false_refs), operation_(operation) {}

  void AllocateVreg(MaglevVregAllocationState*);
  void GenerateCode(MaglevCodeGenState*, const ProcessingState&);
  void PrintParams(std::ostream&, MaglevGraphLabeller*) const;

 private:
  Operation operation_;
};

}  // namespace maglev
}  // namespace internal
}  // namespace v8

#endif  // V8_MAGLEV_MAGLEV_IR_H_
