part of dart.jsc;

/// Wraps a JavaScriptCore context
class JSContext extends JSObject {
  // // final JSWorkerQueue mWorker = new JSWorkerQueue(new Runnable() {
  // //     @Override
  // //     public void run() {
  // //         if (deadReferences.size() > 100) {
  // //             cleanDeadReferences();
  // //         }
  // //     }
  // // });

  // void sync(Runnable runnable) {
  //   mWorker.sync(runnable);
  // }

  // void async(Runnable runnable) {
  //   mWorker.async(runnable);
  // }

  // public final Object mMutex = new Object();

  final List<int> _deadReferences = new List();

  void markForUnprotection(int valueR) {
//       synchronized (mMutex) {
    _deadReferences.add(valueR);
//       }
  }

  void _cleanDeadReferences() {
//       synchronized (mMutex) {
    for (int reference in _deadReferences) {
      JSValue.unprotect(ctxRef(), reference);
    }
    _deadReferences.clear();
//       }
  }

  int ctx;
  IJSExceptionHandler exceptionHandler;

  ///
  /// Creates a new JavaScript context
  ///
  JSContext() {
    print('JSContext_constructor');
    // _constructor();
    context = this;
    _staticInit();
    ctx = create();
     print('ctx get $ctx');
    valueRef = getGlobalObject(ctx);
  }

  // void _constructor() native 'JSContext_constructor';


  // /**
  //  * Creates a new JavaScript context in the context group 'inGroup'.
  //  * @param inGroup  The context group to create the context in
  //  * @since 1.0
  //  */
  // public JSContext(final JSContextGroup inGroup) {
  //     context = this;
  //     sync(new Runnable() {
  //         @Override public void run() {
  //             static_init();
  //             ctx = createInGroup(inGroup.groupRef());
  //             valueRef = getGlobalObject(ctx);
  //         }
  //     });
  // }
  // /**
  //  * Creates a JavaScript context, and defines the global object with interface 'iface'.  This
  //  * object must implement 'iface'.  The methods in 'iface' will be exposed to the JavaScript environment.
  //  * @param iface  The interface to expose to JavaScript
  //  * @since 1.0
  //  */
  // public JSContext(final Class<?> iface) {
  //     context = this;
  //     sync(new Runnable() {
  //         @Override public void run() {
  //             static_init();
  //             ctx = create();
  //             valueRef = getGlobalObject(ctx);
  //             Method[] methods = iface.getDeclaredMethods();
  //             for (Method m : methods) {
  //                 JSObject f = new JSFunction(context, m, JSObject.class, context);
  //                 property(m.getName(),f);
  //             }
  //         }
  //     });
  // }
  // /**
  //  * Creates a JavaScript context in context group 'inGroup', and defines the global object
  //  * with interface 'iface'.  This object must implement 'iface'.  The methods in 'iface' will
  //  * be exposed to the JavaScript environment.
  //  * @param inGroup  The context group to create the context in
  //  * @param iface  The interface to expose to JavaScript
  //  * @since 1.0
  //  */
  // public JSContext(final JSContextGroup inGroup, final Class<?> iface) {
  //     context = this;
  //     sync(new Runnable() {
  //         @Override public void run() {
  //             static_init();
  //             ctx = createInGroup(inGroup.groupRef());
  //             valueRef = getGlobalObject(ctx);
  //             Method[] methods = iface.getDeclaredMethods();
  //             for (Method m : methods) {
  //                 JSObject f = new JSFunction(context, m, JSObject.class, context);
  //                 property(m.getName(),f);
  //             }
  //         }
  //     });
  // }
  // @Override
  // protected void finalize() throws Throwable {
  //     super.finalize();
  //     cleanDeadReferences();
  //     isDefunct = true;
  //     release(ctx);
  //     if (mWorker != null) {
  //         mWorker.quit();
  //     }
  // }

  ///
  /// Sets the JS exception handler for this context.  Any thrown JSException in this
  /// context will call the 'handle' method on this object.  The calling function will
  /// return with an undefined value.
  /// @param handler An object that implements 'IJSExceptionHandler'
  ///
  void setExceptionHandler(IJSExceptionHandler handler) {
    exceptionHandler = handler;
  }

  ///
  /// Clears a previously set exception handler.
  ///
  void clearExceptionHandler() {
    exceptionHandler = null;
  }

  ///
  /// If an exception handler is set, calls the exception handler, otherwise throws
  /// the JSException.
  /// @param exception The JSException to be thrown
  ///
  void throwJSException(JSException exception) {
    if (exceptionHandler == null) {
      throw exception;
    } else {
      // Before handling this exception, disable the exception handler.  If a JSException
      // is thrown in the handler, then it would recurse and blow the stack.  This way an
      // actual exception will get thrown.  If successfully handled, then turn it back on.
      IJSExceptionHandler temp = exceptionHandler;
      exceptionHandler = null;
      temp.handle(exception);
      exceptionHandler = temp;
    }
  }

