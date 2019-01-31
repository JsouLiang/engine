//
// Created by liangting on 2019/1/30.
//

#ifndef JSC_JSVALUE_H
#define JSC_JSVALUE_H
#include "third_party/dart/runtime/include/dart_api.h"

namespace tonic {
class DartLibraryNatives;
}  // namespace tonic
namespace blink{

class JSValue{
    public :
        JSValue();

        ~JSValue();

        static void RegisterNatives(tonic::DartLibraryNatives* natives);

        Dart_Handle getType(JSContextRef ctxRef,  JSValueRef valueRef);

        Dart_Handle isUndefined(JSContextRef ctxRef,  JSValueRef valueRef);

        Dart_Handle isNull(JSContextRef ctxRef,  JSValueRef valueRef);

        Dart_Handle isBoolean(JSContextRef ctxRef,  JSValueRef valueRef);

        Dart_Handle isNumber(JSContextRef ctxRef,  JSValueRef valueRef);

        Dart_Handle isString(JSContextRef ctxRef,  JSValueRef valueRef);
        Dart_Handle isObject(JSContextRef ctxRef,  JSValueRef valueRef);

        Dart_Handle isArray(JSContextRef ctxRef,  JSValueRef valueRef);

        Dart_Handle isDate(JSContextRef ctxRef,  JSValueRef valueRef);


};
}

#endif //JSC_JSVALUE_H
