#include "JSValue.h"
#include "JavaScriptCore/JSContextRef.h"

#include "third_party/tonic/converter/dart_converter.h"
#include "third_party/tonic/dart_args.h"
#include "third_party/tonic/dart_library_natives.h"
#include "third_party/tonic/dart_microtask_queue.h"
#include "third_party/tonic/logging/dart_invoke.h"
#include "third_party/tonic/typed_data/dart_byte_data.h"
#include "third_party/tonic/converter/dart_converter.h"
#include "third_party/tonic/dart_binding_macros.h"
#include "third_party/tonic/dart_library_natives.h"


namespace blink{
// RefCountedDartWrappable 暂时没看懂这个用来干嘛的。。似乎是用来维持一堆ui引用的。。。。可能用得着吧，，，先放着
// IMPLEMENT_WRAPPERTYPEINFO(jsc, JSValue);

#define FOR_EACH_BINDING(V) \
  V(JSValue, getType)           \
  V(JSValue, isUndefined)          \
  V(JSValue, isNull)      \
  V(JSValue, isBoolean)     \
  V(JSValue, isNumber)     \
  V(JSValue, isString)     \
  V(JSValue, isArray)     \
  V(JSValue, isObject)     \
  V(JSValue, isDate)

FOR_EACH_BINDING(DART_NATIVE_CALLBACK)

void JSValue::RegisterNatives(tonic::DartLibraryNatives* natives) {
  natives->Register({FOR_EACH_BINDING(DART_REGISTER_NATIVE)});
}

    Dart_Handle JSValue::getType(JSContextRef ctxRef,  JSValueRef valueRef){
        JSValueGetType((JSContextRef)ctxRef, (JSValueRef)valueRef);
    }

    Dart_Handle JSValue::isUndefined(JSContextRef ctxRef,  JSValueRef valueRef){
        JSValueGetType((JSContextRef)ctxRef, (JSValueRef)valueRef);
    }

    Dart_Handle JSValue::isNull(JSContextRef ctxRef,  JSValueRef valueRef){
        JSValueGetType((JSContextRef)ctxRef, (JSValueRef)valueRef);
    }

    Dart_Handle JSValue::isBoolean(JSContextRef ctxRef,  JSValueRef valueRef){
        JSValueGetType((JSContextRef)ctxRef, (JSValueRef)valueRef);
    }

    Dart_Handle JSValue::isNumber(JSContextRef ctxRef,  JSValueRef valueRef){
        JSValueGetType((JSContextRef)ctxRef, (JSValueRef)valueRef);
    }

    Dart_Handle JSValue::isString(JSContextRef ctxRef,  JSValueRef valueRef){
        JSValueGetType((JSContextRef)ctxRef, (JSValueRef)valueRef);
    }

    Dart_Handle JSValue::isObject(JSContextRef ctxRef,  JSValueRef valueRef){
        JSValueGetType((JSContextRef)ctxRef, (JSValueRef)valueRef);
    }

    Dart_Handle JSValue::isArray(JSContextRef ctxRef,  JSValueRef valueRef){
        JSValueGetType((JSContextRef)ctxRef, (JSValueRef)valueRef);
    }

    Dart_Handle JSValue::isDate(JSContextRef ctxRef,  JSValueRef valueRef){
        JSValueGetType((JSContextRef)ctxRef, (JSValueRef)valueRef);
    }

    // void _getType(Dart_NativeArguments args) {
    //     tonic::DartCall(&getType, args);
    // }

// }

// void JSValue::RegisterNatives(tonic::DartLibraryNatives* natives) {
//   natives->Register({
//       {"_getType", _getType, 2, true},
//       {"JSValue_isUndefined", ScheduleFrame, 1, true},
//       {"JSValue_isNull", _SendPlatformMessage, 4, true},
//       {"JSValue_respondToPlatformMessage", _RespondToPlatformMessage, 3, true},
//       {"JSValue_render", Render, 2, true},
//       {"JSValue_updateSemantics", UpdateSemantics, 2, true},
//       {"JSValue_setIsolateDebugName", SetIsolateDebugName, 2, true},

//       {"JSValue_initJSContext",          _initJSContext,          1, true},
//       {"JSValue_releaseJSContext",       _releaseJSContext,       1, true},
//       {"JSValue_execJavaScript",         _execJavaScript,         3, true},
//       {"JSValue_evalJavaScript",         _evalJavaScript,         3, true},
//       {"JSValue_setGlobalVariable",      _setGlobalVariable,      3, true},
//       {"JSValue_invokeMethod",           _invokeMethod,           4, true},
//       {"JSValue_addJavaScriptInterface", _addJavaScriptInterface, 3, true},
//   });
// }

}

/* Comparing values */

// NATIVE(JSValue,jobject,isEqual) (PARAMS, jlong ctxRef, jlong a, jlong b)
// {
//     JSValueRef exception = NULL;

//     jclass ret = env->FindClass("org/liquidplayer/webkit/javascriptcore/JSValue$JNIReturnObject");
//     jmethodID cid = env->GetMethodID(ret,"<init>","()V");
//     jobject out = env->NewObject(ret, cid);

//     jfieldID fid = env->GetFieldID(ret ,"bool", "Z");

//     bool bret = JSValueIsEqual((JSContextRef) ctxRef, (JSValueRef)a, (JSValueRef)b,
//          &exception);

