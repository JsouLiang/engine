part of dart.jsc;

class JSProperty<T> {
  Map<String, T> _propertyValueMap;

  T operator [](String propertyName) => _propertyValueMap[propertyName];

  operator []=(String propertyName, T value) =>
      _propertyValueMap[propertyName] = value;
}
