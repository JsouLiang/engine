//
// Created by liangting on 2019/1/30.
//

#ifndef JSC_JSVALUE_H
#define JSC_JSVALUE_H

#include "third_party/dart/runtime/include/dart_api.h"
#include "JavaScriptCore/JSContextRef.h"

#include "flutter/lib/ui/dart_wrapper.h"
// #include "third_party/tonic/dart_wrappable.h"

namespace tonic {
class DartLibraryNatives;
}  // namespace tonic
namespace blink {

class JSValue {
// :public tonic::DartWrappable{
// : public RefCountedDartWrappable<JSValue> {
  // DEFINE_WRAPPERTYPEINFO();
  //下面两个是RefCountedThreadSafe 保证构造/析构私有
  // FML_FRIEND_REF_COUNTED_THREAD_SAFE(JSValue)
  // FML_FRIEND_MAKE_REF_COUNTED(JSValue)
 public:
  JSValue();

  ~JSValue() ;
  // static fml::RefPtr<JSValue> Create() {
    // return fml::MakeRefCounted<JSValue>();
  // }
  
  static void RegisterNatives(tonic::DartLibraryNatives* natives);

  static int getType(long ctxRef, long valueRef);
  static bool isUndefined(long ctxRef, long valueRef);
  static bool isNull(long ctxRef, long valueRef);
  static bool isBoolean(long ctxRef, long valueRef);
  static bool isNumber(long ctxRef, long valueRef);
  static bool isString(long ctxRef, long valueRef);
  static bool isObject(long ctxRef, long valueRef);

  static bool isArray(long ctxRef, long valueRef);
  static bool isDate(long ctxRef, long valueRef);

  /* Comparing values */

  static Dart_Handle isEqual(long ctxRef,
                      long valueRef1,
                      long ValueRef2);
  static bool isStrictEqual(long ctxRef, long a, long b);

  static Dart_Handle isInstanceOfConstructor(long ctxRef,
                                      long valueRef,
                                      long constructor);
  /* Creating values */
  static long makeUndefined(long ctxRef);
  static long makeNull(long ctxRef);
  static long makeBoolean(long ctxRef, bool value);
  static long makeNumber(long ctxRef, double value);
  static long makeString(long ctxRef, long stringRef);

  /* Converting to and from JSON formatted strings */
  static long makeFromJSONString(long ctxRef, long stringRef);
  static Dart_Handle createJSONString(long ctxRef, long valueRef, int indent);

  /* Converting to primitive values */
  static bool toBoolean(long ctxRef, long valueRef);
  static Dart_Handle toNumber(long ctxRef, long valueRef);
  static Dart_Handle toStringCopy(long ctxRef, long valueRef);
  static Dart_Handle toObject(long ctxRef, long valueRef);

  /* Garbage collection */
  static void protect(long ctxRef, long valueRef);
  static void unprotect(long ctxRef, long valueRef);
  static void setException(long valueRef, long exceptionRefRef);
  static std::u16string toStringValue(long ctxRef, long valueRef);

};
}  // namespace blink

#endif  // JSC_JSVALUE_H
