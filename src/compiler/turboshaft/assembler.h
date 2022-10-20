// Copyright 2022 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_COMPILER_TURBOSHAFT_ASSEMBLER_H_
#define V8_COMPILER_TURBOSHAFT_ASSEMBLER_H_

#include <cstring>
#include <iterator>
#include <limits>
#include <memory>
#include <type_traits>

#include "src/base/logging.h"
#include "src/base/macros.h"
#include "src/base/small-vector.h"
#include "src/base/template-utils.h"
#include "src/codegen/reloc-info.h"
#include "src/compiler/turboshaft/graph.h"
#include "src/compiler/turboshaft/operation-matching.h"
#include "src/compiler/turboshaft/operations.h"
#include "src/compiler/turboshaft/representations.h"

namespace v8::internal::compiler::turboshaft {

// This class is used to extend an assembler with useful short-hands that still
// forward to the regular operations of the deriving assembler.
template <class Subclass, class Superclass>
class AssemblerInterface : public Superclass {
 public:
  using Superclass::Superclass;
  using Base = Superclass;

#define DECL_MULTI_REP_BINOP(name, operation, rep_type, kind)               \
  OpIndex name(OpIndex left, OpIndex right, rep_type rep) {                 \
    return subclass().Reduce##operation(left, right,                        \
                                        operation##Op::Kind::k##kind, rep); \
  }
#define DECL_SINGLE_REP_BINOP(name, operation, kind, rep)                   \
  OpIndex name(OpIndex left, OpIndex right) {                               \
    return subclass().Reduce##operation(left, right,                        \
                                        operation##Op::Kind::k##kind, rep); \
  }
#define DECL_SINGLE_REP_BINOP_NO_KIND(name, operation, rep) \
  OpIndex name(OpIndex left, OpIndex right) {               \
    return subclass().Reduce##operation(left, right, rep);  \
  }
  DECL_MULTI_REP_BINOP(WordAdd, WordBinop, WordRepresentation, Add)
  DECL_SINGLE_REP_BINOP(Word32Add, WordBinop, Add, WordRepresentation::Word32())
  DECL_SINGLE_REP_BINOP(Word64Add, WordBinop, Add, WordRepresentation::Word64())

  DECL_MULTI_REP_BINOP(WordMul, WordBinop, WordRepresentation, Mul)
  DECL_SINGLE_REP_BINOP(Word32Mul, WordBinop, Mul, WordRepresentation::Word32())
  DECL_SINGLE_REP_BINOP(Word64Mul, WordBinop, Mul, WordRepresentation::Word64())

  DECL_MULTI_REP_BINOP(WordBitwiseAnd, WordBinop, WordRepresentation,
                       BitwiseAnd)
  DECL_SINGLE_REP_BINOP(Word32BitwiseAnd, WordBinop, BitwiseAnd,
                        WordRepresentation::Word32())
  DECL_SINGLE_REP_BINOP(Word64BitwiseAnd, WordBinop, BitwiseAnd,
                        WordRepresentation::Word64())

  DECL_MULTI_REP_BINOP(WordBitwiseOr, WordBinop, WordRepresentation, BitwiseOr)
  DECL_SINGLE_REP_BINOP(Word32BitwiseOr, WordBinop, BitwiseOr,
                        WordRepresentation::Word32())
  DECL_SINGLE_REP_BINOP(Word64BitwiseOr, WordBinop, BitwiseOr,
                        WordRepresentation::Word64())

  DECL_MULTI_REP_BINOP(WordBitwiseXor, WordBinop, WordRepresentation,
                       BitwiseXor)
  DECL_SINGLE_REP_BINOP(Word32BitwiseXor, WordBinop, BitwiseXor,
                        WordRepresentation::Word32())
  DECL_SINGLE_REP_BINOP(Word64BitwiseXor, WordBinop, BitwiseXor,
                        WordRepresentation::Word64())

  DECL_MULTI_REP_BINOP(WordSub, WordBinop, WordRepresentation, Sub)
  DECL_SINGLE_REP_BINOP(Word32Sub, WordBinop, Sub, WordRepresentation::Word32())
  DECL_SINGLE_REP_BINOP(Word64Sub, WordBinop, Sub, WordRepresentation::Word64())

  DECL_MULTI_REP_BINOP(IntDiv, WordBinop, WordRepresentation, SignedDiv)
  DECL_SINGLE_REP_BINOP(Int32Div, WordBinop, SignedDiv,
                        WordRepresentation::Word32())
  DECL_SINGLE_REP_BINOP(Int64Div, WordBinop, SignedDiv,
                        WordRepresentation::Word64())
  DECL_MULTI_REP_BINOP(UintDiv, WordBinop, WordRepresentation, UnsignedDiv)
  DECL_SINGLE_REP_BINOP(Uint32Div, WordBinop, UnsignedDiv,
                        WordRepresentation::Word32())
  DECL_SINGLE_REP_BINOP(Uint64Div, WordBinop, UnsignedDiv,
                        WordRepresentation::Word64())
  DECL_MULTI_REP_BINOP(IntMod, WordBinop, WordRepresentation, SignedMod)
  DECL_SINGLE_REP_BINOP(Int32Mod, WordBinop, SignedMod,
                        WordRepresentation::Word32())
  DECL_SINGLE_REP_BINOP(Int64Mod, WordBinop, SignedMod,
                        WordRepresentation::Word64())
  DECL_MULTI_REP_BINOP(UintMod, WordBinop, WordRepresentation, UnsignedMod)
  DECL_SINGLE_REP_BINOP(Uint32Mod, WordBinop, UnsignedMod,
                        WordRepresentation::Word32())
  DECL_SINGLE_REP_BINOP(Uint64Mod, WordBinop, UnsignedMod,
                        WordRepresentation::Word64())
  DECL_MULTI_REP_BINOP(IntMulOverflownBits, WordBinop, WordRepresentation,
                       SignedMulOverflownBits)
  DECL_SINGLE_REP_BINOP(Int32MulOverflownBits, WordBinop,
                        SignedMulOverflownBits, WordRepresentation::Word32())
  DECL_SINGLE_REP_BINOP(Int64MulOverflownBits, WordBinop,
                        SignedMulOverflownBits, WordRepresentation::Word64())
  DECL_MULTI_REP_BINOP(UintMulOverflownBits, WordBinop, WordRepresentation,
                       UnsignedMulOverflownBits)
  DECL_SINGLE_REP_BINOP(Uint32MulOverflownBits, WordBinop,
                        UnsignedMulOverflownBits, WordRepresentation::Word32())
  DECL_SINGLE_REP_BINOP(Uint64MulOverflownBits, WordBinop,
                        UnsignedMulOverflownBits, WordRepresentation::Word64())

