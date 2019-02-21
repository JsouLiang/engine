part of dart.jsc;

class JSValue {
//
//  protected static class JSWorkerQueue {
//  public JSWorkerQueue(final Runnable monitor) {
//  mMonitor = monitor;
//  }
//  final Runnable mMonitor;
//
//  private class JSTask extends AsyncTask<Runnable, Void, JSException> {
//  @Override
//  public JSException doInBackground(Runnable ... params) {
//  try {
//  params[0].run();
//  mMonitor.run();
//  } catch (JSException e) {
//  return e;
//  }
//  return null;
//  }
//  }
//
//  public void sync(final Runnable runnable) {
//  if (Looper.myLooper() == Looper.getMainLooper()) {
//  try {
//  JSException e = new JSTask().executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, runnable).get();
//  if (e != null) throw e;
//  } catch (ExecutionException e) {
//  Log.e("JSWorkerQueue", e.getMessage());
//  } catch (InterruptedException e) {
//  Thread.interrupted();
//  }
//  } else {
//  runnable.run();
//  mMonitor.run();
//  }
//  }
//
//  public void async(final Runnable runnable) {
//  if (Looper.myLooper() == Looper.getMainLooper()) {
//  new JSTask().executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, runnable);
//  } else {
//  runnable.run();
//  mMonitor.run();
//  }
//  }
//
//  public void quit() {
//
//  }
//  }
//
//  
  int valueRef = 0;
  JSContext context;
  bool isDefunct = false;

  /* Constructors */
  ///   * Creates a new JavaScript value from a Java value.  Classes supported are:
  ///   * Boolean, Double, Integer, int, String, and JSString.  Any other object will
  ///   * generate an undefined JavaScript value.
  ///   * @param ctx  The context in which to create the value
  ///   * @param val  The Java value

  JSValue([this.context,final Object val]) {
    if (context == null) {
      ///
      /// Called only by convenience subclasses.  If you use
      /// this, you must set context and valueRef yourself.
      ///
      return;
    }
// context.sync(new Runnable() {
// @Override
// public void run() {
  if (val == null) {
  valueRef = makeNull(context.ctxRef());
  } else if (val is JSValue) {
  valueRef = val.valueRef;
  protect(context.ctxRef(), valueRef);
  } else if (val is Map) {
    //todo map
//    valueRef = new JSObjectPropertiesMap(context, (Map)val, Object.class).getJSObject().valueRef();
//  protect(context.ctxRef(), valueRef);
  } else if (val is List) {
      //todo list
//  valueRef = new JSArray<>(context, (List) val, JSValue.class).valueRef();
//  protect(context.ctxRef(), valueRef);
//  } else if (val.getClass().isArray()) {
//  valueRef = new JSArray<>(context, (Object[])val, JSValue.class).valueRef();
//  protect(context.ctxRef(), valueRef);
  } else if (val is bool) {
  valueRef = makeBoolean(context.ctxRef(), val);
  } else if (val is double) {
  valueRef = makeNumber(context.ctxRef(), val);
  } else if (val is int ) {
  valueRef = makeNumber(context.ctxRef(), val.toDouble());
  } else if (val is String) {
  JSString s = new JSString((String)val);
  valueRef = makeString(context.ctxRef(), s.stringRef);
  } else {
  valueRef = makeUndefined(context.ctxRef());
  }
// }
// });
  }

  /// Wraps an existing JavaScript value
  /// @param valueRef  The JavaScriptCore reference to the value
  /// @param ctx  The context in which the value exists
  /// @since 1.0
  JSValue.fromValueRef(final int valueRef, JSContext ctx) {
    context = ctx;
    if (valueRef == 0) {
      this.valueRef = makeUndefined(context.ctxRef());
    }
    else {
      this.valueRef = valueRef;
      protect(context.ctxRef(), valueRef);
    }
  }

