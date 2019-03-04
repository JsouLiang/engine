#include "flutter/lib/jsc/shell/JSString.h"
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

long JSString::createWithCharacters(const std::u16string& str) {
    JSStringRef string = JSStringRetain(JSStringCreateWithCharacters(reinterpret_cast<const JSChar *>(str.data()), str.length()));
    return (long)string;
}

long JSString::createWithUTF8CString(std::string& str) {
    JSStringRef string = JSStringRetain(JSStringCreateWithUTF8CString(reinterpret_cast<const char*>(str.data())));
    return (long)string;
}

long JSString::retain(long strRef) {
    return (long) JSStringRetain((JSStringRef)strRef);
}

void JSString::release(long strRef) {
    JSStringRelease((JSStringRef)strRef);
}

long JSString::getLength(long strRef) {
  //TODO SIZE_T to long
    return (long)(JSStringGetLength((JSStringRef)strRef));
}

const char * JSString::toString(long strRef) {
    return reinterpret_cast<const char *>(JSStringGetCharactersPtr((JSStringRef)strRef));
    // char *buffer = new char[JSStringGetMaximumUTF8CStringSize((JSStringRef)stringRef)+1];
    // JSStringGetUTF8CString((JSStringRef)stringRef, buffer,
    //     JSStringGetMaximumUTF8CStringSize((JSStringRef)stringRef)+1);
    // jstring ret = env->NewStringUTF(buffer);
    // delete buffer;
    // return ret;
}

long JSString::getMaximumUTF8CStringSize(long strRef) {
    return (long) JSStringGetMaximumUTF8CStringSize((JSStringRef)strRef);
}

bool JSString::isEqual(long strRef,long strRef1 ) {
    return (bool) JSStringIsEqual((JSStringRef)strRef, (JSStringRef)strRef1);
}

bool JSString::isEqualToUTF8CString(long strRef,std::string& str) {
    return (bool) JSStringIsEqualToUTF8CString((JSStringRef)strRef, str.data());
}


#define FOR_EACH_BINDING(V) \
  V(JSString, createWithCharacters)       \
  V(JSString, createWithUTF8CString)   \
  V(JSString, retain)        \
  V(JSString, release)     \
  V(JSString, getLength)      \
  V(JSString, toString)      \
  V(JSString, getMaximumUTF8CStringSize)      \
  V(JSString, isEqual)      \
  V(JSString, isEqualToUTF8CString)
 
DART_BIND_ALL_STATIC(JSString, FOR_EACH_BINDING)

}  // namespace blink
