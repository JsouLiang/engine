#include "flutter/lib/jsc/shell/JSValue.h"
#include "JavaScriptCore/JSContextRef.h"
#include "JavaScriptCore/JSStringRef.h"

#include "third_party/dart/runtime/include/dart_api.h"
#include "third_party/tonic/converter/dart_converter.h"
#include "third_party/tonic/dart_args.h"
#include "third_party/tonic/dart_binding_macros.h"
#include "third_party/tonic/dart_library_natives.h"
#include "third_party/tonic/dart_persistent_value.h"
// #include "third_party/tonic/dart_microtask_queue.h"
#include "third_party/tonic/logging/dart_invoke.h"
// #include "third_party/tonic/typed_data/dart_byte_data.h"
#include "flutter/lib/jsc/shell/jsc_utils.h"

using tonic::ToDart;

namespace blink {

// RefCountedDartWrappable
// IMPLEMENT_WRAPPERTYPEINFO(jsc, JSValue);

#define FOR_EACH_BINDING(V) \
  V(JSValue, getType)       \
  V(JSValue, isUndefined)   \
  V(JSValue, isNull)        \
  V(JSValue, isBoolean)     \
  V(JSValue, isNumber)      \
  V(JSValue, isString)      \
  V(JSValue, isObject)      \
  V(JSValue, isEqual)       \
  V(JSValue, isStrictEqual)       \
  V(JSValue, isInstanceOfConstructor)       \
  V(JSValue, makeUndefined) \
  V(JSValue, makeNull)      \
  V(JSValue, makeBoolean)   \
  V(JSValue, makeNumber)    \
  V(JSValue, makeString)    \
  V(JSValue, makeFromJSONString)    \
  V(JSValue, createJSONString)    \
  V(JSValue, toBoolean)    \
  V(JSValue, toNumber)    \
  V(JSValue, toStringCopy)    \
  V(JSValue, toObject)    \
  V(JSValue, protect)    \
  V(JSValue, unprotect)    \
  V(JSValue, toStringValue)    \
  V(JSValue, setException)   

