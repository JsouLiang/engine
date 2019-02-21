part of dart.jsc;

///  
///  A JSException is thrown for a number of different reasons, mostly by the JavaScriptCore
/// library.  The description of the exception is given in the message. 
/// 
class JSException extends Exception {
  static final int _serialVersionUID = 1;

   JSError _error;

    ///
    /// Creates a Java exception from a thrown JavaScript exception
    ///@param error  The JSValue thrown by the JavaScriptCore engine
    ///@since 1.0
    ///
     JSException(JSValue error) {
        super(new JSError(error).message());
        this.error = new JSError(error);
    }

    /**
     * Creates a JavaScriptCore exception from a string message
     * @param ctx  The JSContext in which to create the exception
     * @param message  The exception meessage
     * @since 1.0
     */
    public JSException(JSContext ctx, String message) {
        super(message);
        try {
            this.error = new JSError(ctx,message);
        } catch (JSException e) {
            // We are having an Exception Inception. Stop the madness
            this.error = null;
        }
    }
    /**
     * Gets the JSValue of the thrown exception
     * @return  the JSValue of the JavaScriptCore exception
     * @since 1.0
     */
    public JSError getError() {
        return error;
    }

    /**
     * JavaScript error stack trace, see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Error/Stack
     * @return stack trace for error
     * @since 3.0
     */
    public String stack() {
        return (error!=null) ? error.stack() : "undefined";
    }

    /**
     * JavaScript error name, see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Error/name
     * @return error name
     * @since 3.0
     */
    public String name() {
        return (error!=null) ? error.name() : "JSError";
    }

    @Override
    public String toString() {
        if (error!=null) {
            try {
                return error.toString();
            } catch (JSException e) {
                return "Unknown Error";
            }
        }
        return "Unknown Error";
    }
}