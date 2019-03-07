part of dart.jsc;

class JSNumber extends JSValue {

  bool _isNumber = true;

  JSNumber(double number, [JSContext context]) {
//    makeNumber(context, number);
  }

  @override
  //TODO:
  bool isUndefined() {
  }

  @override
  //TODO:
  bool isNull() {
  }

  @override
  bool isBoolean() {
    return !_isNumber;
  }

  @override
  bool isNumber() {
    return _isNumber;
  }

  @override
  bool isString() {
    return false;
  }

  /// Native Function
  ///
  int makeBoolean(int ctx, bool bool) native 'JSValue_makeBoolean';

  int makeNumber(int ctx, double number) native 'JSValue_makeNumber';
}