  // V(JSValue, isDate)        
  // V(JSValue, isArray)       


DART_BIND_ALL_STATIC(JSValue, FOR_EACH_BINDING)

// FOR_EACH_BINDING(DART_NATIVE_CALLBACK)

// void JSValue::RegisterNatives(tonic::DartLibraryNatives* natives) {
//   natives->Register({FOR_EACH_BINDING(DART_REGISTER_NATIVE)});
// }
JSValue::JSValue() = default;
JSValue::~JSValue() = default;
int JSValue::getType(long ctxRef, long valueRef) {
  return (int)(JSValueGetType((JSContextRef)ctxRef, (JSValueRef)valueRef));
}
bool JSValue::isUndefined(long ctxRef, long valueRef) {
  return JSValueIsUndefined((JSContextRef)ctxRef, (JSValueRef)valueRef);
}
bool JSValue::isNull(long ctxRef, long valueRef) {
  return JSValueIsNull((JSContextRef)ctxRef, (JSValueRef)valueRef);
}
bool JSValue::isBoolean(long ctxRef, long valueRef) {
  return JSValueIsBoolean((JSContextRef)ctxRef, (JSValueRef)valueRef);
}
bool JSValue::isNumber(long ctxRef, long valueRef) {
  return JSValueIsNumber((JSContextRef)ctxRef, (JSValueRef)valueRef);
}
bool JSValue::isString(long ctxRef, long valueRef) {
  return JSValueIsString((JSContextRef)ctxRef, (JSValueRef)valueRef);
}
bool JSValue::isObject(long ctxRef, long valueRef) {
  return JSValueIsObject((JSContextRef)ctxRef, (JSValueRef)valueRef);
}
// todo JSValueIsArray、JSValueIsDate支持版本问题 iOS 9.0+ OSX 10.11 Android
// jsc里不确定是否已经有了
// ref:https://github.com/WebKit/webkit/commit/a558e2d150c37a87222a83f8cd30f5d1be7d4355
// http://exbrowser.com/?p=660

// bool JSValue::isArray(long ctxRef,  long valueRef){
//     return JSValueIsArray((JSContextRef)ctxRef, (JSValueRef)valueRef);
// }
// bool JSValue::isDate(long ctxRef,  long valueRef){
//     return JSValueIsDate((JSContextRef)ctxRef, (JSValueRef)valueRef);
// }

/* Comparing values */

Dart_Handle JSValue::isEqual(long ctxRef,
                             long valueRef1,
                             long valueRef2) {
  JSValueRef exception = NULL;
  bool is_equal = JSValueIsEqual((JSContextRef)ctxRef, (JSValueRef)valueRef1,
                                 (JSValueRef)valueRef2, &exception);
  return jsc::newCReturnValue(is_equal, exception);
}

bool JSValue::isStrictEqual(long ctxRef, long a, long b) {
  return JSValueIsStrictEqual((JSContextRef)ctxRef, (JSValueRef)a,
                              (JSValueRef)b);
}

Dart_Handle JSValue::isInstanceOfConstructor(long ctxRef,
                                             long valueRef,
                                             long constructor) {
  JSValueRef exception = NULL;
  bool value =
      JSValueIsInstanceOfConstructor((JSContextRef)ctxRef, (JSValueRef)valueRef,
                                     (JSObjectRef)constructor, &exception);

  return jsc::newCReturnValue(value, exception);
}

/* Creating values */
long JSValue::makeUndefined(long ctxRef) {
  JSValueRef valueRef = JSValueMakeUndefined((JSContextRef)ctxRef);
  JSValueProtect((JSContextRef)ctxRef, valueRef);
  return (long)valueRef;
}

long JSValue::makeNull(long ctxRef) {
  JSValueRef valueRef = JSValueMakeNull((JSContextRef)ctxRef);
  JSValueProtect((JSContextRef)ctxRef, valueRef);
  return (long)valueRef;
}

long JSValue::makeBoolean(long ctxRef, bool value) {
  JSValueRef valueRef = JSValueMakeBoolean((JSContextRef)ctxRef, value);
  JSValueProtect((JSContextRef)ctxRef, valueRef);
  return (long)valueRef;
}

long JSValue::makeNumber(long ctxRef, double value) {
  JSValueRef valueRef = JSValueMakeNumber((JSContextRef)ctxRef, value);
  JSValueProtect((JSContextRef)ctxRef, valueRef);
  return (long)valueRef;
}

long JSValue::makeString(long ctxRef, long stringRef) {
  JSValueRef valueRef =
      JSValueMakeString((JSContextRef)ctxRef, (JSStringRef)stringRef);
  JSValueProtect((JSContextRef)ctxRef, valueRef);
  return (long)valueRef;
}

/* Converting to and from JSON formatted strings */
long JSValue::makeFromJSONString(long ctxRef, long stringRef) {
  JSValueRef value =
      JSValueMakeFromJSONString((JSContextRef)ctxRef, (JSStringRef)stringRef);
  JSValueProtect((JSContextRef)ctxRef, value);
  return (long)value;
}

Dart_Handle JSValue::createJSONString(long ctxRef,
                                      long valueRef,
                                      int indent) {
  JSValueRef exception = NULL;
  JSStringRef value = JSValueCreateJSONString(
      (JSContextRef)ctxRef, (JSValueRef)valueRef, (unsigned)indent, &exception);
  if (value)
    value = JSStringRetain(value);
  return jsc::newCReturnValue(value, exception);
}

/* Converting to primitive values */
bool JSValue::toBoolean(long ctxRef, long valueRef) {
  return JSValueToBoolean((JSContextRef)ctxRef, (JSValueRef)valueRef);
}

Dart_Handle JSValue::toNumber(long ctxRef, long valueRef) {
  JSValueRef exception = NULL;
  double dret =
      JSValueToNumber((JSContextRef)ctxRef, (JSValueRef)valueRef, &exception);
  return jsc::newCReturnValue(dret, exception);
}

std::u16string JSValue::toStringValue(long ctxRef, long valueRef) {
  if (!isUndefined(ctxRef, valueRef) && !isNull(ctxRef, valueRef)) {
    JSValueRef exception = NULL;
    JSStringRef s = JSValueToStringCopy((JSContextRef)ctxRef,
                                           (JSValueRef)valueRef, &exception);
    if (s != nullptr) {
      std::u16string u16str{(const char16_t*)JSStringGetCharactersPtr(s), JSStringGetLength(s)};
      JSStringRelease(s);
      return u16str;
    }
  }
  return std::u16string{};
}

Dart_Handle JSValue::toStringCopy(long ctxRef, long valueRef) {
  JSValueRef exception = NULL;
  JSStringRef string = JSValueToStringCopy((JSContextRef)ctxRef,
                                           (JSValueRef)valueRef, &exception);
  if (string) {
    string = JSStringRetain(string);
  }
  return jsc::newCReturnValue(string, exception);
}

Dart_Handle JSValue::toObject(long ctxRef, long valueRef) {
  JSValueRef exception = NULL;
  JSObjectRef value =
      JSValueToObject((JSContextRef)ctxRef, (JSValueRef)valueRef, &exception);
  JSValueProtect((JSContextRef)ctxRef, value);
  return jsc::newCReturnValue(value, exception);
}

/* Garbage collection */
void JSValue::protect(long ctxRef, long valueRef) {
  JSValueProtect((JSContextRef)ctxRef, (JSValueRef)valueRef);
}
void JSValue::unprotect(long ctxRef, long valueRef) {
  JSValueUnprotect((JSContextRef)ctxRef, (JSValueRef)valueRef);
}

void JSValue::setException(long valueRef, long exceptionRefRef) {
  JSValueRef* exception = (JSValueRef*)exceptionRefRef;
  *exception = (JSValueRef)valueRef;
}
}  // namespace blink