  ///todo dart 没有finalize这个机制
//  @Override
//  protected void finalize() throws Throwable {
//  super.finalize();
//  unprotect();
//  }
//
  /* Testers */
  /// Tests whether the value is undefined
  /// @return  true if undefined, false otherwise
  /// @since 1.0
  bool isUndefined() {
    return _isUndefined(context.ctxRef(), valueRef);
//  CReturnValue runnable = new JNIReturnClass() {
//  @Override
//  public void run() {
//  jni = new JNIReturnObject();
//  jni.bool = isUndefined(context.ctxRef(), valueRef);
//  }
//  };
//  context.sync(runnable);
//  return runnable.jni.bool;
  }

  ///
  /// Tests whether the value is null
  /// @return  true if null, false otherwise
  /// @since 1.0
  ///
  bool isNull() {
    return _isNull(context.ctxRef(), valueRef);
//  JNIReturnClass runnable = new JNIReturnClass() {
//  @Override
//  public void run() {
//  jni = new JNIReturnObject();
//  jni.bool = isNull(context.ctxRef(), valueRef);
//  }
//  };
//  context.sync(runnable);
//  return runnable.jni.bool;
  }

  /// Tests whether the value is boolean
  /// @return  true if boolean, false otherwise
  /// 
   bool isBoolean() {
     return _isBoolean(context.ctxRef(), valueRef);
  }

 ///
 /// Tests whether the value is a number
 /// @return  true if a number, false otherwise
 ///
 bool isNumber() {
 return _isNumber(context.ctxRef(), valueRef);
 }
  ///
   /// Tests whether the value is a string
   /// @return  true if a string, false otherwise
   ///
 bool isString() {
 return _isString(context.ctxRef(), valueRef);
 }
//   ///
//   /// Tests whether the value is an array
//   /// @return  true if an array, false otherwise
//   /// 
//  bool isArray() {
//  return _isArray(context.ctxRef(), valueRef);
//  }

//  /**
//   * Tests whether the value is a date object
//   * @return  true if a date object, false otherwise
//   */
//  bool isDate() {
//  return _isDate(context.ctxRef(), valueRef);
//  }

 ///
 /// Tests whether the value is an object
 /// @return  true if an object, false otherwise
 ///
 bool isObject() {
 return _isObject(context.ctxRef(), valueRef);
 }
//  /**
//   * Tests whether a value in an instance of a constructor object
//   * @param constructor  The constructor object to test
//   * @return  true if the value is an instance of the given constructor object, false otherwise
//   * @since 1.0
//   */
//  public Boolean isInstanceOfConstructor(final JSObject constructor) {
//  JNIReturnClass runnable = new JNIReturnClass() {
//  @Override
//  public void run() {
//  jni = isInstanceOfConstructor(context.ctxRef(), valueRef, constructor.valueRef());
//  }
//  };
//  context.sync(runnable);
//  if (runnable.jni.exception!=0) {
//  context.throwJSException(new JSException(new JSValue(runnable.jni.exception, context)));
//  runnable.jni.bool = false;
//  }
//  return runnable.jni.bool;
//  }
//
 /* Comparators */
@override
bool operator ==(other){
 return isEqual(other);
}

