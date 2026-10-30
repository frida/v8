// Copyright 2026 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Renames libunicode's global symbols so that this copy cannot collide with
// another one linked into the same binary, such as the one inside QuickJS.

#ifndef V8_THIRD_PARTY_LIBUNICODE_V8_LIBUNICODE_PREFIX_H_
#define V8_THIRD_PARTY_LIBUNICODE_V8_LIBUNICODE_PREFIX_H_

#define __dbuf_put_u16 v8_libunicode___dbuf_put_u16
#define __dbuf_put_u32 v8_libunicode___dbuf_put_u32
#define __dbuf_put_u64 v8_libunicode___dbuf_put_u64
#define __dbuf_putc v8_libunicode___dbuf_putc
#define cr_copy v8_libunicode_cr_copy
#define cr_free v8_libunicode_cr_free
#define cr_init v8_libunicode_cr_init
#define cr_invert v8_libunicode_cr_invert
#define cr_op v8_libunicode_cr_op
#define cr_op1 v8_libunicode_cr_op1
#define cr_realloc v8_libunicode_cr_realloc
#define cr_regexp_canonicalize v8_libunicode_cr_regexp_canonicalize
#define dbuf_claim v8_libunicode_dbuf_claim
#define dbuf_free v8_libunicode_dbuf_free
#define dbuf_init v8_libunicode_dbuf_init
#define dbuf_init2 v8_libunicode_dbuf_init2
#define dbuf_printf v8_libunicode_dbuf_printf
#define dbuf_put v8_libunicode_dbuf_put
#define dbuf_put_self v8_libunicode_dbuf_put_self
#define dbuf_putstr v8_libunicode_dbuf_putstr
#define has_suffix v8_libunicode_has_suffix
#define lre_canonicalize v8_libunicode_lre_canonicalize
#define lre_case_conv v8_libunicode_lre_case_conv
#define lre_ctype_bits v8_libunicode_lre_ctype_bits
#define lre_is_case_ignorable v8_libunicode_lre_is_case_ignorable
#define lre_is_cased v8_libunicode_lre_is_cased
#define lre_is_id_continue v8_libunicode_lre_is_id_continue
#define lre_is_id_start v8_libunicode_lre_is_id_start
#define lre_is_space_non_ascii v8_libunicode_lre_is_space_non_ascii
#define pstrcat v8_libunicode_pstrcat
#define pstrcpy v8_libunicode_pstrcpy
#define rqsort v8_libunicode_rqsort
#define strstart v8_libunicode_strstart
#define unicode_from_utf8 v8_libunicode_unicode_from_utf8
#define unicode_general_category v8_libunicode_unicode_general_category
#define unicode_normalize v8_libunicode_unicode_normalize
#define unicode_prop v8_libunicode_unicode_prop
#define unicode_script v8_libunicode_unicode_script
#define unicode_sequence_prop v8_libunicode_unicode_sequence_prop
#define unicode_to_utf8 v8_libunicode_unicode_to_utf8

#endif  // V8_THIRD_PARTY_LIBUNICODE_V8_LIBUNICODE_PREFIX_H_
