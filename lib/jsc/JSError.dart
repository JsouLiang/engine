part of dart.jsc;

///
/// A convenience class for managing JavaScript error objects
///
class JSError extends JSObject {

    ///
    /// Generates a JavaScript throwable exception object
    /// @param ctx  The context in which to create the error
    /// @param message  The description of the error
    ///
    JSError(JSContext ctx, {String message}) {
        context = ctx;
        List<int> args;
        if(message != null){
            args = [JSValue(context,message).valueRef()];
        } else{
            args = List<int>(0);
        }
        CReturnValue jni = makeError(context.ctxRef(), args);
        if (BuildConfig.DEBUG && jni.exception != 0) throw new AssertionError();
        valueRef = jni.reference;
    }

    ///
    /// Constructs a JSError from a JSValue.  Assumes JSValue is a properly constructed JS Error
    /// object.
    /// @param error the JavaScript Error object
    ///
    JSError.fromJSValue(JSValue error){
        super(error.valueRef(), error.getContext())

    }


    /// JavaScript error stack trace, see:
    /// https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Error/Stack
    /// @return stack trace for error
    /// @since 3.0
    String stack() {
        return property("stack").toString();
    }

    /// JavaScript error message, see:
    /// https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Error/message
    /// @return error message
    /// @since 3.0
    String message() {
        return property("message").toString();
    }

    ///
    /// JavaScript error name, see:
    /// https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Error/name
    /// @return error name
    /// @since 3.0
    ///
    String name() {
        return property("name").toString();
    }

}