 /**
  * JavaScript definition of equality (==).  JSValue.equals() and JSValue.isEqual() represent
  * the Java and JavaScript definitions, respectively.  Normally they will return the same
  * value, however some classes may override and offer different results.  Example,
  * in JavaScript, new Float32Array([1,2,3]) == new Float32Array([1,2,3]) will be false (as
  * the equality is only true if they are the same physical object), but from a Java util.java.List
  * perspective, these two are equal.
  * @param other the value to compare for equality
  * @return true if == from JavaScript perspective, false otherwise
  * @since 3.0
  */
 bool isEqual(Object other) {
 if (other == this) return true;
 JSValue otherJSValue;
 if (other is JSValue) {
 otherJSValue = other;
 } else {
 otherJSValue = new JSValue(context, other);
 }
 final JSValue ojsv = otherJSValue;
 CReturnValue crv= _isEqual(context.ctxRef(), valueRef, ojsv.valueRef);
 return crv.exception==0 && crv.boolean;
 }

//  /**
//   * Tests whether two values are strict equal.  In JavaScript, equivalent to '===' operator.
//   * @param other  The value to test against
//   * @return  true if values are strict equal, false otherwise
//   * @since 1.0
//   */
//  public boolean isStrictEqual(Object other) {
//  if (other == this) return true;
//  JSValue otherJSValue;
//  if (other instanceof JSValue) {
//  otherJSValue = (JSValue)other;
//  } else {
//  otherJSValue = new JSValue(context, other);
//  }
//  final JSValue ojsv = otherJSValue;
//  JNIReturnClass runnable = new JNIReturnClass() {
//  @Override
//  public void run() {
//  jni = new JNIReturnObject();
//  jni.bool = isStrictEqual(context.ctxRef(), valueRef, ojsv.valueRef);
//  }
//  };
//  context.sync(runnable);
//  return runnable.jni.bool;
//  }
//
//  /* Getters */
//  /**
//   * Gets the Boolean value of this JS value
//   * @return  the Boolean value
//   * @since 1.0
//   */
//  public Boolean toBoolean() {
//  JNIReturnClass runnable = new JNIReturnClass() {
//  @Override
//  public void run() {
//  jni = new JNIReturnObject();
//  jni.bool = toBoolean(context.ctxRef(), valueRef);
//  }
//  };
//  context.sync(runnable);
//  return runnable.jni.bool;
//  }
//  /**
//   * Gets the numeric value of this JS value
//   * @return  The numeric value
//   * @since 1.0
//   */
//  public Double toNumber() {
//  JNIReturnClass runnable = new JNIReturnClass() {
//  @Override
//  public void run() {
//  jni = toNumber(context.ctxRef(), valueRef);
//  }
//  };
//  context.sync(runnable);
//  if (runnable.jni.exception!=0) {
//  context.throwJSException(new JSException(new JSValue(runnable.jni.exception, context)));
//  return 0.0;
//  }
//  return runnable.jni.number;
//  }
//  @Override
//  public String toString() {
//  try {
//  return toJSString().toString();
//  } catch (JSException e) {
//  return e.toString();
//  }
//  }
//  /**
//   * Gets the JSString value of this JS value
//   * @return  The JSString value
//   * @since 1.0
//   */
//  protected JSString toJSString() {
//  JNIReturnClass runnable = new JNIReturnClass() {
//  @Override
//  public void run() {
//  jni = toStringCopy(context.ctxRef(), valueRef);
//  }
//  };
//  context.sync(runnable);
//  if (runnable.jni.exception!=0) {
//  context.throwJSException(new JSException(new JSValue(runnable.jni.exception, context)));
//  return null;
//  }
//  return new JSString(runnable.jni.reference);
//  }
//  /**
//   * If the JS value is an object, gets the JSObject
//   * @return  The JSObject for this value
//   * @since 1.0
//   */
//  public JSObject toObject() {
//  JNIReturnClass runnable = new JNIReturnClass() {
//  @Override
//  public void run() {
//  jni = toObject(context.ctxRef(), valueRef);
//  }
//  };
//  context.sync(runnable);
//  if (runnable.jni.exception!=0) {
//  context.throwJSException(new JSException(new JSValue(runnable.jni.exception, context)));
//  return new JSObject(context);
//  }
//  return context.getObjectFromRef(runnable.jni.reference);
//  }
//
//  /**
//   * If the JS value is a function, gets the JSFunction
//   * @return  The JSFunction for this value
//   * @since 3.0
//   */
//  public JSFunction toFunction() {
//  if (isObject() && toObject() instanceof JSFunction) {
//  return (JSFunction)toObject();
//  } else if (!isObject()) {
//  toObject();
//  return null;
//  } else {
//  context.throwJSException(new JSException(context, "JSObject not a function"));
//  return null;
//  }
//  }
//
//  /**
//   * If the JS value is an array, gets the JSArray
//   * @return  The JSArray for this value
//   * @since 3.0
//   */
//  public JSBaseArray toJSArray() {
//  if (isObject() && toObject() instanceof JSBaseArray) {
//  return (JSBaseArray)toObject();
//  } else if (!isObject()) {
//  toObject();
//  return null;
//  } else {
//  context.throwJSException(new JSException(context, "JSObject not an array"));
//  return null;
//  }
//  }
//
//  /**
//   * Gets the JSON of this JS value
//   * @param indent  number of spaces to indent
//   * @return  the JSON representing this value, or null if value is undefined
//   * @since 1.0
//   */
//  public String toJSON(final int indent) {
//  JNIReturnClass runnable = new JNIReturnClass() {
//  @Override
//  public void run() {
//  jni = createJSONString(context.ctxRef(), valueRef, indent);
//  }
//  };
//  context.sync(runnable);
//  if (runnable.jni.exception!=0) {
//  context.throwJSException(new JSException(new JSValue(runnable.jni.exception, context)));
//  return null;
//  }
//  if (runnable.jni.reference==0) {
//  return null;
//  }
//  return new JSString(runnable.jni.reference).toString();
//  }
//  /**
//   * Gets the JSON of this JS value
//   * @return  the JSON representing this value
//   * @since 1.0
//   */
//  public String toJSON() {
//  return toJSON(0);
//  }
//
//  @SuppressWarnings("unchecked")
//  protected Object toJavaObject(Class clazz) {
//  if (clazz == Object.class)
//  return this;
//  else if (clazz == Map.class)
//  return new JSObjectPropertiesMap(toObject(),Object.class);
//  else if (clazz == List.class)
//  return toJSArray();
//  else if (clazz == String.class)
//  return toString();
//  else if (clazz == Double.class || clazz == double.class)
//  return toNumber();
//  else if (clazz == Float.class || clazz == float.class)
//  return toNumber().floatValue();
//  else if (clazz == Integer.class || clazz == int.class)
//  return toNumber().intValue();
//  else if (clazz == int.class || clazz == int.class)
//  return toNumber().intValue();
//  else if (clazz == Byte.class || clazz == byte.class)
//  return toNumber().byteValue();
//  else if (clazz == Short.class || clazz == short.class)
//  return toNumber().shortValue();
//  else if (clazz == Boolean.class || clazz == boolean.class)
//  return toBoolean();
//  else if (clazz.isArray())
//  return toJSArray().toArray(clazz.getComponentType());
//  else if (JSObject.class.isAssignableFrom(clazz))
//  return clazz.cast(toObject());
//  else if (JSValue.class.isAssignableFrom(clazz))
//  return clazz.cast(this);
//  return null;
//  }

//  @Override
//  int hashCode() {
//    if (isBoolean()) return toBoolean().hashCode();
//    else if (isNumber()) return toNumber().hashCode();
//    else if (isString()) return toString().hashCode();
//    else if (isUndefined() || isNull()) return 0;
//    else return super.hashCode();
//  }

