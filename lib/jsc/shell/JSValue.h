//
// Created by liangting on 2019/1/30.
//

#ifndef JSC_JSVALUE_H
#define JSC_JSVALUE_H

#include "third_party/dart/runtime/include/dart_api.h"
// #include "flutter/lib/ui/dart_wrapper.h"

namespace tonic {
class DartLibraryNatives;
}  // namespace tonic
namespace blink{

class JSValue {
// : public RefCountedDartWrappable<JSValue> {
//   DEFINE_WRAPPERTYPEINFO();
//   FML_FRIEND_MAKE_REF_COUNTED(JSValue);

    public :
        JSValue();

        ~JSValue();

        static void RegisterNatives(tonic::DartLibraryNatives* natives);

        int getType(JSContextRef ctxRef,  JSValueRef valueRef);

        bool isUndefined(JSContextRef ctxRef,  JSValueRef valueRef);

        bool isNull(JSContextRef ctxRef,  JSValueRef valueRef);

        bool isBoolean(JSContextRef ctxRef,  JSValueRef valueRef);

        bool isNumber(JSContextRef ctxRef,  JSValueRef valueRef);

        bool isString(JSContextRef ctxRef,  JSValueRef valueRef);
        bool isObject(JSContextRef ctxRef,  JSValueRef valueRef);

        bool isArray(JSContextRef ctxRef,  JSValueRef valueRef);
        bool isDate(JSContextRef ctxRef,  JSValueRef valueRef);

        /* Comparing values */

        Dart_Handle JSValue::isEqual(JSContextRef ctxRef,  JSValueRef valueRef1, JSValueRef ValueRef2);
};
}

#endif //JSC_JSVALUE_H