//     env->SetBooleanField( out, fid, bret);

//     fid = env->GetFieldID(ret , "exception", "J");
//     env->SetLongField( out, fid, (long)exception);

//     return out;
// }

// NATIVE(JSValue,jboolean,isStrictEqual) (PARAMS, jlong ctxRef, jlong a, jlong b)
// {
//     return (jboolean) JSValueIsStrictEqual((JSContextRef)ctxRef, (JSValueRef)a, (JSValueRef)b);
// }

// NATIVE(JSValue,jobject,isInstanceOfConstructor) (PARAMS, jlong ctxRef, jlong valueRef,
//     jlong constructor)
// {
//     JSValueRef exception = NULL;

//     jclass ret = env->FindClass("org/liquidplayer/webkit/javascriptcore/JSValue$JNIReturnObject");
//     jmethodID cid = env->GetMethodID(ret,"<init>","()V");
//     jobject out = env->NewObject(ret, cid);

//     jfieldID fid = env->GetFieldID(ret ,"bool", "Z");

//     bool bret = JSValueIsInstanceOfConstructor((JSContextRef) ctxRef, (JSValueRef)valueRef,
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
//     JSValueRef value = JSValueMakeBoolean((JSContextRef) ctx, (bool) boolean);
//     JSValueProtect((JSContextRef) ctx, value);
//     return (long)value;
// }

// NATIVE(JSValue,jlong,makeNumber) (PARAMS, jlong ctx, jdouble number)
// {
//     JSValueRef value = JSValueMakeNumber((JSContextRef) ctx, (double) number);
//     JSValueProtect((JSContextRef) ctx, value);
//     return (long)value;
// }

// NATIVE(JSValue,jlong,makeString) (PARAMS, jlong ctx, jlong stringRef)
// {
//     JSValueRef value = JSValueMakeString((JSContextRef) ctx, (JSStringRef) stringRef);
//     JSValueProtect((JSContextRef) ctx, value);
//     return (long)value;
// }

// /* Converting to and from JSON formatted strings */

// NATIVE(JSValue,jlong,makeFromJSONString) (PARAMS, jlong ctx, jlong stringRef)
// {
//     JSValueRef value = JSValueMakeFromJSONString((JSContextRef) ctx, (JSStringRef) stringRef);
//     JSValueProtect((JSContextRef) ctx, value);
//     return (long)value;
// }

// NATIVE(JSValue,jobject,createJSONString) (PARAMS, jlong ctxRef, jlong valueRef, jint indent)
// {
//     JSValueRef exception = NULL;

//     jclass ret = env->FindClass("org/liquidplayer/webkit/javascriptcore/JSValue$JNIReturnObject");
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
//     return (jboolean) JSValueToBoolean((JSContextRef)ctx, (JSValueRef)valueRef);
// }

// NATIVE(JSValue,jobject,toNumber) (PARAMS, jlong ctxRef, jlong valueRef)
// {
//     JSValueRef exception = NULL;

//     jclass ret = env->FindClass("org/liquidplayer/webkit/javascriptcore/JSValue$JNIReturnObject");
//     jmethodID cid = env->GetMethodID(ret,"<init>","()V");
//     jobject out = env->NewObject(ret, cid);

//     jfieldID fid = env->GetFieldID(ret , "number", "D");

//     jdouble dret = JSValueToNumber((JSContextRef)ctxRef, (JSValueRef)valueRef, &exception);

//     env->SetDoubleField( out, fid, dret);

//     fid = env->GetFieldID(ret , "exception", "J");
//     env->SetLongField( out, fid, (long) exception);

//     return out;
// }

// NATIVE(JSValue,jobject,toStringCopy) (PARAMS, jlong ctxRef, jlong valueRef)
// {
//     JSValueRef exception = NULL;

//     jclass ret = env->FindClass("org/liquidplayer/webkit/javascriptcore/JSValue$JNIReturnObject");
//     jmethodID cid = env->GetMethodID(ret,"<init>","()V");
//     jobject out = env->NewObject(ret, cid);

//     jfieldID fid = env->GetFieldID(ret , "reference", "J");

//     JSStringRef string = JSValueToStringCopy((JSContextRef)ctxRef, (JSValueRef)valueRef, &exception);
//     if (string)
//         string = JSStringRetain(string);

//     env->SetLongField( out, fid, (long)string);

//     fid = env->GetFieldID(ret , "exception", "J");
//     env->SetLongField( out, fid, (long) exception);

//     return out;
// }

// NATIVE(JSValue,jobject,toObject) (PARAMS, jlong ctxRef, jlong valueRef)
// {
//     JSValueRef exception = NULL;

//     jclass ret = env->FindClass("org/liquidplayer/webkit/javascriptcore/JSValue$JNIReturnObject");
//     jmethodID cid = env->GetMethodID(ret,"<init>","()V");
//     jobject out = env->NewObject(ret, cid);

//     jfieldID fid = env->GetFieldID(ret , "reference", "J");

//     JSObjectRef value = JSValueToObject((JSContextRef)ctxRef, (JSValueRef)valueRef, &exception);
//     JSValueProtect((JSContextRef)ctxRef, value);

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

// NATIVE(JSValue,void,setException) (PARAMS, jlong valueRef, jlong exceptionRefRef)
// {
//     JSValueRef *exception = (JSValueRef *)exceptionRefRef;
//     *exception = (JSValueRef)valueRef;
// }