  DECL_MULTI_REP_BINOP(IntAddCheckOverflow, OverflowCheckedBinop,
                       WordRepresentation, SignedAdd)
  DECL_SINGLE_REP_BINOP(Int32AddCheckOverflow, OverflowCheckedBinop, SignedAdd,
                        WordRepresentation::Word32())
  DECL_SINGLE_REP_BINOP(Int64AddCheckOverflow, OverflowCheckedBinop, SignedAdd,
                        WordRepresentation::Word64())
  DECL_MULTI_REP_BINOP(IntSubCheckOverflow, OverflowCheckedBinop,
                       WordRepresentation, SignedSub)
  DECL_SINGLE_REP_BINOP(Int32SubCheckOverflow, OverflowCheckedBinop, SignedSub,
                        WordRepresentation::Word32())
  DECL_SINGLE_REP_BINOP(Int64SubCheckOverflow, OverflowCheckedBinop, SignedSub,
                        WordRepresentation::Word64())
  DECL_MULTI_REP_BINOP(IntMulCheckOverflow, OverflowCheckedBinop,
                       WordRepresentation, SignedMul)
  DECL_SINGLE_REP_BINOP(Int32MulCheckOverflow, OverflowCheckedBinop, SignedMul,
                        WordRepresentation::Word32())
  DECL_SINGLE_REP_BINOP(Int64MulCheckOverflow, OverflowCheckedBinop, SignedMul,
                        WordRepresentation::Word64())

  DECL_MULTI_REP_BINOP(FloatAdd, FloatBinop, FloatRepresentation, Add)
  DECL_SINGLE_REP_BINOP(Float32Add, FloatBinop, Add,
                        FloatRepresentation::Float32())
  DECL_SINGLE_REP_BINOP(Float64Add, FloatBinop, Add,
                        FloatRepresentation::Float64())
  DECL_MULTI_REP_BINOP(FloatMul, FloatBinop, FloatRepresentation, Mul)
  DECL_SINGLE_REP_BINOP(Float32Mul, FloatBinop, Mul,
                        FloatRepresentation::Float32())
  DECL_SINGLE_REP_BINOP(Float64Mul, FloatBinop, Mul,
                        FloatRepresentation::Float64())
  DECL_MULTI_REP_BINOP(FloatSub, FloatBinop, FloatRepresentation, Sub)
  DECL_SINGLE_REP_BINOP(Float32Sub, FloatBinop, Sub,
                        FloatRepresentation::Float32())
  DECL_SINGLE_REP_BINOP(Float64Sub, FloatBinop, Sub,
                        FloatRepresentation::Float64())
  DECL_MULTI_REP_BINOP(FloatDiv, FloatBinop, FloatRepresentation, Div)
  DECL_SINGLE_REP_BINOP(Float32Div, FloatBinop, Div,
                        FloatRepresentation::Float32())
  DECL_SINGLE_REP_BINOP(Float64Div, FloatBinop, Div,
                        FloatRepresentation::Float64())
  DECL_MULTI_REP_BINOP(FloatMin, FloatBinop, FloatRepresentation, Min)
  DECL_SINGLE_REP_BINOP(Float32Min, FloatBinop, Min,
                        FloatRepresentation::Float32())
  DECL_SINGLE_REP_BINOP(Float64Min, FloatBinop, Min,
                        FloatRepresentation::Float64())
  DECL_MULTI_REP_BINOP(FloatMax, FloatBinop, FloatRepresentation, Max)
  DECL_SINGLE_REP_BINOP(Float32Max, FloatBinop, Max,
                        FloatRepresentation::Float32())
  DECL_SINGLE_REP_BINOP(Float64Max, FloatBinop, Max,
                        FloatRepresentation::Float64())
  DECL_SINGLE_REP_BINOP(Float64Mod, FloatBinop, Mod,
                        FloatRepresentation::Float64())
  DECL_SINGLE_REP_BINOP(Float64Power, FloatBinop, Power,
                        FloatRepresentation::Float64())
  DECL_SINGLE_REP_BINOP(Float64Atan2, FloatBinop, Atan2,
                        FloatRepresentation::Float64())

  OpIndex Shift(OpIndex left, OpIndex right, ShiftOp::Kind kind,
                WordRepresentation rep) {
    return subclass().ReduceShift(left, right, kind, rep);
  }

