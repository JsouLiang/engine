//
// Created by liangting on 2019/2/27.
//

#ifndef JSC_JSC_UTILS_H
#define JSC_JSC_UTILS_H

#include "JavaScriptCore/JSContextRef.h"
#include "third_party/dart/runtime/include/dart_api.h"
#include "third_party/tonic/dart_binding_macros.h"

#define DART_REGISTER_NATIVE_STATIC_(CLASS, METHOD) \
  DART_REGISTER_NATIVE_STATIC(CLASS, METHOD),
  
#define DART_BIND_ALL_STATIC(CLASS, FOR_EACH)                              \
  FOR_EACH(DART_NATIVE_CALLBACK_STATIC)                                    \
  void CLASS::RegisterNatives(tonic::DartLibraryNatives* natives) { \
    natives->Register({FOR_EACH(DART_REGISTER_NATIVE_STATIC_)});            \
  }


namespace jsc {

Dart_Handle newCReturnValue();
Dart_Handle newCReturnValue(bool boolean, JSValueRef exception);
Dart_Handle newCReturnValue(double dvalue, JSValueRef exception);
Dart_Handle newCReturnValue(JSValueRef reference, JSValueRef exception);
Dart_Handle newCReturnValue(double dvalue,
                            bool boolean,
                            JSValueRef reference,
                            JSValueRef exception);
}
#endif //JSC_JSC_UTILS_H
