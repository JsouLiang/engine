//
// Created by 李少杰 on 2018/6/15.
//

#ifndef ENGINE_JS_EXECUTOR_H
#define ENGINE_JS_EXECUTOR_H

#include <string>
#include <unordered_map>
#include "third_party/dart/runtime/include/dart_api.h"
#include "JavaScriptCore/JSContextRef.h"

class JSExecutor {

public:
  void ExecJavaScript(const std::u16string &script, const std::u16string &sourceUrl, std::string &exception);

  void SetGlobalVariable(const std::u16string &name, Dart_Handle value, std::string &exception);

  Dart_Handle
  EvalJavaScript(const std::u16string &script, const std::u16string &sourceUrl, std::string &exception);

  Dart_Handle InvokeMethod(const std::u16string &objName, const std::u16string &function,
                           Dart_Handle arguments, std::string &exception);

  void AddJavaScriptInterface(const std::u16string &name, Dart_Handle handler, std::string &exception);

  JSExecutor();

  ~JSExecutor();

  JSValueRef OnJavaScriptCall(JSObjectRef thisObject, JSObjectRef function, JSValueRef const *argv, size_t argc);

  static JSExecutor *GetExecutor(JSContextRef context) {
    return context_executor_map_[JSContextGetGlobalContext(context)];
  }

private:

  static std::unordered_map<JSGlobalContextRef, JSExecutor *> context_executor_map_;

  JSGlobalContextRef context_;
  std::unordered_map<JSObjectRef, Dart_PersistentHandle> object_dart_map_;

};

#endif //ENGINE_JS_EXECUTOR_H
