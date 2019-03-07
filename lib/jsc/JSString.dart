part of dart.jsc;

class JSString extends JSValue {
  
  JSString([JSContext context, String value]) : super(context) {

  }

  @override
  // TODO:
  String toString() {
    return "";
  }

  @override
  // TODO:
  bool toBoolean() {
  }

  @override
  bool isNumber() {
    return false;
  }

  @override
  bool isString() {
    return true;
  }

  @override
  bool isBoolean() {
    // TODO: implement isBoolean
    return null;
  }

  @override
  bool isNull() {
    // TODO: implement isNull
    return null;
  }

  @override
  bool isObject() {
    // TODO: implement isObject
    return null;
  }

  @override
  bool isUndefined() {
    // TODO: implement isUndefined
    return null;
  }

  @override
  double toNumber() {
    // TODO: implement toNumber
    return null;
  }

  /// Gets the JSString value of this JS value
  /// @return  The JSString value
  JSString toJSString() {
    print('toJSString Entry $valueRef');
    CReturnValue cReturnValue = _toStringCopy(context.ctxRef(), valueRef);
    if (cReturnValue.exception != 0) {
      context.throwJSException(JSException.fromJSValue(
          JSValue.fromValueRef(cReturnValue.exception, context)));
      return null;
    }
    print('toJSString'+ cReturnValue.reference.toString());
    print('toJSString JSString'+ JSString(cReturnValue.reference).toString());
    return JSString(cReturnValue.reference);
  }


  int makeString(JSContext ctx, String value) native 'JSValue_makeString';

  //native function
  int createWithCharacters(String str) native 'JSString_createWithCharacters';

  int retain(int strRef) native 'JSString_retain';

  void release(int stringRef) native 'JSString_release';

  bool _isEqual(int a, int b) native 'JSString_isEqual';

  String _toString(int strRef) native 'JSString_toString';

  int getLength(int stringRef) native 'JSString_getLength';

  int getMaximumUTF8CStringSize(int stringRef) native 'JSString_getMaximumUTF8CStringSize';

  int createWithUTF8CString(String str) native 'JSString_createWithUTF8CString';

  bool isEqualToUTF8CString(int a, String b) native 'JSString_isEqualToUTF8CString';

}
