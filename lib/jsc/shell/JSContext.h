//
// Created by liangting on 2019/2/26.
//

#ifndef JSC_JSCONTEXT_H
#define JSC_JSCONTEXT_H

#include "flutter/lib/ui/dart_wrapper.h"
#include "third_party/dart/runtime/include/dart_api.h"
// #include "third_party/tonic/dart_wrappable.h"

namespace tonic {
class DartLibraryNatives;
}  // namespace tonic
namespace blink {

class JSContext {
// {// :public tonic::DartWrappable{ Object has been disposed.
// : public RefCountedDartWrappable<JSContext>{
  // DEFINE_WRAPPERTYPEINFO();
  // FML_FRIEND_MAKE_REF_COUNTED(JSContext);

 public:
  JSContext();
  // ~JSContext();
  ~JSContext();
  // static fml::RefPtr<JSContext> Create() {
  //   return fml::MakeRefCounted<JSContext>();
  // }
  static void RegisterNatives(tonic::DartLibraryNatives* natives);

  static void staticInit();
  static long create();
  static long createInGroup(long group);
  static long retain(long ctxRef);
  static void release(long ctxRef);
  static long getGlobalObject(long ctxRef);

  static Dart_Handle evaluateScript(long ctx,
                             long script,
                             long thisObject,
                             long sourceURL,
                             int startingLineNumber);

  static Dart_Handle checkScriptSyntax(long ctx,
                                long script,
                                long thisObject,
                                long sourceURL,
                                int startingLineNumber);
  static void garbageCollect(long ctxRef);
};

class JSContextGroup// :  public tonic::DartWrappable {
  : public RefCountedDartWrappable<JSContextGroup> {
  DEFINE_WRAPPERTYPEINFO();
  // FML_FRIEND_MAKE_REF_COUNTED(JSContextGroup);

 public:
  JSContextGroup() ;

  ~JSContextGroup() override;
  static void RegisterNatives(tonic::DartLibraryNatives* natives);

  static long create();
  static long retain(long group);
  static void release(long group);
  static long getGroup(long ctx);
};
}  // namespace blink

#endif  // JSC_JSCONTEXT_H
