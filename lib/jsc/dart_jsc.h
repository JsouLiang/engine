// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_LIB_UI_DART_JSC_H_
#define FLUTTER_LIB_UI_DART_JSC_H_

#include "flutter/fml/macros.h"

namespace blink {

class DartJSC {
 public:
  static void InitForIsolate();

 private:
  FML_DISALLOW_IMPLICIT_CONSTRUCTORS(DartJSC);
};

}  // namespace blink

#endif  // FLUTTER_LIB_UI_DART_JSC_H_
