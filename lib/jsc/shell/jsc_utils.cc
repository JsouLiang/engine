#include "flutter/lib/jsc/shell/jsc_utils.h"
#include "third_party/dart/runtime/include/dart_api.h"
#include "third_party/tonic/converter/dart_converter.h"
#include "flutter/fml/logging.h"

using tonic::ToDart;

namespace jsc{

Dart_Handle newCReturnValue() {
  Dart_Handle jsc_lib = Dart_LookupLibrary(ToDart("dart:jsc"));
  DART_CHECK_VALID(jsc_lib);
  Dart_Handle type = Dart_GetType(jsc_lib, ToDart("CReturnValue"), 0, NULL);
  DART_CHECK_VALID(type);
  Dart_Handle result = Dart_New(type, Dart_Null(), 0, NULL);
  return result;
}

Dart_Handle newCReturnValue(bool boolean, JSValueRef exception) {

  Dart_Handle result = newCReturnValue();

  result = Dart_SetField(result, ToDart("boolean"), Dart_NewBoolean(boolean));
  result = Dart_SetField(result, ToDart("exception"),
                         Dart_NewInteger((long)exception));

  return result;
}

Dart_Handle newCReturnValue(double dvalue, JSValueRef exception) {
  Dart_Handle result = newCReturnValue();
  result = Dart_SetField(result, ToDart("number"), Dart_NewBoolean(dvalue));
  result = Dart_SetField(result, ToDart("exception"),
                         Dart_NewInteger((long)exception));
  return result;
}

Dart_Handle newCReturnValue(JSValueRef reference, JSValueRef exception) {
  Dart_Handle result = newCReturnValue();
  FML_LOG(ERROR)<< "JSContext::newCReturnValue0"<<result;

//todo set 完变成了null

  result = Dart_SetField(result, ToDart("reference"),
                         Dart_NewInteger((long)reference));
  FML_LOG(ERROR)<< "JSContext::newCReturnValue1"<<result<< (Dart_IsError(result))<<Dart_NewInteger((long)exception);

  Dart_Handle result1 = Dart_GetField(result, ToDart("reference"));
  int64_t buffer_size = 0;
  Dart_IntegerToInt64(result1, &buffer_size);
  FML_LOG(ERROR)<< "JSContext::Dart_GetField"<<result1<<buffer_size;
                       

  result = Dart_SetField(result, ToDart("exception"),
                         Dart_NewInteger((long)exception));
  FML_LOG(ERROR)<< "JSContext::newCReturnValue2"<<result<< (Dart_IsError(result))<<Dart_NewInteger((long)exception);

  return result;
}

Dart_Handle newCReturnValue(double dvalue,
                            bool boolean,
                            JSValueRef reference,
                            JSValueRef exception) {
  Dart_Handle result = newCReturnValue();
  result = Dart_SetField(result, ToDart("boolean"), Dart_NewBoolean(boolean));
  result = Dart_SetField(result, ToDart("number"), Dart_NewDouble(dvalue));
  result = Dart_SetField(result, ToDart("reference"),
                         Dart_NewInteger((long)reference));
  result = Dart_SetField(result, ToDart("exception"),
                         Dart_NewInteger((long)exception));
  return result;
}
}

// #endif //JSC_JSC_UTILS_CC