  DECL_MULTI_REP_BINOP(ShiftRightArithmeticShiftOutZeros, Shift,
                       WordRepresentation, ShiftRightArithmeticShiftOutZeros)
  DECL_SINGLE_REP_BINOP(Word32ShiftRightArithmeticShiftOutZeros, Shift,
                        ShiftRightArithmeticShiftOutZeros,
                        WordRepresentation::Word32())
  DECL_SINGLE_REP_BINOP(Word64ShiftRightArithmeticShiftOutZeros, Shift,
                        ShiftRightArithmeticShiftOutZeros,
                        WordRepresentation::Word64())
  DECL_MULTI_REP_BINOP(ShiftRightArithmetic, Shift, WordRepresentation,
                       ShiftRightArithmetic)
  DECL_SINGLE_REP_BINOP(Word32ShiftRightArithmetic, Shift, ShiftRightArithmetic,
                        WordRepresentation::Word32())
  DECL_SINGLE_REP_BINOP(Word64ShiftRightArithmetic, Shift, ShiftRightArithmetic,
                        WordRepresentation::Word64())
  DECL_MULTI_REP_BINOP(ShiftRightLogical, Shift, WordRepresentation,
                       ShiftRightLogical)
  DECL_SINGLE_REP_BINOP(Word32ShiftRightLogical, Shift, ShiftRightLogical,
                        WordRepresentation::Word32())
  DECL_SINGLE_REP_BINOP(Word64ShiftRightLogical, Shift, ShiftRightLogical,
                        WordRepresentation::Word64())
  DECL_MULTI_REP_BINOP(ShiftLeft, Shift, WordRepresentation, ShiftLeft)
  DECL_SINGLE_REP_BINOP(Word32ShiftLeft, Shift, ShiftLeft,
                        WordRepresentation::Word32())
  DECL_SINGLE_REP_BINOP(Word64ShiftLeft, Shift, ShiftLeft,
                        WordRepresentation::Word64())
  DECL_MULTI_REP_BINOP(RotateRight, Shift, WordRepresentation, RotateRight)
  DECL_SINGLE_REP_BINOP(Word32RotateRight, Shift, RotateRight,
                        WordRepresentation::Word32())
  DECL_SINGLE_REP_BINOP(Word64RotateRight, Shift, RotateRight,
                        WordRepresentation::Word64())
  DECL_MULTI_REP_BINOP(RotateLeft, Shift, WordRepresentation, RotateLeft)
  DECL_SINGLE_REP_BINOP(Word32RotateLeft, Shift, RotateLeft,
                        WordRepresentation::Word32())
  DECL_SINGLE_REP_BINOP(Word64RotateLeft, Shift, RotateLeft,
                        WordRepresentation::Word64())

  OpIndex ShiftRightLogical(OpIndex left, uint32_t right,
                            WordRepresentation rep) {
    DCHECK_GE(right, 0);
    DCHECK_LT(right, rep.bit_width());
    return ShiftRightLogical(left, this->Word32Constant(right), rep);
  }
  OpIndex ShiftRightArithmetic(OpIndex left, uint32_t right,
                               WordRepresentation rep) {
    DCHECK_GE(right, 0);
    DCHECK_LT(right, rep.bit_width());
    return ShiftRightArithmetic(left, this->Word32Constant(right), rep);
  }

  DECL_SINGLE_REP_BINOP_NO_KIND(Word32Equal, Equal,
                                WordRepresentation::Word32())
  DECL_SINGLE_REP_BINOP_NO_KIND(Word64Equal, Equal,
                                WordRepresentation::Word64())
  DECL_SINGLE_REP_BINOP_NO_KIND(Float32Equal, Equal,
                                FloatRepresentation::Float32())
  DECL_SINGLE_REP_BINOP_NO_KIND(Float64Equal, Equal,
                                FloatRepresentation::Float64())
  OpIndex Equal(OpIndex left, OpIndex right, RegisterRepresentation rep) {
    return subclass().ReduceEqual(left, right, rep);
  }

  DECL_MULTI_REP_BINOP(IntLessThan, Comparison, RegisterRepresentation,
                       SignedLessThan)
  DECL_SINGLE_REP_BINOP(Int32LessThan, Comparison, SignedLessThan,
                        WordRepresentation::Word32())
  DECL_SINGLE_REP_BINOP(Int64LessThan, Comparison, SignedLessThan,
                        WordRepresentation::Word64())
  DECL_MULTI_REP_BINOP(UintLessThan, Comparison, RegisterRepresentation,
                       UnsignedLessThan)
  DECL_SINGLE_REP_BINOP(Uint32LessThan, Comparison, UnsignedLessThan,
                        WordRepresentation::Word32())
  DECL_SINGLE_REP_BINOP(Uint64LessThan, Comparison, UnsignedLessThan,
                        WordRepresentation::Word64())
  DECL_MULTI_REP_BINOP(FloatLessThan, Comparison, RegisterRepresentation,
                       SignedLessThan)
  DECL_SINGLE_REP_BINOP(Float32LessThan, Comparison, SignedLessThan,
                        FloatRepresentation::Float32())
  DECL_SINGLE_REP_BINOP(Float64LessThan, Comparison, SignedLessThan,
                        FloatRepresentation::Float64())

