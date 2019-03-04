#include "flutter/lib/jsc/shell/jsc_utils.h"
#include "third_party/dart/runtime/include/dart_api.h"
#include "third_party/tonic/converter/dart_converter.h"
#include "flutter/fml/logging.h"

using tonic::ToDart;

namespace jsc{

Dart_Handle newCReturnValue() {
  Dart_Handle jsc_lib = Dart_LookupLibrary(ToDart("dart:jsc"));
  DART_CHECK_VALID(jsc_lib);
  // Dart_Handle retobj = Dart_Invoke(jsc_lib, ToDart("getCReturnValue"), 0, NULL);
  // return retobj;
  Dart_Handle type = Dart_GetType(jsc_lib, ToDart("CReturnValue"), 0, NULL);
  DART_CHECK_VALID(type);
  Dart_Handle result = Dart_New(type, Dart_Null(), 0, NULL);
  return result;
}

Dart_Handle newCReturnValue(bool boolean, JSValueRef exception) {

  Dart_Handle retobj = newCReturnValue();

  Dart_Handle result = Dart_SetField(retobj, ToDart("boolean"), Dart_NewBoolean(boolean));
  result = Dart_SetField(retobj, ToDart("exception"),
                         Dart_NewInteger((long)exception));

  return retobj;
}

Dart_Handle newCReturnValue(double dvalue, JSValueRef exception) {
  Dart_Handle retobj = newCReturnValue();
  Dart_Handle result = Dart_SetField(retobj, ToDart("number"), Dart_NewBoolean(dvalue));
  result = Dart_SetField(retobj, ToDart("exception"),
                         Dart_NewInteger((long)exception));
  return result;
}

Dart_Handle newCReturnValue(JSValueRef reference, JSValueRef exception) {
  Dart_Handle retobj = newCReturnValue();
  Dart_Handle result = Dart_SetField(retobj, ToDart("reference"),
                         Dart_NewInteger((long)reference));
  result = Dart_SetField(retobj, ToDart("exception"),
                         Dart_NewInteger((long)exception));
  return retobj;
}

Dart_Handle newCReturnValue(double dvalue,
                            bool boolean,
                            JSValueRef reference,
                            JSValueRef exception) {
  Dart_Handle retobj = newCReturnValue();
  Dart_Handle result = Dart_SetField(retobj, ToDart("boolean"), Dart_NewBoolean(boolean));
  result = Dart_SetField(retobj, ToDart("number"), Dart_NewDouble(dvalue));
  result = Dart_SetField(retobj, ToDart("reference"),
                         Dart_NewInteger((long)reference));
  result = Dart_SetField(retobj, ToDart("exception"),
                         Dart_NewInteger((long)exception));
  return retobj;
}
}

// #endif //JSC_JSC_UTILS_CC
