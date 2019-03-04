part of dart.jsc;


class JSValue {
  int valueRef = 0;
  JSContext context;
  bool isDefunct = false;

  /* Constructors */
  ///   * Creates a new JavaScript value from a Java value.  Classes supported are:
  ///   * Boolean, Double, Integer, int, String, and JSString.  Any other object will
  ///   * generate an undefined JavaScript value.
  ///   * @param ctx  The context in which to create the value
  ///   * @param val  The Java value
  JSValue([this.context, final Object val]) {
    if (context == null) {
      ///
      /// Called only by convenience subclasses.  If you use
      /// this, you must set context and valueRef yourself.
      ///
      return;
    }
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
    } else if (val is int) {
      valueRef = makeNumber(context.ctxRef(), val.toDouble());
    } else if (val is String) {
      JSString s = new JSString(val);
      valueRef = makeString(context.ctxRef(), s.stringRef);
    } else {
      valueRef = makeUndefined(context.ctxRef());
    }
  }

  /// Wraps an existing JavaScript value
  /// @param valueRef  The JavaScriptCore reference to the value
  /// @param ctx  The context in which the value exists
  /// @since 1.0
  JSValue.fromValueRef(final int valueRef, this.context) {
    if (valueRef == 0) {
      this.valueRef = makeUndefined(context.ctxRef());
    } else {
      this.valueRef = valueRef;
      protect(context.ctxRef(), valueRef);
    }
  }

  ///todo dart 没有finalize这个机制
  ///
  void dispose() {
//    super.dispose();
    _unprotect();
  }

  /* Testers */

  /// Tests whether the value is undefined
  /// @return  true if undefined, false otherwise
  /// @since 1.0
  bool isUndefined() {
    return _isUndefined(context.ctxRef(), valueRef);
  }

  ///
  /// Tests whether the value is null
  /// @return  true if null, false otherwise
  /// @since 1.0
  ///
  bool isNull() {
    return _isNull(context.ctxRef(), valueRef);
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

  ///todo —— JSValueIsArray/JSValueIsDate 只有 OSX 10.11+ iOS 9.0+支持 android 目前美团里的jsc里没有
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
  bool operator ==(dynamic other) {
    return isEqual(other);
  }

  ///
  /// JavaScript definition of equality (==).  JSValue.equals() and JSValue.isEqual() represent
  /// the Java and JavaScript definitions, respectively.  Normally they will return the same
  /// value, however some classes may override and offer different results.  Example,
  /// in JavaScript, new Float32Array([1,2,3]) == new Float32Array([1,2,3]) will be false (as
  /// the equality is only true if they are the same physical object), but from a Java util.java.List
  /// perspective, these two are equal.
  /// @param other the value to compare for equality
  /// @return true if == from JavaScript perspective, false otherwise
  ///
  bool isEqual(Object other) {
    if (other == this) return true;
    JSValue otherJSValue;
    if (other is JSValue) {
      otherJSValue = other;
    } else {
      otherJSValue = new JSValue(context, other);
    }
    final JSValue ojsv = otherJSValue;
    CReturnValue crv = _isEqual(context.ctxRef(), valueRef, ojsv.valueRef);
    return crv.exception == 0 && crv.boolean;
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
  /* Getters */
  /// Gets the Boolean value of this JS value
  /// @return  the Boolean value
  bool toBoolean() {
    return _toBoolean(context.ctxRef(), valueRef);
  }

  /// Gets the numeric value of this JS value
  /// @return  The numeric value
  /// @since 1.0
  double toNumber() {
  CReturnValue cReturnValue = _toNumber(context.ctxRef(), valueRef);
  if (cReturnValue.exception!=0) {
  context.throwJSException(JSException.fromJSValue(JSValue.fromValueRef(cReturnValue.exception, context)));
  return 0.0;
  }
  return cReturnValue.number;
  }

  @override
  String toString() {
  try {
  return toJSString().toString();
  }  on JSException catch (e)  {
    return e.toString();
  }
  }
  /// Gets the JSString value of this JS value
  /// @return  The JSString value
  JSString toJSString() {
  CReturnValue cReturnValue = _toStringCopy(context.ctxRef(), valueRef);
  if (cReturnValue.exception!=0) {
  context.throwJSException(JSException.fromJSValue(JSValue.fromValueRef(cReturnValue.exception, context)));
  return null;
  }
  return  JSString(cReturnValue.reference);
  }
  /// If the JS value is an object, gets the JSObject
  /// @return  The JSObject for this value
  JSObject toObject() {
    CReturnValue cReturnValue = _toObject(context.ctxRef(), valueRef);
  if (cReturnValue.exception!=0) {
  context.throwJSException( JSException.fromJSValue(JSValue.fromValueRef(cReturnValue.exception, context)));
  return  JSObject(context);
  }
  return context.getObjectFromRef(cReturnValue.reference);
  }
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
//  context.throwJSException( JSException(context, "JSObject not a function"));
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
//  context.throwJSException( JSException(context, "JSObject not an array"));
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
//  JNIReturnClass runnable =  JNIReturnClass() {
//  @Override
//  public void run() {
//  jni = createJSONString(context.ctxRef(), valueRef, indent);
//  }
//  };
//  context.sync(runnable);
//  if (cReturnValueexception!=0) {
//  context.throwJSException( JSException( JSValue(cReturnValueexception, context)));
//  return null;
//  }
//  if (cReturnValuereference==0) {
//  return null;
//  }
//  return  JSString(cReturnValuereference).toString();
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
//  return  JSObjectPropertiesMap(toObject(),Object.class);
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

  @override
  int get hashCode {
    if (isBoolean()) return toBoolean().hashCode;
    else if (isNumber()) return toNumber().hashCode;
    else if (isString()) return toString().hashCode;
    else if (isUndefined() || isNull()) return 0;
    else return super.hashCode;
  }

  /// Gets the JSContext of this value
  /// @return the JSContext of this value
  /// @since 1.0
  JSContext getContext() {
    return context;
  }

  void _unprotect() {
    if (_isProtected && !context.isDefunct)
      context.markForUnprotection(valueRef);
    _isProtected = false;
  }

//  void realUnprotect(){}

  bool _isProtected = true;

  /* Native functions */
  static int _getType(int ctxRef, int valueRef) native 'JSValue_getType';

  static bool _isUndefined(int ctxRef, int valueRef) native 'JSValue_isUndefined';

  static bool _isNull(int ctxRef, int valueRef) native 'JSValue_isNull';

  static bool _isBoolean(int ctxRef, int valueRef) native 'JSValue_isBoolean';

  static bool _isNumber(int ctxRef, int valueRef) native 'JSValue_isNumber';

  static bool _isString(int ctxRef, int valueRef) native 'JSValue_isString';

  static bool _isObject(int ctxRef, int valueRef) native 'JSValue_isObject';

  //static  bool _isArray(int ctxRef, int valueRef) native 'JSValue_isArray';

  //static  bool _isDate(int ctxRef, int valueRef) native 'JSValue_isDate';

  static CReturnValue _isEqual(int ctxRef, int a, int b) native 'JSValue_isEqual';

  static bool _isStrictEqual(int ctxRef, int a, int b) native 'JSValue_isStrictEqual';

  static CReturnValue _isInstanceOfConstructor(int ctxRef, int valueRef,
      int constructor) native 'JSValue_isInstanceOfConstructor';

  static int makeUndefined(int ctx) native 'JSValue_makeUndefined';

  static int makeNull(int ctx) native 'JSValue_makeNull';

  static int makeBoolean(int ctx, bool bool) native 'JSValue_makeBoolean';

  static int makeNumber(int ctx, double number) native 'JSValue_makeNumber';

  static int makeString(int ctx, int stringRef) native 'JSValue_makeString';

  static int makeFromJSONString(int ctx, int stringRef)
      native 'JSValue_makeFromJSONString';

  static CReturnValue createJSONString(int ctxRef, int valueRef, int indent)
      native 'JSValue_createJSONString';

  static bool _toBoolean(int ctx, int valueRef) native 'JSValue_toBoolean';

  static CReturnValue _toNumber(int ctxRef, int valueRef) native 'JSValue_toNumber';

  static CReturnValue _toStringCopy(int ctxRef, int valueRef)
      native 'JSValue_toStringCopy';

  static CReturnValue _toObject(int ctxRef, int valueRef) native 'JSValue_toObject';

  static void protect(int ctx, int valueRef) native 'JSValue_protect';

  static void unprotect(int ctx, int valueRef) native 'JSValue_unprotect';

  static void _setException(int valueRef, int exceptionRefRef)
      native 'JSValue_setException';
}

/// Used in communicating with JavaScriptCore JNI.
/// Clients do not need to use this.
class CReturnValue extends NativeFieldWrapperClass4  {
  /// The boolean return value
  bool boolean;

  /// The numeric return value
  double number;

  /// The reference return value
  int reference;

  /// The exception reference if one was thrown, otherwise 0L
  int exception;
}

CReturnValue getCReturnValue(){
  

}
class JSString {
  int stringRef;

  ///
  /// Creates a JavaScript string from a Java string
  /// OR  Wraps an existing JavaScript string
  /// @param s  The Java string with which to initialize the JavaScript string
  /// @param stringRef  The JavaScriptCore reference to the string
  ///
  JSString(final dynamic s) {
    if (s == null) {
      stringRef = 0;
    } else if (s is int) {
      stringRef = s;
    } else {
      stringRef = createWithCharacters(s);
    }
  }

//  @Override
//  protected void finalize() throws Throwable {
//  super.finalize();
//  if (stringRef != 0)
//  release(stringRef);
//  }

  @override
  String toString() {
    return _toString(stringRef);
  }

  //native function 
  static int createWithCharacters(String str) native 'JSString_createWithCharacters';

  static int retain(int strRef) native 'JSString_retain';

  static void release(int stringRef) native 'JSString_release';

  static bool isEqual(int a, int b) native 'JSString_isEqual';

  static String _toString(int strRef) native 'JSString_toString';

  static int getLength(int stringRef) native 'JSString_getLength';

  static int getMaximumUTF8CStringSize(int stringRef)
      native 'JSString_getMaximumUTF8CStringSize';

  static int createWithUTF8CString(String str) native 'JSString_createWithUTF8CString';

  static bool isEqualToUTF8CString(int a, String b)
      native 'JSString_isEqualToUTF8CString';
}