  DECL_MULTI_REP_BINOP(IntLessThanOrEqual, Comparison, RegisterRepresentation,
                       SignedLessThanOrEqual)
  DECL_SINGLE_REP_BINOP(Int32LessThanOrEqual, Comparison, SignedLessThanOrEqual,
                        WordRepresentation::Word32())
  DECL_SINGLE_REP_BINOP(Int64LessThanOrEqual, Comparison, SignedLessThanOrEqual,
                        WordRepresentation::Word64())
  DECL_MULTI_REP_BINOP(UintLessThanOrEqual, Comparison, RegisterRepresentation,
                       UnsignedLessThanOrEqual)
  DECL_SINGLE_REP_BINOP(Uint32LessThanOrEqual, Comparison,
                        UnsignedLessThanOrEqual, WordRepresentation::Word32())
  DECL_SINGLE_REP_BINOP(Uint64LessThanOrEqual, Comparison,
                        UnsignedLessThanOrEqual, WordRepresentation::Word64())
  DECL_MULTI_REP_BINOP(FloatLessThanOrEqual, Comparison, RegisterRepresentation,
                       SignedLessThanOrEqual)
  DECL_SINGLE_REP_BINOP(Float32LessThanOrEqual, Comparison,
                        SignedLessThanOrEqual, FloatRepresentation::Float32())
  DECL_SINGLE_REP_BINOP(Float64LessThanOrEqual, Comparison,
                        SignedLessThanOrEqual, FloatRepresentation::Float64())
  OpIndex Comparison(OpIndex left, OpIndex right, ComparisonOp::Kind kind,
                     RegisterRepresentation rep) {
    return subclass().ReduceComparison(left, right, kind, rep);
  }

#undef DECL_SINGLE_REP_BINOP
#undef DECL_MULTI_REP_BINOP
#undef DECL_SINGLE_REP_BINOP_NO_KIND

#define DECL_MULTI_REP_UNARY(name, operation, rep_type, kind)                \
  OpIndex name(OpIndex input, rep_type rep) {                                \
    return subclass().Reduce##operation(input, operation##Op::Kind::k##kind, \
                                        rep);                                \
  }
#define DECL_SINGLE_REP_UNARY(name, operation, kind, rep)                    \
  OpIndex name(OpIndex input) {                                              \
    return subclass().Reduce##operation(input, operation##Op::Kind::k##kind, \
                                        rep);                                \
  }

  DECL_MULTI_REP_UNARY(FloatAbs, FloatUnary, FloatRepresentation, Abs)
  DECL_SINGLE_REP_UNARY(Float32Abs, FloatUnary, Abs,
                        FloatRepresentation::Float32())
  DECL_SINGLE_REP_UNARY(Float64Abs, FloatUnary, Abs,
                        FloatRepresentation::Float64())
  DECL_MULTI_REP_UNARY(FloatNegate, FloatUnary, FloatRepresentation, Negate)
  DECL_SINGLE_REP_UNARY(Float32Negate, FloatUnary, Negate,
                        FloatRepresentation::Float32())
  DECL_SINGLE_REP_UNARY(Float64Negate, FloatUnary, Negate,
                        FloatRepresentation::Float64())
  DECL_SINGLE_REP_UNARY(Float64SilenceNaN, FloatUnary, SilenceNaN,
                        FloatRepresentation::Float64())
  DECL_MULTI_REP_UNARY(FloatRoundDown, FloatUnary, FloatRepresentation,
                       RoundDown)
  DECL_SINGLE_REP_UNARY(Float32RoundDown, FloatUnary, RoundDown,
                        FloatRepresentation::Float32())
  DECL_SINGLE_REP_UNARY(Float64RoundDown, FloatUnary, RoundDown,
                        FloatRepresentation::Float64())
  DECL_MULTI_REP_UNARY(FloatRoundUp, FloatUnary, FloatRepresentation, RoundUp)
  DECL_SINGLE_REP_UNARY(Float32RoundUp, FloatUnary, RoundUp,
                        FloatRepresentation::Float32())
  DECL_SINGLE_REP_UNARY(Float64RoundUp, FloatUnary, RoundUp,
                        FloatRepresentation::Float64())
  DECL_MULTI_REP_UNARY(FloatRoundToZero, FloatUnary, FloatRepresentation,
                       RoundToZero)
  DECL_SINGLE_REP_UNARY(Float32RoundToZero, FloatUnary, RoundToZero,
                        FloatRepresentation::Float32())
  DECL_SINGLE_REP_UNARY(Float64RoundToZero, FloatUnary, RoundToZero,
                        FloatRepresentation::Float64())
  DECL_MULTI_REP_UNARY(FloatRoundTiesEven, FloatUnary, FloatRepresentation,
                       RoundTiesEven)
  DECL_SINGLE_REP_UNARY(Float32RoundTiesEven, FloatUnary, RoundTiesEven,
                        FloatRepresentation::Float32())
  DECL_SINGLE_REP_UNARY(Float64RoundTiesEven, FloatUnary, RoundTiesEven,
                        FloatRepresentation::Float64())
  DECL_SINGLE_REP_UNARY(Float64Log, FloatUnary, Log,
                        FloatRepresentation::Float64())
  DECL_MULTI_REP_UNARY(FloatSqrt, FloatUnary, FloatRepresentation, Sqrt)
  DECL_SINGLE_REP_UNARY(Float32Sqrt, FloatUnary, Sqrt,
                        FloatRepresentation::Float32())
  DECL_SINGLE_REP_UNARY(Float64Sqrt, FloatUnary, Sqrt,
                        FloatRepresentation::Float64())
  DECL_SINGLE_REP_UNARY(Float64Exp, FloatUnary, Exp,
                        FloatRepresentation::Float64())
  DECL_SINGLE_REP_UNARY(Float64Expm1, FloatUnary, Expm1,
                        FloatRepresentation::Float64())
  DECL_SINGLE_REP_UNARY(Float64Sin, FloatUnary, Sin,
                        FloatRepresentation::Float64())
  DECL_SINGLE_REP_UNARY(Float64Cos, FloatUnary, Cos,
                        FloatRepresentation::Float64())
  DECL_SINGLE_REP_UNARY(Float64Sinh, FloatUnary, Sinh,
                        FloatRepresentation::Float64())
  DECL_SINGLE_REP_UNARY(Float64Cosh, FloatUnary, Cosh,
                        FloatRepresentation::Float64())
  DECL_SINGLE_REP_UNARY(Float64Asin, FloatUnary, Asin,
                        FloatRepresentation::Float64())
  DECL_SINGLE_REP_UNARY(Float64Acos, FloatUnary, Acos,
                        FloatRepresentation::Float64())
  DECL_SINGLE_REP_UNARY(Float64Asinh, FloatUnary, Asinh,
                        FloatRepresentation::Float64())
  DECL_SINGLE_REP_UNARY(Float64Acosh, FloatUnary, Acosh,
                        FloatRepresentation::Float64())
  DECL_SINGLE_REP_UNARY(Float64Tan, FloatUnary, Tan,
                        FloatRepresentation::Float64())
  DECL_SINGLE_REP_UNARY(Float64Tanh, FloatUnary, Tanh,
                        FloatRepresentation::Float64())
  DECL_SINGLE_REP_UNARY(Float64Log2, FloatUnary, Log2,
                        FloatRepresentation::Float64())
  DECL_SINGLE_REP_UNARY(Float64Log10, FloatUnary, Log10,
                        FloatRepresentation::Float64())
  DECL_SINGLE_REP_UNARY(Float64Log1p, FloatUnary, Log1p,
                        FloatRepresentation::Float64())
  DECL_SINGLE_REP_UNARY(Float64Atan, FloatUnary, Atan,
                        FloatRepresentation::Float64())
  DECL_SINGLE_REP_UNARY(Float64Atanh, FloatUnary, Atanh,
                        FloatRepresentation::Float64())
  DECL_SINGLE_REP_UNARY(Float64Cbrt, FloatUnary, Cbrt,
                        FloatRepresentation::Float64())

  DECL_MULTI_REP_UNARY(WordReverseBytes, WordUnary, WordRepresentation,
                       ReverseBytes)
  DECL_SINGLE_REP_UNARY(Word32ReverseBytes, WordUnary, ReverseBytes,
                        WordRepresentation::Word32())
  DECL_SINGLE_REP_UNARY(Word64ReverseBytes, WordUnary, ReverseBytes,
                        WordRepresentation::Word64())
  DECL_MULTI_REP_UNARY(WordCountLeadingZeros, WordUnary, WordRepresentation,
                       CountLeadingZeros)
  DECL_SINGLE_REP_UNARY(Word32CountLeadingZeros, WordUnary, CountLeadingZeros,
                        WordRepresentation::Word32())
  DECL_SINGLE_REP_UNARY(Word64CountLeadingZeros, WordUnary, CountLeadingZeros,
                        WordRepresentation::Word64())
  DECL_MULTI_REP_UNARY(WordCountTrailingZeros, WordUnary, WordRepresentation,
                       CountTrailingZeros)
  DECL_SINGLE_REP_UNARY(Word32CountTrailingZeros, WordUnary, CountTrailingZeros,
                        WordRepresentation::Word32())
  DECL_SINGLE_REP_UNARY(Word64CountTrailingZeros, WordUnary, CountTrailingZeros,
                        WordRepresentation::Word64())
  DECL_MULTI_REP_UNARY(WordPopCount, WordUnary, WordRepresentation, PopCount)
  DECL_SINGLE_REP_UNARY(Word32PopCount, WordUnary, PopCount,
                        WordRepresentation::Word32())
  DECL_SINGLE_REP_UNARY(Word64PopCount, WordUnary, PopCount,
                        WordRepresentation::Word64())
  DECL_MULTI_REP_UNARY(WordSignExtend8, WordUnary, WordRepresentation,
                       SignExtend8)
  DECL_SINGLE_REP_UNARY(Word32SignExtend8, WordUnary, SignExtend8,
                        WordRepresentation::Word32())
  DECL_SINGLE_REP_UNARY(Word64SignExtend8, WordUnary, SignExtend8,
                        WordRepresentation::Word64())
  DECL_MULTI_REP_UNARY(WordSignExtend16, WordUnary, WordRepresentation,
                       SignExtend16)
  DECL_SINGLE_REP_UNARY(Word32SignExtend16, WordUnary, SignExtend16,
                        WordRepresentation::Word32())
  DECL_SINGLE_REP_UNARY(Word64SignExtend16, WordUnary, SignExtend16,
                        WordRepresentation::Word64())