  /// Gets the JSContext of this value
  /// @return the JSContext of this value
  /// @since 1.0
  JSContext getContext() {
    return context;
  }

  void unprotect() {
    if (_isProtected && !context.isDefunct)
      context.markForUnprotection(valueRef());
    _isProtected = false;
  }

  bool _isProtected = true;

  /* Native functions */
  int _getType(int ctxRef, int valueRef) native 'JSValue_getType';

  bool _isUndefined(int ctxRef, int valueRef) native 'JSValue_isUndefined';

  bool _isNull(int ctxRef, int valueRef) native 'JSValue_isNull';

  bool _isBoolean(int ctxRef, int valueRef) native 'JSValue_isBoolean';

  bool _isNumber(int ctxRef, int valueRef) native 'JSValue_isNumber';

  bool _isString(int ctxRef, int valueRef) native 'JSValue_isString';

  bool _isObject(int ctxRef, int valueRef) native 'JSValue_isObject';

  // bool _isArray(int ctxRef, int valueRef) native 'JSValue_isArray';

  // bool _isDate(int ctxRef, int valueRef) native 'JSValue_isDate';

  CReturnValue _isEqual(int ctxRef, int a, int b) native 'JSValue_isEqual';

  bool _isStrictEqual(int ctxRef, int a, int b) native 'JSValue_isStrictEqual';

