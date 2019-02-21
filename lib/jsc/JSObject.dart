part of dart.jsc;

// import java.lang.reflect.Method;
// import java.util.ArrayList;
// import java.util.List;
// import java.util.Map;

///  
///  A JavaScript object.
/// 
class JSObject extends JSValue {

//  private abstract class JNIReturnClass implements Runnable {
//  JNIReturnObject jni;
//  }

  /// Specifies that a property has no special attributes.
  static int JSPropertyAttributeNone = 0;
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
    if(ctx == null){
      ///
      /// Called only by convenience subclasses.  If you use
      /// this, you must set context and valueRef yourself.
      /// @since 3.0
      ///
      return;
    }
  context = ctx;
//  context.sync(new Runnable() {
//  @Override
//  public void run() {
  valueRef = make(context.ctxRef(), 0);
//  }
//  });
  context.persistObject(this);
  }

//
//  ///
//  /// Wraps an existing object from JavaScript
//  ///
//  /// @param objRef The JavaScriptCore object reference
//  /// @param ctx    The JSContext of the reference
//  /// @since 1.0
//  ///
//  protected JSObject(final int objRef, JSContext ctx) {
//  super(objRef, ctx);
//  context.persistObject(this);
//  }
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
  bool hasProperty(final String prop) {
  JNIReturnClass runnable = new JNIReturnClass() {
  @Override
  public void run() {
  jni = new JNIReturnObject();
  jni.bool = hasProperty(context.ctxRef(), valueRef, new JSString(prop).stringRef());
  }
  };
  context.sync(runnable);
  return runnable.jni.bool;
  }

  ///
  /// Gets the property named 'prop'
  ///
  /// @param prop The name of the property to fetch
  /// @return The JSValue of the property, or null if it does not exist
  ///
  JSValue property(final String prop) {
  JNIReturnClass runnable = new JNIReturnClass() {
  @Override
  public void run() {
  jni = getProperty(context.ctxRef(), valueRef, new JSString(prop).stringRef());
  }
  };
  context.sync(runnable);
  if (runnable.jni.exception != 0) {
  context.throwJSException(new JSException(new JSValue(runnable.jni.exception, context)));
  return new JSValue(context);
  }
  return new JSValue(runnable.jni.reference, context);
  }

  ///
  /// Sets the value of property 'prop'
  ///
  /// @param prop       The name of the property to set
  /// @param value      The Java object to set.  The Java object will be converted to a JavaScript object
  ///                   automatically.
  /// @param attributes And OR'd list of JSProperty constants
  ///
  void property(final String prop, final Object value, final int attributes) {
  JNIReturnClass runnable = new JNIReturnClass() {
  @Override
  public void run() {
  JSString name = new JSString(prop);
  jni = setProperty(
  context.ctxRef(),
  valueRef,
  name.stringRef,
  (value instanceof JSValue) ? ((JSValue) value).valueRef() : new JSValue(context, value).valueRef(),
  attributes);
  }
  };
  context.sync(runnable);
  if (runnable.jni.exception != 0) {
  context.throwJSException(new JSException(new JSValue(runnable.jni.exception, context)));
  }
  }

  ///
  /// Sets the value of property 'prop'.  No JSProperty attributes are set.
  ///
  /// @param prop  The name of the property to set
  /// @param value The Java object to set.  The Java object will be converted to a JavaScript object
  ///              automatically.
  ///
  void property(String prop, Object value) {
  property(prop, value, JSPropertyAttributeNone);
  }

  ///
  /// Deletes a property from the object
  ///
  /// @param prop The name of the property to delete
  /// @return true if the property was deleted, false otherwise
  ///
  boolean deleteProperty(final String prop) {
  JNIReturnClass runnable = new JNIReturnClass() {
  @Override
  public void run() {
  JSString name = new JSString(prop);
  jni = deleteProperty(context.ctxRef(), valueRef, name.stringRef());
  }
  };
  context.sync(runnable);
  if (runnable.jni.exception != 0) {
  context.throwJSException(new JSException(new JSValue(runnable.jni.exception, context)));
  return false;
  }
  return runnable.jni.bool;
  }

  ///
  /// Returns the property at index 'index'.  Used for arrays.
  ///
  /// @param index The index of the property
  /// @return The JSValue of the property at index 'index'
  ///
  JSValue propertyAtIndex(final int index) {
  JNIReturnClass runnable = new JNIReturnClass() {
  @Override
  public void run() {
  jni = getPropertyAtIndex(context.ctxRef(), valueRef, index);
  }
  };
  context.sync(runnable);
  if (runnable.jni.exception != 0) {
  context.throwJSException(new JSException(new JSValue(runnable.jni.exception, context)));
  return new JSValue(context);
  }
  return new JSValue(runnable.jni.reference, context);
  }

  ///
  /// Sets the property at index 'index'.  Used for arrays.
  ///
  /// @param index The index of the property to set
  /// @param value The Java object to set, will be automatically converted to a JavaScript value
  ///
  void propertyAtIndex(final int index, final Object value) {
  JNIReturnClass runnable = new JNIReturnClass() {
  @Override
  public void run() {
  jni = setPropertyAtIndex(context.ctxRef(), valueRef, index,
  (value instanceof JSValue) ? ((JSValue) value).valueRef() : new JSValue(context, value).valueRef());
  }
  };
  context.sync(runnable);
  if (runnable.jni.exception != 0) {
  context.throwJSException(new JSException(new JSValue(runnable.jni.exception, context)));
  }
  }

  private abstract class StringArrayReturnClass implements Runnable {
  String[] sArray;
  }

  ///
  /// Gets the list of set property names on the object
  ///
  /// @return A string array containing the property names
  ///
  String[] propertyNames() {
  StringArrayReturnClass runnable = new StringArrayReturnClass() {
  @Override
  public void run() {
  int propertyNameArray = copyPropertyNames(context.ctxRef(), valueRef);
  long[] refs = getPropertyNames(propertyNameArray);
  String[] names = new String[refs.length];
  for (int i = 0; i < refs.length; i++) {
  JSString name = new JSString(refs[i]);
  names[i] = name.toString();
  }
  releasePropertyNames(propertyNameArray);
  sArray = names;
  }
  };
  context.sync(runnable);
  return runnable.sArray;
  }

  ///
  /// Determines if the object is a function
  ///
  /// @return true if the object is a function, false otherwise
  ///
  boolean isFunction() {
  JNIReturnClass runnable = new JNIReturnClass() {
  @Override
  public void run() {
  jni = new JNIReturnObject();
  jni.bool = isFunction(context.ctxRef(), valueRef);
  }
  };
  context.sync(runnable);
  return runnable.jni.bool;
  }

  ///
  /// Determines if the object is a constructor
  ///
  /// @return true if the object is a constructor, false otherwise
  ///
  boolean isConstructor() {
  JNIReturnClass runnable = new JNIReturnClass() {
  @Override
  public void run() {
  jni = new JNIReturnObject();
  jni.bool = isConstructor(context.ctxRef(), valueRef);
  }
  };
  context.sync(runnable);
  return runnable.jni.bool;
  }

  @Override
  int hashCode() {
  return valueRef().intValue();
  }

  final List<JSObject> zombies = new ArrayList<>();

  @Override
  void finalize() throws Throwable {
  super.finalize();
  context.finalizeObject(this);
  }

  void setThis(JSObject thiz) {
  this._thiz = thiz;
  }

  JSObject getThis() {
  return _thiz;
  }

  @SuppressWarnings("unused")
  JSValue __nullFunc() {
  return new JSValue(context);
  }

  protected JSFunction isInstanceOf = null;
  JSObject _thiz = null;

  /* Native Methods */

  int make(int ctx, int data) native 'JSObject_';

  @SuppressWarnings("unused")
  int makeInstance(int ctx) native 'JSObject_';

  CReturnValue makeArray(int ctx, List<int> args) native 'JSObject_';

  CReturnValue makeDate(int ctx,List<int> args) native 'JSObject_';

  CReturnValue makeError(int ctx,List<int> args) native 'JSObject_';

  CReturnValue makeRegExp(int ctx, List<int> args) native 'JSObject_';

  int getPrototype(int ctx, int object) native 'JSObject_';

  void setPrototype(int ctx, int object, int value) native 'JSObject_';

  boolean hasProperty(int ctx, int object, int propertyName) native 'JSObject_';

  CReturnValue getProperty(int ctx, int object, int propertyName) native 'JSObject_';

  CReturnValue setProperty(int ctx, int object, int propertyName, int value, int attributes) native 'JSObject_';

  CReturnValue deleteProperty(int ctx, int object, int propertyName) native 'JSObject_';

  CReturnValue getPropertyAtIndex(int ctx, int object, int propertyIndex) native 'JSObject_';

  CReturnValue setPropertyAtIndex(int ctx, int object, int propertyIndex, int value) native 'JSObject_';

  @SuppressWarnings("unused")
  int getPrivate(int object) native 'JSObject_';

  @SuppressWarnings("unused")
  boolean setPrivate(int object, int data) native 'JSObject_';

  boolean isFunction(int ctx, int object) native 'JSObject_';

  CReturnValue callAsFunction(int ctx, int object, int thisObject, long[] args native 'JSObject_';

  boolean isConstructor(int ctx, int object) native 'JSObject_';

  CReturnValue callAsConstructor(int ctx, int object, long[] args) native 'JSObject_';

  int copyPropertyNames(int ctx, int object) native 'JSObject_';

  List<int> getPropertyNames(int propertyNameArray) native 'JSObject_';

  void releasePropertyNames(int propertyNameArray) native 'JSObject_';

  int makeFunctionWithCallback(int ctx, int name) native 'JSObject_';

  CReturnValue makeFunction(int ctx, int name, List<int> parameterNames,
  int body, int sourceURL, int startingLineNumber) native 'JSObject_';

}