#undef DECL_SINGLE_REP_UNARY
#undef DECL_MULTI_REP_UNARY

  OpIndex Float64InsertWord32(OpIndex float64, OpIndex word32,
                              Float64InsertWord32Op::Kind kind) {
    return subclass().ReduceFloat64InsertWord32(float64, word32, kind);
  }

  OpIndex TaggedBitcast(OpIndex input, RegisterRepresentation from,
                        RegisterRepresentation to) {
    return subclass().ReduceTaggedBitcast(input, from, to);
  }
  OpIndex BitcastTaggedToWord(OpIndex tagged) {
    return TaggedBitcast(tagged, RegisterRepresentation::Tagged(),
                         RegisterRepresentation::PointerSized());
  }
  OpIndex BitcastWordToTagged(OpIndex word) {
    return TaggedBitcast(word, RegisterRepresentation::PointerSized(),
                         RegisterRepresentation::Tagged());
  }

  OpIndex Word32Constant(uint32_t value) {
    return subclass().ReduceConstant(ConstantOp::Kind::kWord32,
                                     uint64_t{value});
  }
  OpIndex Word32Constant(int32_t value) {
    return Word32Constant(static_cast<uint32_t>(value));
  }
  OpIndex Word64Constant(uint64_t value) {
    return subclass().ReduceConstant(ConstantOp::Kind::kWord64, value);
  }
  OpIndex Word64Constant(int64_t value) {
    return Word64Constant(static_cast<uint64_t>(value));
  }
  OpIndex WordConstant(uint64_t value, WordRepresentation rep) {
    switch (rep.value()) {
      case WordRepresentation::Word32():
        return Word32Constant(static_cast<uint32_t>(value));
      case WordRepresentation::Word64():
        return Word64Constant(value);
    }
  }
  OpIndex Float32Constant(float value) {
    return subclass().ReduceConstant(ConstantOp::Kind::kFloat32, value);
  }
  OpIndex Float64Constant(double value) {
    return subclass().ReduceConstant(ConstantOp::Kind::kFloat64, value);
  }
  OpIndex FloatConstant(double value, FloatRepresentation rep) {
    switch (rep.value()) {
      case FloatRepresentation::Float32():
        return Float32Constant(static_cast<float>(value));
      case FloatRepresentation::Float64():
        return Float64Constant(value);
    }
  }
  OpIndex NumberConstant(double value) {
    return subclass().ReduceConstant(ConstantOp::Kind::kNumber, value);
  }
  OpIndex TaggedIndexConstant(int32_t value) {
    return subclass().ReduceConstant(ConstantOp::Kind::kTaggedIndex,
                                     uint64_t{static_cast<uint32_t>(value)});
  }
  OpIndex HeapConstant(Handle<HeapObject> value) {
    return subclass().ReduceConstant(ConstantOp::Kind::kHeapObject, value);
  }
  OpIndex CompressedHeapConstant(Handle<HeapObject> value) {
    return subclass().ReduceConstant(ConstantOp::Kind::kHeapObject, value);
  }
  OpIndex ExternalConstant(ExternalReference value) {
    return subclass().ReduceConstant(ConstantOp::Kind::kExternal, value);
  }
  OpIndex RelocatableConstant(int64_t value, RelocInfo::Mode mode) {
    DCHECK_EQ(mode, any_of(RelocInfo::WASM_CALL, RelocInfo::WASM_STUB_CALL));
    return subclass().ReduceConstant(
        mode == RelocInfo::WASM_CALL
            ? ConstantOp::Kind::kRelocatableWasmCall
            : ConstantOp::Kind::kRelocatableWasmStubCall,
        static_cast<uint64_t>(value));
  }

