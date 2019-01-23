//
// Created by 李少杰 on 2018/6/15.
//

#include <sstream>
#include "third_party/tonic/converter/dart_converter.h"
#include "js_executor.h"
#include "JSRetainPtr1.h"
#include "flutter/fml/logging.h"

std::unordered_map<JSGlobalContextRef, JSExecutor *> JSExecutor::context_executor_map_;

namespace {
JSStringRef ERROR_STACK_PROPERTY_NAME = JSStringCreateWithUTF8CString("stack");

class JSStringWrapper {
public:
  JSStringWrapper(JSContextRef context, JSValueRef jsValue) :
      strValue_(Adopt, JSValueToStringCopy(context, jsValue, nullptr)) {
  }

  JSStringWrapper(const std::u16string &str) : strValue_(Adopt, JSStringCreateWithCharacters(
      reinterpret_cast<const JSChar *>(str.data()), str.length())) {
  }

  JSStringRef ValueRef() const {
    return strValue_.get();
  }

  const std::string &str() {
    if (str_.length()) {
      return str_;
    }
    size_t len = JSStringGetMaximumUTF8CStringSize(strValue_.get());
    std::unique_ptr<char[]> utf8Buf = std::make_unique<char[]>(len);
    JSStringGetUTF8CString(strValue_.get(), utf8Buf.get(), len);
    str_ = std::string(utf8Buf.get());
    return str_;
  }

  const std::u16string &u16str() {
    if (u16str_.length()) {
      return u16str_;
    }
    size_t len = JSStringGetLength(strValue_.get());
    const JSChar *jsStrPtr = JSStringGetCharactersPtr(strValue_.get());
    u16str_ = std::u16string(reinterpret_cast<const char16_t *>(jsStrPtr), len);
    return u16str_;
  }

private:
  JSRetainPtr1<JSStringRef> strValue_;
  std::string str_;
  std::u16string u16str_;
};

void collectJSCExceptionInfo(const JSContextRef context, const JSValueRef exp, std::string &info) {
  JSValueProtect(context, exp);
  JSObjectRef expObj = JSValueToObject(context, exp, nullptr);
  std::ostringstream output;
  output << "JavaScript exception:" << std::endl << JSStringWrapper(context, expObj).str() << std::endl;
  if (JSObjectHasProperty(context, expObj, ERROR_STACK_PROPERTY_NAME)) {
    JSValueRef stackObj = JSObjectGetProperty(context, expObj, ERROR_STACK_PROPERTY_NAME, nullptr);
    if (stackObj && !JSValueIsNull(context, stackObj)) {
      output << JSStringWrapper(context, stackObj).str() << std::endl;
    }
    output << "<native stack>";
    info = output.str();
    JSValueUnprotect(context, exp);
  }
}

JSValueRef
nativeCallback(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount,
               const JSValueRef arguments[], JSValueRef *exception) {
  JSExecutor *executor = JSExecutor::GetExecutor(ctx);
  if (executor) {
    return executor->OnJavaScriptCall(thisObject, function, arguments, argumentCount);
  }
//  JSValueRef args[1] = {JSValueMakeString(ctx, JSStringCreateWithUTF8CString("No Executor found for this context."))};
//  exception = JSObjectMakeError(ctx, 1, args, nullptr);
  return nullptr;
}

Dart_Handle JSToDart(JSContextRef context, JSValueRef value) {
  Dart_Handle handle;
  if (JSValueIsNull(context, value) || JSValueIsUndefined(context, value)) {
    handle = Dart_Null();
  } else if (JSValueIsBoolean(context, value)) {
    handle = tonic::ToDart(JSValueToBoolean(context, value));
  } else if (JSValueIsNumber(context, value)) {
    handle = tonic::ToDart(JSValueToNumber(context, value, nullptr));
  } else if (JSValueIsString(context, value)) {
    handle = tonic::ToDart(JSStringWrapper(context, value).u16str());
  } else {
    handle = Dart_Null();
    FML_LOG(ERROR) << "Unsupported JSValue type";
    FML_DCHECK(false);
  }
  return handle;
}

JSValueRef DartToJS(JSContextRef context, Dart_Handle handle) {
  JSValueRef value;
  if (Dart_IsNull(handle)) {
    value = JSValueMakeNull(context);
  } else if (Dart_IsBoolean(handle)) {
    bool b;
    Dart_BooleanValue(handle, &b);
    value = JSValueMakeBoolean(context, b);
  } else if (Dart_IsDouble(handle)) {
    double d;
    Dart_DoubleValue(handle, &d);
    value = JSValueMakeNumber(context, d);
  } else if (Dart_IsString(handle)) {
    const std::u16string &str = tonic::DartConverter<std::u16string>::FromDart(handle);
    value = JSValueMakeString(context, JSStringWrapper(str).ValueRef());
//  } else if (Dart_IsMap(handle)) {
//    JSObjectRef json = JSObjectMake(context, nullptr, nullptr);
//    Dart_Handle keys = Dart_MapKeys(handle);
//    long keyCount;
//    Dart_ListLength(keys, &keyCount);
//    for (int i = 0; i < keyCount; ++i) {
//      Dart_Handle key = Dart_ListGetAt(keys, i);
//      JSObjectSetProperty(context, json, DartToJSString(context, key),
//                          DartToJS(context, Dart_MapGetAt(handle, key)),
//                          nullptr, nullptr);
//    }
//    value;
//  It's very hard to make a list or map for JSValue.
  } else {
    value = JSValueMakeNull(context);

#ifndef NDEBUG
    const char *typeName = "unknown";
    if (Dart_IsInteger(handle)) {
      typeName = "int(use double instead)";
    } else if (Dart_IsList(handle)) {
      typeName = "List";
    } else if (Dart_IsMap(handle)) {
      typeName = "Map";
    } else if (Dart_IsFunction(handle)) {
      typeName = "Function";
    } else if (Dart_IsClosure(handle)) {
      typeName = "Closure";
    }
    FML_LOG(ERROR) << "Unsupported Dart_Handle type: " << typeName;
    FML_DCHECK(false);
#endif
  }
  return value;
}


}

