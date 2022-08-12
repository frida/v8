// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// The file/directory layout of Google Test is not yet considered stable. Until
// it stabilizes, Chromium code will use forwarding headers in testing/gtest
// and testing/gmock, instead of directly including files in
// third_party/googletest.

#if defined(V8_DISABLE_TESTS)
#ifndef FRIEND_TEST
#define FRIEND_TEST(test_case_name, test_name)
#endif
#elif !defined(GOOGLE3)
#include "third_party/googletest/src/googletest/include/gtest/gtest_prod.h"
#endif