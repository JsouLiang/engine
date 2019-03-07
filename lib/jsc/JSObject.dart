part of dart.jsc;

///  
///  A JavaScript object.
/// 
class JSObject extends JSValue {

//  private abstract class JNIReturnClass implements Runnable {
//  JNIReturnObject jni;
//  }

  /// Specifies that a property has no special attributes.
  static const int JSPropertyAttributeNone = 0;

  /// Specifies that a property is read-only.
  static int JSPropertyAttributeReadOnly = 1 << 1;

  /// Specifies that a property should not be enumerated by
  /// JSPropertyEnumerators and JavaScript for...in loops.
  static int JSPropertyAttributeDontEnum = 1 << 2;

  /// Specifies that the delete operation should fail on a property.
  static int JSPropertyAttributeDontDelete = 1 << 3;

  ///
  ///  Creates a new, empty JavaScript object.  In JS:
  ///  <pre>
  ///  {@code
  ///  var obj = {}; // OR
  ///  var obj = new Object();
  ///}
  ///</pre>
  ///
  ///@param ctx The JSContext to create the object in
  ///
  JSObject([JSContext ctx]) {
    if (ctx == null) {
      ///
      /// Called only by convenience subclasses.  If you use
      /// this, you must set context and valueRef yourself.
      /// @since 3.0
      ///
      return;
    }
    context = ctx;
    valueRef = make(context.ctxRef(), 0);
    context.persistObject(this);
  }
//
//  get string(String name) {
//    return property(name).toString();
//  }
//
//
//
//  /// JSObject()
//  /// {a: 1, b:2, c: ""}
//  /// JSObject().number["a"] = 1;
//  ///          ..number["b"] = 2;
  JSProperty<String> string;
  JSProperty<JSObject> object;
  JSProperty<JSNumber> number;

  ///
  /// Wraps an existing object from JavaScript
  ///
  /// @param objRef The JavaScriptCore object reference
  /// @param ctx    The JSContext of the reference
  /// @since 1.0
  ///
  JSObject.fromValueRef(final int objRef, JSContext ctx)
      : super.fromValueRef(objRef, ctx) {
    context.persistObject(this);
  }

//
//  ///
//  /// Creates a new object with function properties set for each method
//  /// in the defined interface.
//  /// In JS:
//  /// <pre>
//  /// {@code
//  /// var obj = {
//  ///     func1: function(a)   { alert(a); },
//  ///     func2: function(b,c) { alert(b+c); }
//  /// };
//  /// }
//  /// </pre>
//  /// Where func1, func2, etc. are defined in interface 'iface'.  This JSObject
//  /// must implement 'iface'.
//  ///
//  /// @param ctx   The JSContext to create the object in
//  /// @param iface The Java Interface defining the methods to expose to JavaScript
//  ///
//  JSObject(JSContext ctx, final Class<?> iface) {
//  context = ctx;
////  context.sync(new Runnable() {
////  @Override
////  public void run() {
////  valueRef = make(context.ctxRef(), 0L);
////  Method[] methods = iface.getDeclaredMethods();
////  for (Method m : methods) {
////  JSObject f = new JSFunction(context, m,
////  JSObject.class, JSObject.this);
////  property(m.getName(), f);
////  }
////  }
////  });
//  context.persistObject(this);
//  }
//
//  ///
//  /// Creates a new function object with the entries in 'map' set as properties.
//  ///
//  /// @param ctx  The JSContext to create object in
//  /// @param map  The map containing the properties
//  ///
//  @SuppressWarnings("unchecked")
//  JSObject(JSContext ctx, final Map map) {
//  this(ctx);
//  new JSObjectPropertiesMap<>(this,Object.class).putAll(map);
//  }

  ///
  /// Determines if the object contains a given property
  ///
  /// @param prop The property to test the existence of
  /// @return true if the property exists on the object, false otherwise
  ///
  bool hasProperty(String prop) {
    return _hasProperty(
        context.ctxRef(), valueRef, new JSString(prop).stringRef);
  }