void
JSExecutor::ExecJavaScript(const std::u16string &script, const std::u16string &sourceUrl, std::string &exception) {
  JSValueRef exp = nullptr;
  JSEvaluateScript(context_, JSStringWrapper(script).ValueRef(), nullptr, JSStringWrapper(sourceUrl).ValueRef(), 0,
                   &exp);
  if (exp) {
    collectJSCExceptionInfo(context_, exp, exception);
  }
}

void JSExecutor::SetGlobalVariable(const std::u16string &name, Dart_Handle value, std::string &exception) {
  JSStringRef jsName = JSStringCreateWithCharacters(reinterpret_cast<const JSChar *>(name.data()), name.length());
  JSObjectSetProperty(context_, JSContextGetGlobalObject(context_),
                      jsName,
                      DartToJS(context_, value),
                      0, nullptr);
  JSStringRelease(jsName);
}

Dart_Handle JSExecutor::InvokeMethod(const std::u16string &objName, const std::u16string &function,
                                     Dart_Handle arguments, std::string &exception) {
  JSObjectRef thisObj = JSContextGetGlobalObject(context_);
  JSValueRef exp = nullptr;
  if (!objName.empty()) {
    JSValueRef propValue = JSObjectGetProperty(context_, thisObj, JSStringWrapper(objName).ValueRef(), &exp);
    if (exp) return collectJSCExceptionInfo(context_, exp, exception), Dart_Null();
    thisObj = JSValueToObject(context_, propValue, &exp);
    if (exp) return collectJSCExceptionInfo(context_, exp, exception), Dart_Null();
  }
  JSValueRef funValue = JSObjectGetProperty(context_, thisObj, JSStringWrapper(function).ValueRef(), &exp);
  if (exp) return collectJSCExceptionInfo(context_, exp, exception), Dart_Null();
  JSObjectRef funObj = JSValueToObject(context_, funValue, &exp);
  if (exp) return collectJSCExceptionInfo(context_, exp, exception), Dart_Null();
  std::vector<JSValueRef> args;
  if (!Dart_IsList(arguments)) {
    args.push_back(DartToJS(context_, arguments));
  } else {
    intptr_t len;
    Dart_ListLength(arguments, &len);
    for (intptr_t i = 0; i < len; ++i) {
      args.push_back(DartToJS(context_, Dart_ListGetAt(arguments, i)));
    }
  }
  JSValueRef ret = JSObjectCallAsFunction(context_, funObj, thisObj, args.size(), args.data(), &exp);
  if (exp) return collectJSCExceptionInfo(context_, exp, exception), Dart_Null();
  return JSToDart(context_, ret);
}

Dart_Handle
JSExecutor::EvalJavaScript(const std::u16string &script, const std::u16string &sourceUrl, std::string &exception) {
  JSValueRef exp = nullptr;
  JSValueRef retValueRef = JSEvaluateScript(context_, JSStringWrapper(script).ValueRef(), nullptr,
                                            JSStringWrapper(sourceUrl).ValueRef(), 0, &exp);
  if (exp) {
    collectJSCExceptionInfo(context_, exp, exception);
  } else {
    return JSToDart(context_, retValueRef);
  }
  return Dart_Null();
}

void JSExecutor::AddJavaScriptInterface(const std::u16string &name, Dart_Handle handler, std::string &exception) {
  Dart_PersistentHandle persistHandle = Dart_NewPersistentHandle(handler);
  JSObjectRef global = JSContextGetGlobalObject(context_);
  JSStringWrapper nameWrapper(name);
  JSStringRef jsName = nameWrapper.ValueRef();
  JSObjectRef jsFunctionObj = JSObjectMakeFunctionWithCallback(context_, nullptr, nativeCallback);
  JSObjectSetProperty(context_, global, jsName, jsFunctionObj, 0, nullptr);
  object_dart_map_[jsFunctionObj] = persistHandle;
}

JSExecutor::~JSExecutor() {
  context_executor_map_.erase(context_);
  JSGlobalContextRelease(context_);
}

JSExecutor::JSExecutor() :
    context_(JSGlobalContextCreateInGroup(nullptr, nullptr)) {
  context_executor_map_[context_] = this;
}

JSValueRef
JSExecutor::OnJavaScriptCall(JSObjectRef thisObject, JSObjectRef function, JSValueRef const *argv, size_t argc) {
  Dart_PersistentHandle dartHandle = object_dart_map_[function];
  if (!dartHandle) {
    return JSValueMakeNull(context_);
  }
  Dart_Handle dartArgList = Dart_NewList(argc);
  for (size_t i = 0; i < argc; i++) {
    Dart_ListSetAt(dartArgList, i, JSToDart(context_, argv[i]));
  }
  Dart_Handle dartArgs[1] = {dartArgList};
  return DartToJS(context_, Dart_InvokeClosure(dartHandle, 1, dartArgs));
}