  CReturnValue _isInstanceOfConstructor(int ctxRef, int valueRef,
      int constructor) native 'JSValue_isInstanceOfConstructor';

  int makeUndefined(int ctx) native 'JSValue_makeUndefined';

  int makeNull(int ctx) native 'JSValue_makeNull';

  int makeBoolean(int ctx, bool bool) native 'JSValue_makeBoolean';

  int makeNumber(int ctx, double number) native 'JSValue_makeNumber';

  int makeString(int ctx, int stringRef) native 'JSValue_makeString';

  int makeFromJSONString(int ctx,
      int stringRef) native 'JSValue_makeFromJSONString';

  CReturnValue createJSONString(int ctxRef, int valueRef,
      int indent) native 'JSValue_createJSONString';

  bool _toBoolean(int ctx, int valueRef) native 'JSValue_toBoolean';

  CReturnValue _toNumber(int ctxRef, int valueRef) native 'JSValue_toNumber';

  CReturnValue _toStringCopy(int ctxRef,
      int valueRef) native 'JSValue_toStringCopy';

  CReturnValue _toObject(int ctxRef, int valueRef) native 'JSValue_toObject';

  void protect(int ctx, int valueRef) native 'JSValue_protect';

  void _unprotect(int ctx, int valueRef) native 'JSValue_unprotect';

  void _setException(int valueRef,
      int exceptionRefRef) native 'JSValue_setException';
}

/// Used in communicating with JavaScriptCore JNI.
/// Clients do not need to use this.
class CReturnValue {

  /// The boolean return value
  bool boolean;

  /// The numeric return value
  double number;

  /// The reference return value
  int reference;

  /// The exception reference if one was thrown, otherwise 0L
  int exception;
}

class JSString {

// static final JSWorkerQueue workerQueue = new JSWorkerQueue(new Runnable() {
//  @Override
//  public void run() {
//  }
//  });

//  private abstract class JNIStringReturnClass implements Runnable {
//  String string;
//  }

@protected
int stringRef;

 /**
  * Creates a JavaScript string from a Java string
  * @param s  The Java string with which to initialize the JavaScript string
  * @since 1.0
  */
 public JSString(final String s) {
 if (s==null) stringRef = 0L;
 else {
 workerQueue.sync(new Runnable() {
 @Override
 public void run() {
 stringRef = createWithCharacters(s);
 }
 });
 }
 }
//  /**
//   * Wraps an existing JavaScript string
//   * @param stringRef  The JavaScriptCore reference to the string
//   */
//  public JSString(int stringRef) {
//  this.stringRef = stringRef;
//  }
//  @Override
//  protected void finalize() throws Throwable {
//  super.finalize();
//  if (stringRef != 0)
//  release(stringRef);
//  }

 @Override
 public String toString() {
 JNIStringReturnClass payload = new JNIStringReturnClass() {
 @Override
 public void run() {
 string = JSString.this.toString(stringRef);
 }
 };
 workerQueue.sync(payload);
 return payload.string;
 }

 /**
  * Gets the JavaScriptCore string reference
  * @return  the JavaScriptCore string reference
  * @since 1.0
  */
 public int stringRef() {
 return stringRef;
 }

 protected native int createWithCharacters(String str);
 protected native int retain(int strRef);
 protected native void release(int stringRef);
 protected native boolean isEqual(int a, int b);
 protected native String toString(int strRef);

 @SuppressWarnings("unused")
 protected native int getLength(int stringRef);
 @SuppressWarnings("unused")
 protected native int createWithUTF8CString(String str);
 @SuppressWarnings("unused")
 protected native int getMaximumUTF8CStringSize(int stringRef);
 @SuppressWarnings("unused")
 protected native boolean isEqualToUTF8CString(int a, String b);
 }

 private abstract class JNIReturnClass implements Runnable {
 JNIReturnObject jni;
 }