  ///
  /// Gets the property named 'prop'
  ///
  /// @param prop The name of the property to fetch
  /// @return The JSValue of the property, or null if it does not exist
  ///
  JSValue getProperty(final String prop) {
    final CReturnValue cReturnValue = _getProperty(
        context.ctxRef(), valueRef, new JSString(prop).stringRef);
    if (cReturnValue.exception != 0) {
      context.throwJSException(new JSException.fromJSValue(
          new JSValue.fromValueRef(cReturnValue.exception, context)));
      return new JSValue(context);
    }
    return new JSValue.fromValueRef(cReturnValue.reference, context);
  }

  ///
  /// Sets the value of property 'prop'
  ///
  /// @param prop       The name of the property to set
  /// @param value      The Java object to set.  The Java object will be converted to a JavaScript object
  ///                   automatically.
  /// @param attributes And OR'd list of JSProperty constants
  ///
  void setProperty(final String prop, final Object value,
      [final int attributes = JSPropertyAttributeNone ]) {
    JSString name = new JSString(prop);
    final CReturnValue cReturnValue = _setProperty(
        context.ctxRef(), valueRef, name.stringRef,
        (value is JSValue) ? value.valueRef : JSValue(context, value)
            .valueRef,
        attributes);
    if (cReturnValue.exception != 0) {
      context.throwJSException(JSException.fromJSValue(
          JSValue.fromValueRef(cReturnValue.exception, context)));
    }
  }


  ///
  /// Deletes a property from the object
  ///
  /// @param prop The name of the property to delete
  /// @return true if the property was deleted, false otherwise
  ///
  bool deleteProperty(final String prop) {
    JSString name = JSString(prop);
    final CReturnValue cReturnValue = _deleteProperty(
        context.ctxRef(), valueRef, name.stringRef);
    if (cReturnValue.exception != 0) {
      context.throwJSException(JSException.fromJSValue(
          JSValue.fromValueRef(cReturnValue.exception, context)));
      return false;
    }
    return cReturnValue.boolean;
  }

  ///
  /// Returns the property at index 'index'.  Used for arrays.
  ///
  /// @param index The index of the property
  /// @return The JSValue of the property at index 'index'
  ///
  JSValue setPropertyAtIndex(final int index) {
    final CReturnValue cReturnValue = _getPropertyAtIndex(
        context.ctxRef(), valueRef, index);
    if (cReturnValue.exception != 0) {
      context.throwJSException(JSException.fromJSValue(
          JSValue.fromValueRef(cReturnValue.exception, context)));
      return JSValue(context);
    }
    return JSValue.fromValueRef(cReturnValue.reference, context);
  }

  ///
  /// Sets the property at index 'index'.  Used for arrays.
  ///
  /// @param index The index of the property to set
  /// @param value The Java object to set, will be automatically converted to a JavaScript value
  ///
  void getPropertyAtIndex(final int index, final Object value) {
    final CReturnValue cReturnValue = _setPropertyAtIndex(
        context.ctxRef(), valueRef, index,
        (value is JSValue) ? value.valueRef : JSValue(context, value)
            .valueRef);
    if (cReturnValue.exception != 0) {
      context.throwJSException(JSException.fromJSValue(
          JSValue.fromValueRef(cReturnValue.exception, context)));
    }
  }

//  private abstract class StringArrayReturnClass implements Runnable {
//  String[] sArray;
//  }

//  ///
//  /// Gets the list of set property names on the object
//  ///
//  /// @return A string array containing the property names
//  ///
//  String[] propertyNames() {
//  StringArrayReturnClass runnable = StringArrayReturnClass() {
//  @Override
//  public void run() {
//  int propertyNameArray = copyPropertyNames(context.ctxRef(), valueRef);
//  long[] refs = getPropertyNames(propertyNameArray);
//  String[] names = String[refs.length];
//  for (int i = 0; i < refs.length; i++) {
//  JSString name = JSString(refs[i]);
//  names[i] = name.toString();
//  }
//  releasePropertyNames(propertyNameArray);
//  sArray = names;
//  }
//  };
//  context.sync(runnable);
//  return runnable.sArray;
//  }