#define DECL_CHANGE(name, kind, assumption, from, to)                  \
  OpIndex name(OpIndex input) {                                        \
    return subclass().ReduceChange(                                    \
        input, ChangeOp::Kind::kind, ChangeOp::Assumption::assumption, \
        RegisterRepresentation::from(), RegisterRepresentation::to()); \
  }
#define DECL_TRY_CHANGE(name, kind, from, to)                         \
  OpIndex name(OpIndex input) {                                       \
    return subclass().ReduceTryChange(input, TryChangeOp::Kind::kind, \
                                      FloatRepresentation::from(),    \
                                      WordRepresentation::to());      \
  }

  DECL_CHANGE(BitcastWord32ToWord64, kBitcast, kNoAssumption, Word32, Word64)
  DECL_CHANGE(BitcastFloat32ToWord32, kBitcast, kNoAssumption, Float32, Word32)
  DECL_CHANGE(BitcastWord32ToFloat32, kBitcast, kNoAssumption, Word32, Float32)
  DECL_CHANGE(BitcastFloat64ToWord64, kBitcast, kNoAssumption, Float64, Word64)
  DECL_CHANGE(BitcastWord64ToFloat64, kBitcast, kNoAssumption, Word64, Float64)
  DECL_CHANGE(ChangeUint32ToUint64, kZeroExtend, kNoAssumption, Word32, Word64)
  DECL_CHANGE(ChangeInt32ToInt64, kSignExtend, kNoAssumption, Word32, Word64)
  DECL_CHANGE(ChangeInt32ToFloat64, kSignedToFloat, kNoAssumption, Word32,
              Float64)
  DECL_CHANGE(ChangeInt64ToFloat64, kSignedToFloat, kNoAssumption, Word64,
              Float64)
  DECL_CHANGE(ChangeInt32ToFloat32, kSignedToFloat, kNoAssumption, Word32,
              Float32)
  DECL_CHANGE(ChangeInt64ToFloat32, kSignedToFloat, kNoAssumption, Word64,
              Float32)
  DECL_CHANGE(ChangeUint32ToFloat32, kUnsignedToFloat, kNoAssumption, Word32,
              Float32)
  DECL_CHANGE(ChangeUint64ToFloat32, kUnsignedToFloat, kNoAssumption, Word64,
              Float32)
  DECL_CHANGE(ReversibleInt64ToFloat64, kSignedToFloat, kReversible, Word64,
              Float64)
  DECL_CHANGE(ChangeUint64ToFloat64, kUnsignedToFloat, kNoAssumption, Word64,
              Float64)
  DECL_CHANGE(ReversibleUint64ToFloat64, kUnsignedToFloat, kReversible, Word64,
              Float64)
  DECL_CHANGE(ChangeUint32ToFloat64, kUnsignedToFloat, kNoAssumption, Word32,
              Float64)
  DECL_CHANGE(ChangeFloat64ToFloat32, kFloatConversion, kNoAssumption, Float64,
              Float32)
  DECL_CHANGE(ChangeFloat32ToFloat64, kFloatConversion, kNoAssumption, Float32,
              Float64)
  DECL_CHANGE(JSTruncateFloat64ToWord32, kJSFloatTruncate, kNoAssumption,
              Float64, Word32)