  // /**
  //  * Gets the context group to which this context belongs.
  //  * @return  The context group to which this context belongs
  //  */
  // public JSContextGroup getGroup() {
  //     Long g = getGroup(ctx);
  //     if (g==0) return null;
  //     return new JSContextGroup(g);
  // }

  ///
  /// Gets the JavaScriptCore context reference
  /// @return  the JavaScriptCore context reference
  ///
  int ctxRef() {
    return ctx;
  }

  // private abstract class JNIReturnClass implements Runnable {
  //     JNIReturnObject jni;
  // }

  /// Executes a the JavaScript code in 'script' in this context
  /// @param script  The code to execute
  /// @param thiz  The 'this' object
  /// @param sourceURL  The URI of the source file, only used for reporting in stack trace (optional)
  /// @param startingLineNumber  The beginning line number, only used for reporting in stack trace (optional)
  /// @return  The return value returned by 'script'
  /// @since 1.0
  JSValue evaluateScript(final String script,
      [final JSObject thiz,
      final String sourceURL,
      final int startingLineNumber = 0]) {
    JSString jsscript = JSString(script);
    JSString jssourceURL = JSString(sourceURL);
    CReturnValue cReturnValue = _evaluateScript(
        ctx,
        jsscript.stringRef,
        (thiz == null) ? 0 : thiz.valueRef,
        jssourceURL.stringRef,
        startingLineNumber);
    print('cReturnValue');
    print(cReturnValue);
    print(cReturnValue.exception);
    if (cReturnValue.exception != 0) {
      throwJSException(JSException.fromJSValue(
          new JSValue.fromValueRef(cReturnValue.exception, context)));
      return JSValue(this);
    }
    return JSValue.fromValueRef(cReturnValue.reference, this);
  }

  ///todo 没有WeakReference 看看Expando
  Expando<JSObject> _objects = new Expando<JSObject>();

  ///
  /// Keeps a reference to an object in this context.  This is used so that only one
  /// Java object instance wrapping a JavaScript object is maintained at any time.  This way,
  /// local variables in the Java object will stay wrapped around all returns of the same
  /// instance.  This is handled by JSObject, and should not need to be called by clients.
  /// @param obj  The object with which to associate with this context
  ///
  void persistObject(JSObject obj) {
    _objects[obj.valueRef] = obj;
  }

  ///
  /// Removes a reference to an object in this context.  Should only be used from the 'finalize'
  /// object method.  This is handled by JSObject, and should not need to be called by clients.
  /// @param obj the JSObject to dereference
  ///
  void finalizeObject(JSObject obj) {
    _objects[obj.valueRef] = null;
  }

  /// Reuses a stored reference to a JavaScript object if it exists, otherwise, it creates the
  /// reference.
  /// @param objRef the JavaScriptCore object reference
  /// @param create whether to create the object if it does not exist
  /// @since 1.0
  /// @return The JSObject representing the reference
  JSObject getObjectFromRef(int objRef, [bool create = true]) {
    if (objRef == valueRef) {
      return this;
    }
    JSObject obj = _objects[objRef];
    if (obj != null) {
      JSValue.unprotect(ctxRef(), obj.valueRef);
    }
    if (obj == null && create) {
      obj = new JSObject.fromValueRef(objRef, this);
      // if (isArray(ctxRef(), objRef))
      // obj = new JSArray(objRef, this);
      // else if (JSTypedArray.isTypedArray(obj))
      // obj = JSTypedArray.from(obj);
      // else if (isFunction(ctxRef(), objRef)) obj = new JSFunction(objRef, this);
    }
    return obj;
  }

  // /**
  //  * Forces JavaScript garbage collection on this context
  //  * @since 1.0
  //  */
  // public void garbageCollect()
  // {
  //     async(new Runnable() {
  //         @Override
  //         public void run() {
  //             garbageCollect(ctx);
  //         }
  //     });
  // }

  static void staticInit() native 'JSContext_staticInit';

  static int create() native 'JSContext_create';

  // protected native long createInGroup(long group);
  // protected native long retain(long ctx);
  // protected native long release(long ctx);
  // protected native long getGroup(long ctx);
  static int getGlobalObject(int ctx) native 'JSContext_getGlobalObject';

  static CReturnValue _evaluateScript(int ctx, int script, int thisObject,
      int sourceURL, int startingLineNumber) native 'JSContext_evaluateScript';

  // @SuppressWarnings("unused")
  // protected native JNIReturnObject checkScriptSyntax(long ctx, long script, long sourceURL, int startingLineNumber);
  // protected native void garbageCollect(long ctx);

  static bool isInit = false;

  static void _staticInit() {
    if (!isInit) {
      staticInit();
      isInit = true;
    }
  }
}

///
/// Object interface for handling JSExceptions.
///
abstract class IJSExceptionHandler {
  ///
  /// Implement this method to catch JSExceptions
  /// @param exception caught exception
  ///
  void handle(JSException exception);
}