  ///
  /// Determines if the object is a function
  ///
  /// @return true if the object is a function, false otherwise
  ///
  bool isFunction() {
    return _isFunction(context.ctxRef(), valueRef);
  }

  ///
  /// Determines if the object is a constructor
  ///
  /// @return true if the object is a constructor, false otherwise
  ///
  bool isConstructor() {
    return _isConstructor(context.ctxRef(), valueRef);
  }

  @override
  int get hashCode {
    return valueRef;
  }

//  final List<JSObject> zombies = [];

//  @Override
//  void finalize() throws Throwable {
//  super.finalize();
//  context.finalizeObject(this);
//  }

  void setThis(JSObject thiz) {
    this._thiz = thiz;
  }

  JSObject getThis() {
    return _thiz;
  }

  JSValue __nullFunc() {
    return JSValue(context);
  }

//  protected JSFunction isInstanceOf = null;
  JSObject _thiz = null;

  /* Native Methods */

  int make(int ctx, int data) native 'JSObject_make';

  int makeInstance(int ctx) native 'JSObject_makeInstance';

  CReturnValue makeArray(int ctx, List<int> args) native 'JSObject_makeArray';

  CReturnValue makeDate(int ctx, List<int> args) native 'JSObject_makeDate';

  CReturnValue makeError(int ctx, List<int> args) native 'JSObject_makeError';

  CReturnValue makeRegExp(int ctx, List<int> args) native 'JSObject_makeRegExp';

  int getPrototype(int ctx, int object) native 'JSObject_getPrototype';

  void setPrototype(int ctx, int object,
      int value) native 'JSObject_setPrototype';

  bool _hasProperty(int ctx, int object,
      int propertyName) native 'JSObject_hasProperty';

  CReturnValue _getProperty(int ctx, int object,
      int propertyName) native 'JSObject_getProperty';

  CReturnValue _setProperty(int ctx, int object, int propertyName, int value,
      int attributes) native 'JSObject_setProperty';

  CReturnValue _deleteProperty(int ctx, int object,
      int propertyName) native 'JSObject_deleteProperty';

  CReturnValue _getPropertyAtIndex(int ctx, int object,
      int propertyIndex) native 'JSObject_getPropertyAtIndex';

  CReturnValue _setPropertyAtIndex(int ctx, int object, int propertyIndex,
      int value) native 'JSObject_setPropertyAtIndex';

  int getPrivate(int object) native 'JSObject_getPrivate';

  bool setPrivate(int object, int data) native 'JSObject_setPrivate';

  bool _isFunction(int ctx, int object) native 'JSObject_isFunction';

  CReturnValue callAsFunction(int ctx, int object,int thisObject, List<int> args) native 'JSObject_callAsFunction';

  bool _isConstructor(int ctx, int object) native 'JSObject_isConstructor';

  CReturnValue callAsConstructor(int ctx, int object,List<int> args) native 'JSObject_callAsConstructor';

  int copyPropertyNames(int ctx,
      int object) native 'JSObject_copyPropertyNames';

  List<int> getPropertyNames(
      int propertyNameArray) native 'JSObject_getPropertyNames';

  void releasePropertyNames(
      int propertyNameArray) native 'JSObject_releasePropertyNames';

  int makeFunctionWithCallback(int ctx,
      int name) native 'JSObject_makeFunctionWithCallback';

  CReturnValue makeFunction(int ctx, int name, List<int> parameterNames,
      int body, int sourceURL,
      int startingLineNumber) native 'JSObject_makeFunction';

}