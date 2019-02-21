#include "flutter/lib/jsc/shell/JSValue.h"
#include "JavaScriptCore/JSContextRef.h"

#include "third_party/dart/runtime/include/dart_api.h"
#include "third_party/tonic/converter/dart_converter.h"
#include "third_party/tonic/dart_args.h"
#include "third_party/tonic/dart_binding_macros.h"
#include "third_party/tonic/dart_library_natives.h"
#include "third_party/tonic/dart_persistent_value.h"

// #include "third_party/tonic/dart_microtask_queue.h"
#include "third_party/tonic/logging/dart_invoke.h"
// #include "third_party/tonic/typed_data/dart_byte_data.h"
using tonic::ToDart;

namespace blink {
// RefCountedDartWrappable
// 暂时没看懂这个用来干嘛的。。似乎是用来维持一堆ui引用的。。。。可能用得着吧，，，先放着
// IMPLEMENT_WRAPPERTYPEINFO(jsc, JSValue);

#define FOR_EACH_BINDING(V) \
  V(JSValue, getType)       \
  V(JSValue, isUndefined)   \
  V(JSValue, isNull)        \
  V(JSValue, isBoolean)     \
  V(JSValue, isNumber)      \
  V(JSValue, isString)      \
  V(JSValue, isArray)       \
  V(JSValue, isObject)      \
  V(JSValue, isDate)        \
  V(JSValue, isEqual)

DART_BIND_ALL(JSValue, FOR_EACH_BINDING)

// FOR_EACH_BINDING(DART_NATIVE_CALLBACK)

// void JSValue::RegisterNatives(tonic::DartLibraryNatives* natives) {
//   natives->Register({FOR_EACH_BINDING(DART_REGISTER_NATIVE)});
// }

int JSValue::getType(JSContextRef ctxRef, JSValueRef valueRef) {
  return (int)(JSValueGetType((JSContextRef)ctxRef, (JSValueRef)valueRef));
}
bool JSValue::isUndefined(JSContextRef ctxRef, JSValueRef valueRef) {
  return JSValueIsUndefined((JSContextRef)ctxRef, (JSValueRef)valueRef);
}
bool JSValue::isNull(JSContextRef ctxRef, JSValueRef valueRef) {
  return JSValueIsNull((JSContextRef)ctxRef, (JSValueRef)valueRef);
}
bool JSValue::isBoolean(JSContextRef ctxRef, JSValueRef valueRef) {
  return JSValueIsBoolean((JSContextRef)ctxRef, (JSValueRef)valueRef);
}
bool JSValue::isNumber(JSContextRef ctxRef, JSValueRef valueRef) {
  return JSValueIsNumber((JSContextRef)ctxRef, (JSValueRef)valueRef);
}
bool JSValue::isString(JSContextRef ctxRef, JSValueRef valueRef) {
  return JSValueIsString((JSContextRef)ctxRef, (JSValueRef)valueRef);
}
bool JSValue::isObject(JSContextRef ctxRef, JSValueRef valueRef) {
  return JSValueIsObject((JSContextRef)ctxRef, (JSValueRef)valueRef);
}
// todo JSValueIsArray、JSValueIsDate支持版本问题 iOS 9.0+ OSX 10.11 Android
// jsc里不确定是否已经有了
// ref:https://github.com/WebKit/webkit/commit/a558e2d150c37a87222a83f8cd30f5d1be7d4355
// http://exbrowser.com/?p=660

// bool JSValue::isArray(JSContextRef ctxRef,  JSValueRef valueRef){
//     return JSValueIsArray((JSContextRef)ctxRef, (JSValueRef)valueRef);
// }
// bool JSValue::isDate(JSContextRef ctxRef,  JSValueRef valueRef){
//     return JSValueIsDate((JSContextRef)ctxRef, (JSValueRef)valueRef);
// }

/* Comparing values */

Dart_Handle JSValue::isEqual(JSContextRef ctxRef,
                             JSValueRef valueRef1,
                             JSValueRef valueRef2) {
  JSValueRef exception = NULL;

  bool is_equal = JSValueIsEqual((JSContextRef)ctxRef, (JSValueRef)valueRef1,
                 (JSValueRef)valueRef2, &exception);

  Dart_Handle jsc_lib = Dart_LookupLibrary(ToDart("dart:jsc"));
  DART_CHECK_VALID(jsc_lib);
  Dart_Handle type = Dart_GetType(jsc_lib, ToDart("CReturnValue"), 0, NULL);
  DART_CHECK_VALID(type);
  Dart_Handle result = Dart_New(type,Dart_Null(),0,NULL);
  result = Dart_SetField(result,ToDart("boolean"),Dart_NewBoolean(is_equal));
  result = Dart_SetField(result,ToDart("exception"),Dart_NewInteger(exception));

  return result;
}

// NATIVE(JSValue,jboolean,isStrictEqual) (PARAMS, jlong ctxRef, jlong a, jlong
// b)
// {
//     return (jboolean) JSValueIsStrictEqual((JSContextRef)ctxRef,
//     (JSValueRef)a, (JSValueRef)b);
// }

// NATIVE(JSValue,jobject,isInstanceOfConstructor) (PARAMS, jlong ctxRef, jlong
// valueRef,
//     jlong constructor)
// {
//     JSValueRef exception = NULL;

//     jclass ret =
//     env->FindClass("org/liquidplayer/webkit/javascriptcore/JSValue$JNIReturnObject");
//     jmethodID cid = env->GetMethodID(ret,"<init>","()V");
//     jobject out = env->NewObject(ret, cid);

//     jfieldID fid = env->GetFieldID(ret ,"bool", "Z");

//     bool bret = JSValueIsInstanceOfConstructor((JSContextRef) ctxRef,
//     (JSValueRef)valueRef,
//             (JSObjectRef)constructor, &exception);

//     env->SetBooleanField( out, fid, bret);

//     fid = env->GetFieldID(ret , "exception", "J");
//     env->SetLongField( out, fid, (long)exception);

//     return out;
// }

/* Creating values */

// NATIVE(JSValue,jlong,makeUndefined) (PARAMS, jlong ctx)
// {
//     JSValueRef value = JSValueMakeUndefined((JSContextRef) ctx);
//     JSValueProtect((JSContextRef) ctx, value);
//     return (long)value;
// }

// NATIVE(JSValue,jlong,makeNull) (PARAMS, jlong ctx)
// {
//     JSValueRef value = JSValueMakeNull((JSContextRef) ctx);
//     JSValueProtect((JSContextRef) ctx, value);
//     return (long)value;
// }

// NATIVE(JSValue,jlong,makeBoolean) (PARAMS, jlong ctx, jboolean boolean)
// {
//     JSValueRef value = JSValueMakeBoolean((JSContextRef) ctx, (bool)
//     boolean); JSValueProtect((JSContextRef) ctx, value); return (long)value;
// }

// NATIVE(JSValue,jlong,makeNumber) (PARAMS, jlong ctx, jdouble number)
// {
//     JSValueRef value = JSValueMakeNumber((JSContextRef) ctx, (double)
//     number); JSValueProtect((JSContextRef) ctx, value); return (long)value;
// }

// NATIVE(JSValue,jlong,makeString) (PARAMS, jlong ctx, jlong stringRef)
// {
//     JSValueRef value = JSValueMakeString((JSContextRef) ctx, (JSStringRef)
//     stringRef); JSValueProtect((JSContextRef) ctx, value); return
//     (long)value;
// }

// /* Converting to and from JSON formatted strings */

// NATIVE(JSValue,jlong,makeFromJSONString) (PARAMS, jlong ctx, jlong stringRef)
// {
//     JSValueRef value = JSValueMakeFromJSONString((JSContextRef) ctx,
//     (JSStringRef) stringRef); JSValueProtect((JSContextRef) ctx, value);
//     return (long)value;
// }

// NATIVE(JSValue,jobject,createJSONString) (PARAMS, jlong ctxRef, jlong
// valueRef, jint indent)
// {
//     JSValueRef exception = NULL;

//     jclass ret =
//     env->FindClass("org/liquidplayer/webkit/javascriptcore/JSValue$JNIReturnObject");
//     jmethodID cid = env->GetMethodID(ret,"<init>","()V");
//     jobject out = env->NewObject(ret, cid);

//     jfieldID fid = env->GetFieldID(ret , "reference", "J");
//     JSStringRef value = JSValueCreateJSONString(
//         (JSContextRef)ctxRef,
//         (JSValueRef)valueRef,
//         (unsigned)indent,
//         &exception);
//     if (value)
//         value = JSStringRetain(value);

//     env->SetLongField( out, fid, (long)value);

//     fid = env->GetFieldID(ret , "exception", "J");
//     env->SetLongField( out, fid, (long)exception);

//     return out;
// }

// /* Converting to primitive values */

// NATIVE(JSValue,jboolean,toBoolean) (PARAMS, jlong ctx, jlong valueRef)
// {
//     return (jboolean) JSValueToBoolean((JSContextRef)ctx,
//     (JSValueRef)valueRef);
// }

// NATIVE(JSValue,jobject,toNumber) (PARAMS, jlong ctxRef, jlong valueRef)
// {
//     JSValueRef exception = NULL;

//     jclass ret =
//     env->FindClass("org/liquidplayer/webkit/javascriptcore/JSValue$JNIReturnObject");
//     jmethodID cid = env->GetMethodID(ret,"<init>","()V");
//     jobject out = env->NewObject(ret, cid);

//     jfieldID fid = env->GetFieldID(ret , "number", "D");

//     jdouble dret = JSValueToNumber((JSContextRef)ctxRef,
//     (JSValueRef)valueRef, &exception);

//     env->SetDoubleField( out, fid, dret);

//     fid = env->GetFieldID(ret , "exception", "J");
//     env->SetLongField( out, fid, (long) exception);

//     return out;
// }

// NATIVE(JSValue,jobject,toStringCopy) (PARAMS, jlong ctxRef, jlong valueRef)
// {
//     JSValueRef exception = NULL;

//     jclass ret =
//     env->FindClass("org/liquidplayer/webkit/javascriptcore/JSValue$JNIReturnObject");
//     jmethodID cid = env->GetMethodID(ret,"<init>","()V");
//     jobject out = env->NewObject(ret, cid);

//     jfieldID fid = env->GetFieldID(ret , "reference", "J");

//     JSStringRef string = JSValueToStringCopy((JSContextRef)ctxRef,
//     (JSValueRef)valueRef, &exception); if (string)
//         string = JSStringRetain(string);

//     env->SetLongField( out, fid, (long)string);

//     fid = env->GetFieldID(ret , "exception", "J");
//     env->SetLongField( out, fid, (long) exception);

//     return out;
// }

// NATIVE(JSValue,jobject,toObject) (PARAMS, jlong ctxRef, jlong valueRef)
// {
//     JSValueRef exception = NULL;

//     jclass ret =
//     env->FindClass("org/liquidplayer/webkit/javascriptcore/JSValue$JNIReturnObject");
//     jmethodID cid = env->GetMethodID(ret,"<init>","()V");
//     jobject out = env->NewObject(ret, cid);

//     jfieldID fid = env->GetFieldID(ret , "reference", "J");

//     JSObjectRef value = JSValueToObject((JSContextRef)ctxRef,
//     (JSValueRef)valueRef, &exception); JSValueProtect((JSContextRef)ctxRef,
//     value);

//     env->SetLongField( out, fid, (long)value);

//     fid = env->GetFieldID(ret , "exception", "J");
//     env->SetLongField( out, fid, (long) exception);

//     return out;
// }

// /* Garbage collection */

// NATIVE(JSValue,void,protect) (PARAMS, jlong ctxRef, jlong valueRef)
// {
//     JSValueProtect((JSContextRef)ctxRef, (JSValueRef)valueRef );
// }

// NATIVE(JSValue,void,unprotect) (PARAMS, jlong ctxRef, jlong valueRef)
// {
//     JSValueUnprotect((JSContextRef)ctxRef, (JSValueRef)valueRef);
// }

// NATIVE(JSValue,void,setException) (PARAMS, jlong valueRef, jlong
// exceptionRefRef)
// {
//     JSValueRef *exception = (JSValueRef *)exceptionRefRef;
//     *exception = (JSValueRef)valueRef;
// }
}  // namespace blink
