// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/lib/jsc/dart_jsc.h"

#include "flutter/fml/build_config.h"
#include "third_party/tonic/dart_library_natives.h"
#include "third_party/tonic/converter/dart_converter.h"
#include "third_party/tonic/logging/dart_error.h"
#include "flutter/lib/jsc/shell/JSContext.h"
#include "flutter/lib/jsc/shell/JSValue.h"
#include "flutter/lib/jsc/shell/JSString.h"


using tonic::ToDart;

namespace blink {
namespace {

static tonic::DartLibraryNatives* g_natives;
// static tonic::DartLibraryNatives* g_natives_secondary;

Dart_NativeFunction GetNativeFunction(Dart_Handle name,
                                      int argument_count,
                                      bool* auto_setup_scope) {
  return g_natives->GetNativeFunction(name, argument_count, auto_setup_scope);
}

// Dart_NativeFunction GetNativeFunctionSecondary(Dart_Handle name,
//                                                int argument_count,
//                                                bool* auto_setup_scope) {
//   return g_natives_secondary->GetNativeFunction(name, argument_count,
//                                                 auto_setup_scope);
// }

const uint8_t* GetSymbol(Dart_NativeFunction native_function) {
  
  return g_natives->GetSymbol(native_function);
}

// const uint8_t* GetSymbolSecondary(Dart_NativeFunction native_function) {
//   return g_natives_secondary->GetSymbol(native_function);
// }

}  // namespace

void DartJSC::InitForIsolate() {
  if (!g_natives) {
    g_natives = new tonic::DartLibraryNatives();
    JSContext::RegisterNatives(g_natives);
    JSContextGroup::RegisterNatives(g_natives);
    JSValue::RegisterNatives(g_natives);
    JSString::RegisterNatives(g_natives);
  }
  DART_CHECK_VALID(Dart_SetNativeResolver(Dart_LookupLibrary(ToDart("dart:jsc")),
                                          GetNativeFunction, GetSymbol));

}

}  // namespace blink
