//
// Created by liangting on 2019/1/30.
//

#ifndef JSC_JS_STRING_H
#define JSC_JS_STRING_H

#include "third_party/dart/runtime/include/dart_api.h"
#include "JavaScriptCore/JSContextRef.h"

#include "flutter/lib/ui/dart_wrapper.h"
// #include "third_party/tonic/dart_wrappable.h"

namespace tonic {
class DartLibraryNatives;
}  // namespace tonic
namespace blink {

class JSString {
// :public tonic::DartWrappable{
// : public RefCountedDartWrappable<JSValue> {
  // DEFINE_WRAPPERTYPEINFO();
  //下面两个是RefCountedThreadSafe 保证构造/析构私有
  // FML_FRIEND_REF_COUNTED_THREAD_SAFE(JSValue)
  // FML_FRIEND_MAKE_REF_COUNTED(JSValue)
 public:
  JSString();

  ~JSString();

  static void RegisterNatives(tonic::DartLibraryNatives* natives);

  static long createWithCharacters(const std::u16string& str);
  static long createWithUTF8CString(std::string& str);
  static long retain(long strRef);
  static void release(long strRef);
  static long getLength(long strRef);
  static const char * toString(long strRef);
  static long getMaximumUTF8CStringSize(long strRef);
  static bool isEqual(long strRef,long strRef1 );
  static bool isEqualToUTF8CString(long strRef,std::string& str) ;
};
}  // namespace blink

#endif  // JSC_JS_STRING_H
