part of dart.jsc;

///
///  A JSException is thrown for a number of different reasons, mostly by the JavaScriptCore
/// library.  The description of the exception is given in the message.
///
class JSException implements Exception {
  static final int _serialVersionUID = 1;

  JSError error;

  /// Creates a JavaScriptCore exception from a string message
  /// @param ctx  The JSContext in which to create the exception
  /// @param message  The exception meessage
  JSException(JSContext ctx, String message) {
    try {
      error = new JSError(ctx, message);
    } on JSException catch (e) {
      // We are having an Exception Inception. Stop the madness
      error = null;
    }
  }

  ///
  /// Creates a Java exception from a thrown JavaScript exception
  ///@param error  The JSValue thrown by the JavaScriptCore engine
  ///
  JSException.fromJSValue(JSValue error) {
    error = new JSError(error);
  }

  /// Gets the JSValue of the thrown exception
  /// @return  the JSValue of the JavaScriptCore exception
  JSError getError() {
    return error;
  }

  /// JavaScript error stack trace, see:
  /// https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Error/Stack
  /// @return stack trace for error
  /// @since 3.0
  String stack() {
    return (error != null) ? error.stack() : "undefined";
  }

  /// JavaScript error name, see:
  /// https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Error/name
  /// @return error name
  /// @since 3.0
  String name() {
    return (error != null) ? error.name() : "JSError";
  }

  @override
  String toString() {
    if (error != null) {
      try {
        return error.toString();
      } on JSException catch (e) {
        return "JSException: Unknown Error1";
      }
    }
    return "JSException: Unknown Error2";
  }
}