#define DECL_SIGNED_FLOAT_TRUNCATE(FloatBits, ResultBits)                     \
  DECL_CHANGE(TruncateFloat##FloatBits##ToInt##ResultBits##OverflowUndefined, \
              kSignedFloatTruncateOverflowToMin, kNoOverflow,                 \
              Float##FloatBits, Word##ResultBits)                             \
  DECL_CHANGE(TruncateFloat##FloatBits##ToInt##ResultBits##OverflowToMin,     \
              kSignedFloatTruncateOverflowToMin, kNoAssumption,               \
              Float##FloatBits, Word##ResultBits)                             \
  DECL_TRY_CHANGE(TryTruncateFloat##FloatBits##ToInt##ResultBits,             \
                  kSignedFloatTruncateOverflowUndefined, Float##FloatBits,    \
                  Word##ResultBits)

  DECL_SIGNED_FLOAT_TRUNCATE(64, 64)
  DECL_SIGNED_FLOAT_TRUNCATE(64, 32)
  DECL_SIGNED_FLOAT_TRUNCATE(32, 64)
  DECL_SIGNED_FLOAT_TRUNCATE(32, 32)
#undef DECL_SIGNED_FLOAT_TRUNCATE

#define DECL_UNSIGNED_FLOAT_TRUNCATE(FloatBits, ResultBits)                    \
  DECL_CHANGE(TruncateFloat##FloatBits##ToUint##ResultBits##OverflowUndefined, \
              kUnsignedFloatTruncateOverflowToMin, kNoOverflow,                \
              Float##FloatBits, Word##ResultBits)                              \
  DECL_CHANGE(TruncateFloat##FloatBits##ToUint##ResultBits##OverflowToMin,     \
              kUnsignedFloatTruncateOverflowToMin, kNoAssumption,              \
              Float##FloatBits, Word##ResultBits)                              \
  DECL_TRY_CHANGE(TryTruncateFloat##FloatBits##ToUint##ResultBits,             \
                  kUnsignedFloatTruncateOverflowUndefined, Float##FloatBits,   \
                  Word##ResultBits)

  DECL_UNSIGNED_FLOAT_TRUNCATE(64, 64)
  DECL_UNSIGNED_FLOAT_TRUNCATE(64, 32)
  DECL_UNSIGNED_FLOAT_TRUNCATE(32, 64)
  DECL_UNSIGNED_FLOAT_TRUNCATE(32, 32)
#undef DECL_UNSIGNED_FLOAT_TRUNCATE

  DECL_CHANGE(ReversibleFloat64ToInt32, kSignedFloatTruncateOverflowToMin,
              kReversible, Float64, Word32)
  DECL_CHANGE(ReversibleFloat64ToUint32, kUnsignedFloatTruncateOverflowToMin,
              kReversible, Float64, Word32)
  DECL_CHANGE(ReversibleFloat64ToInt64, kSignedFloatTruncateOverflowToMin,
              kReversible, Float64, Word64)
  DECL_CHANGE(ReversibleFloat64ToUint64, kUnsignedFloatTruncateOverflowToMin,
              kReversible, Float64, Word64)
  DECL_CHANGE(Float64ExtractLowWord32, kExtractLowHalf, kNoAssumption, Float64,
              Word32)
  DECL_CHANGE(Float64ExtractHighWord32, kExtractHighHalf, kNoAssumption,
              Float64, Word32)
#undef DECL_CHANGE
#undef DECL_TRY_CHANGE

  OpIndex Load(OpIndex base, LoadOp::Kind kind, MemoryRepresentation loaded_rep,
               int32_t offset = 0) {
    return Load(base, OpIndex::Invalid(), kind, loaded_rep, offset);
  }
  OpIndex Load(OpIndex base, OpIndex index, LoadOp::Kind kind,
               MemoryRepresentation loaded_rep, int32_t offset = 0,
               uint8_t element_size_log2 = 0) {
    return subclass().ReduceLoad(base, index, kind, loaded_rep,
                                 loaded_rep.ToRegisterRepresentation(), offset,
                                 element_size_log2);
  }
  void Store(OpIndex base, OpIndex value, StoreOp::Kind kind,
             MemoryRepresentation stored_rep, WriteBarrierKind write_barrier,
             int32_t offset = 0) {
    Store(base, OpIndex::Invalid(), value, kind, stored_rep, write_barrier,
          offset);
  }
  void Store(OpIndex base, OpIndex index, OpIndex value, StoreOp::Kind kind,
             MemoryRepresentation stored_rep, WriteBarrierKind write_barrier,
             int32_t offset = 0, uint8_t element_size_log2 = 0) {
    subclass().ReduceStore(base, index, value, kind, stored_rep, write_barrier,
                           offset, element_size_log2);
  }

  void Retain(OpIndex value) { subclass().ReduceRetain(value); }

  OpIndex StackPointerGreaterThan(OpIndex limit, StackCheckKind kind) {
    return subclass().ReduceStackPointerGreaterThan(limit, kind);
  }

  OpIndex StackCheckOffset() {
    return subclass().ReduceFrameConstant(
        FrameConstantOp::Kind::kStackCheckOffset);
  }
  OpIndex FramePointer() {
    return subclass().ReduceFrameConstant(FrameConstantOp::Kind::kFramePointer);
  }
  OpIndex ParentFramePointer() {
    return subclass().ReduceFrameConstant(
        FrameConstantOp::Kind::kParentFramePointer);
  }

  OpIndex StackSlot(int size, int alignment) {
    return subclass().ReduceStackSlot(size, alignment);
  }

  void Goto(Block* destination) { subclass().ReduceGoto(destination); }
  void Branch(OpIndex condition, Block* if_true, Block* if_false) {
    subclass().ReduceBranch(condition, if_true, if_false);
  }
  OpIndex Select(OpIndex cond, OpIndex vtrue, OpIndex vfalse,
                 RegisterRepresentation rep, BranchHint hint,
                 SelectOp::Implementation implem) {
    return subclass().ReduceSelect(cond, vtrue, vfalse, rep, hint, implem);
  }
  void Switch(OpIndex input, base::Vector<const SwitchOp::Case> cases,
              Block* default_case) {
    subclass().ReduceSwitch(input, cases, default_case);
  }
  OpIndex CatchException(OpIndex call, Block* if_success, Block* if_exception) {
    return subclass().ReduceCatchException(call, if_success, if_exception);
  }
  void Unreachable() { subclass().ReduceUnreachable(); }

  OpIndex Parameter(int index, const char* debug_name = nullptr) {
    return subclass().ReduceParameter(index, debug_name);
  }
  OpIndex OsrValue(int index) { return subclass().ReduceOsrValue(index); }
  void Return(OpIndex pop_count, base::Vector<OpIndex> return_values) {
    subclass().ReduceReturn(pop_count, return_values);
  }
  void Return(OpIndex result) {
    Return(Word32Constant(0), base::VectorOf({result}));
  }

  OpIndex Call(OpIndex callee, base::Vector<const OpIndex> arguments,
               const CallDescriptor* descriptor) {
    return subclass().ReduceCall(callee, arguments, descriptor);
  }
  OpIndex CallMaybeDeopt(OpIndex callee, base::Vector<const OpIndex> arguments,
                         const CallDescriptor* descriptor,
                         OpIndex frame_state) {
    OpIndex call = subclass().ReduceCall(callee, arguments, descriptor);
    subclass().ReduceCheckLazyDeopt(call, frame_state);
    return call;
  }
  void TailCall(OpIndex callee, base::Vector<const OpIndex> arguments,
                const CallDescriptor* descriptor) {
    subclass().ReduceTailCall(callee, arguments, descriptor);
  }

  OpIndex FrameState(base::Vector<const OpIndex> inputs, bool inlined,
                     const FrameStateData* data) {
    return subclass().ReduceFrameState(inputs, inlined, data);
  }
  void DeoptimizeIf(OpIndex condition, OpIndex frame_state,
                    const DeoptimizeParameters* parameters) {
    subclass().ReduceDeoptimizeIf(condition, frame_state, false, parameters);
  }
  void DeoptimizeIfNot(OpIndex condition, OpIndex frame_state,
                       const DeoptimizeParameters* parameters) {
    subclass().ReduceDeoptimizeIf(condition, frame_state, true, parameters);
  }
  void Deoptimize(OpIndex frame_state, const DeoptimizeParameters* parameters) {
    subclass().ReduceDeoptimize(frame_state, parameters);
  }

  void TrapIf(OpIndex condition, TrapId trap_id) {
    subclass().ReduceTrapIf(condition, false, trap_id);
  }
  void TrapIfNot(OpIndex condition, TrapId trap_id) {
    subclass().ReduceTrapIf(condition, true, trap_id);
  }

  OpIndex Phi(base::Vector<const OpIndex> inputs, RegisterRepresentation rep) {
    return subclass().ReducePhi(inputs, rep);
  }
  OpIndex PendingLoopPhi(OpIndex first, RegisterRepresentation rep,
                         OpIndex old_backedge_index) {
    return subclass().ReducePendingLoopPhi(first, rep, old_backedge_index);
  }
  OpIndex PendingLoopPhi(OpIndex first, RegisterRepresentation rep,
                         Node* old_backedge_index) {
    return subclass().ReducePendingLoopPhi(first, rep, old_backedge_index);
  }

  OpIndex Tuple(OpIndex a, OpIndex b) {
    return subclass().ReduceTuple(base::VectorOf({a, b}));
  }
  OpIndex Projection(OpIndex tuple, uint16_t index) {
    return subclass().ReduceProjection(tuple, index);
  }

 private:
  Subclass& subclass() { return *static_cast<Subclass*>(this); }
};

// This empty base-class is used to provide default-implementations of plain
// methods emitting operations.
template <class Assembler>
class AssemblerBase {
 public:
#define EMIT_OP(Name)                                                       \
  template <class... Args>                                                  \
  OpIndex Reduce##Name(Args... args) {                                      \
    return static_cast<Assembler*>(this)->template Emit<Name##Op>(args...); \
  }
  TURBOSHAFT_OPERATION_LIST(EMIT_OP)
#undef EMIT_OP
};

class Assembler
    : public AssemblerInterface<Assembler, AssemblerBase<Assembler>>,
      public OperationMatching<Assembler> {
 public:
  Block* NewBlock(Block::Kind kind) { return graph_.NewBlock(kind); }

  void EnterBlock(const Block& block) { USE(block); }
  void ExitBlock(const Block& block) { USE(block); }

  V8_INLINE V8_WARN_UNUSED_RESULT bool Bind(Block* block) {
    if (!graph().Add(block)) return false;
    DCHECK_NULL(current_block_);
    current_block_ = block;
    return true;
  }

  V8_INLINE void BindReachable(Block* block) {
    bool bound = Bind(block);
    DCHECK(bound);
    USE(bound);
  }

  void SetCurrentOrigin(OpIndex operation_origin) {
    current_operation_origin_ = operation_origin;
  }

  OpIndex ReducePhi(base::Vector<const OpIndex> inputs,
                    RegisterRepresentation rep) {
    DCHECK(current_block()->IsMerge() &&
           inputs.size() == current_block()->Predecessors().size());
    return Base::ReducePhi(inputs, rep);
  }

  template <class... Args>
  OpIndex ReducePendingLoopPhi(Args... args) {
    DCHECK(current_block()->IsLoop());
    return Base::ReducePendingLoopPhi(args...);
  }

  OpIndex ReduceGoto(Block* destination) {
    destination->AddPredecessor(current_block());
    return Base::ReduceGoto(destination);
  }

  OpIndex ReduceBranch(OpIndex condition, Block* if_true, Block* if_false) {
    if_true->AddPredecessor(current_block());
    if_false->AddPredecessor(current_block());
    return Base::ReduceBranch(condition, if_true, if_false);
  }

  OpIndex ReduceCatchException(OpIndex call, Block* if_success,
                               Block* if_exception) {
    if_success->AddPredecessor(current_block());
    if_exception->AddPredecessor(current_block());
    return Base::ReduceCatchException(call, if_success, if_exception);
  }

  OpIndex ReduceSwitch(OpIndex input, base::Vector<const SwitchOp::Case> cases,
                       Block* default_case) {
    for (SwitchOp::Case c : cases) {
      c.destination->AddPredecessor(current_block());
    }
    default_case->AddPredecessor(current_block());
    return Base::ReduceSwitch(input, cases, default_case);
  }

  explicit Assembler(Graph* graph, Zone* phase_zone)
      : graph_(*graph), phase_zone_(phase_zone) {
    graph_.Reset();
    SupportedOperations::Initialize();
  }

  Block* current_block() { return current_block_; }
  Zone* graph_zone() { return graph().graph_zone(); }
  Graph& graph() { return graph_; }
  Zone* phase_zone() { return phase_zone_; }

 private:
  friend class AssemblerBase<Assembler>;
  void FinalizeBlock() {
    graph().Finalize(current_block_);
    current_block_ = nullptr;
  }

  template <class Op, class... Args>
  OpIndex Emit(Args... args) {
    static_assert((std::is_base_of<Operation, Op>::value));
    static_assert(!(std::is_same<Op, Operation>::value));
    DCHECK_NOT_NULL(current_block_);
    OpIndex result = graph().next_operation_index();
    Op& op = graph().Add<Op>(args...);
    graph().operation_origins()[result] = current_operation_origin_;
    if (op.Properties().is_block_terminator) FinalizeBlock();
    return result;
  }

  Block* current_block_ = nullptr;
  Graph& graph_;
  OpIndex current_operation_origin_ = OpIndex::Invalid();
  Zone* const phase_zone_;
};

}  // namespace v8::internal::compiler::turboshaft

#endif  // V8_COMPILER_TURBOSHAFT_ASSEMBLER_H_
