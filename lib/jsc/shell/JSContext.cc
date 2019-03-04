//
// Created by liangting on 2019/2/26.
//

// #include <pthread.h>
// #include <stdio.h>
// #include <unistd.h>
#include "flutter/lib/jsc/shell/JSContext.h"
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
#include "flutter/lib/jsc/shell/jsc_utils.h"
// #include "flutter/fml/logging.h"


using tonic::ToDart;
namespace blink {
//todo log 
// static int pfd[2];
// static pthread_t thr;

// static void *thread_func(void*)
// {
//     ssize_t rdsz;
//     char buf[128];
//     while((rdsz = read(pfd[0], buf, sizeof buf - 1)) > 0) {
//         if(buf[rdsz - 1] == '\n') --rdsz;
//         buf[rdsz - 1] = 0;  /* add null-terminator */
//         // __android_log_write(ANDROID_LOG_DEBUG, "JavaScriptCore", buf);
//     }
//     return 0;
// }


// IMPLEMENT_WRAPPERTYPEINFO(jsc, JSContext);
IMPLEMENT_WRAPPERTYPEINFO(jsc, JSContextGroup);

#define FOR_EACH_BINDING(V) \
  V(JSContext, staticInit)  \
  V(JSContext, create)   \
  V(JSContext, createInGroup)        \
  V(JSContext, retain)     \
  V(JSContext, release)      \
  V(JSContext, getGlobalObject)      \
  V(JSContext, evaluateScript)       \
  V(JSContext, checkScriptSyntax)      \
  V(JSContext, garbageCollect) 

DART_BIND_ALL_STATIC(JSContext,FOR_EACH_BINDING);
// FOR_EACH_BINDING(DART_NATIVE_CALLBACK);
// FOR_EACH_BINDING(DART_NATIVE_CALLBACK_STATIC);
// void JSContext::RegisterNatives(tonic::DartLibraryNatives* natives) {
//   natives->Register(
//       {FOR_EACH_BINDING(DART_REGISTER_NATIVE_STATIC_)});
// }

// for JSContextGroup
#define FOR_EACH_BINDING1(V) \
  V(JSContextGroup, create)       \
  V(JSContextGroup, retain)       \
  V(JSContextGroup, release)       \
  V(JSContextGroup, getGroup)       

DART_BIND_ALL_STATIC(JSContextGroup, FOR_EACH_BINDING1)
JSContext::JSContext() = default;
JSContext::~JSContext() = default;
JSContextGroup::JSContextGroup() = default;
JSContextGroup::~JSContextGroup() = default;

void JSContext::staticInit() {
    // /* make stdout line-buffered and stderr unbuffered */
    // setvbuf(stdout, 0, _IOLBF, 0);
    // setvbuf(stderr, 0, _IONBF, 0);

    // /* create the pipe and redirect stdout and stderr */
    // pipe(pfd);
    // dup2(pfd[1], 1);
    // dup2(pfd[1], 2);

    // /* spawn the logging thread */
    // if(pthread_create(&thr, 0, thread_func, 0) == -1)
    //     return; // fail silently
    // pthread_detach(thr);
}

long JSContext::create() {
    JSGlobalContextRef ref = JSGlobalContextCreate((JSClassRef) NULL);
    //JSGlobalContextRetain(ref);
    return (long)ref;
}

long JSContext::createInGroup(long group) {
     JSGlobalContextRef ref = JSGlobalContextCreateInGroup((JSContextGroupRef)group,
                            (JSClassRef) NULL);
    //JSGlobalContextRetain(ref);
    return (long)ref;
}

long JSContext::retain(long ctxRef){
    return (long) JSGlobalContextRetain((JSGlobalContextRef) ctxRef);
}

void JSContext::release(long ctxRef){
    JSGlobalContextRelease((JSGlobalContextRef) ctxRef);
}

long JSContext::getGlobalObject(long ctxRef){
    JSObjectRef ref = JSContextGetGlobalObject((JSContextRef) ctxRef);
    JSValueProtect((JSContextRef)ctxRef,ref);
    return (long)ref;
}

long JSContextGroup::create() {
    JSContextGroupRef group = JSContextGroupCreate();
    JSContextGroupRetain(group);
    return (long)group;
}

long JSContextGroup::retain(long group){
    return (long) JSContextGroupRetain((JSContextGroupRef)group);
}

void JSContextGroup::release(long group){
    JSContextGroupRelease((JSContextGroupRef) group);
}

long JSContextGroup::getGroup(long ctx){
    JSContextGroupRef group = JSContextGetGroup((JSContextRef) ctx);
    JSContextGroupRetain(group);
    return (long)group;
}

Dart_Handle JSContext::evaluateScript(long ctx, long script,
    long thisObject, long sourceURL, int startingLineNumber){
    JSValueRef exception = NULL;
    JSValueRef value = JSEvaluateScript(
        (JSContextRef)ctx,
        (JSStringRef)script,
        (JSObjectRef)thisObject,
        (JSStringRef)sourceURL,
        startingLineNumber,
        &exception);
    JSValueProtect((JSContextRef)ctx, value);
    FML_LOG(ERROR)<< "JSContext::evaluateScript" << value;

    return jsc::newCReturnValue(value,exception);
}

Dart_Handle JSContext::checkScriptSyntax(long ctx, long script,
    long thisObject, long sourceURL, int startingLineNumber){
    JSValueRef exception = NULL;
   bool value = JSCheckScriptSyntax(
        (JSContextRef)ctx,
        (JSStringRef)script,
        (JSStringRef)sourceURL,
        startingLineNumber,
        &exception);
    return jsc::newCReturnValue(value,exception);
}
void JSContext::garbageCollect(long ctxRef){
    JSGarbageCollect((JSContextRef)ctxRef);
}
}
