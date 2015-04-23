# This File is still under construction #
# Guidline for Native Functions #

## add native functions ##

To add a native function the TinyJS-class provides a methode defined as
```
typedef void (*JSCallback)(const CFunctionsScopePtr &var, void *userdata);
CScriptVarFunctionNativePtr addNative(const std::string &funcDesc, JSCallback ptr, void *userdata=0);
```
Example:
```
void myCallback(const CFunctionsScopePtr &var, void *userdata) {
  // do some thing
}
CTinyJS tinyJS();
tinyJS.addNative("function nativeFnc(arguments)", myCallback, 0);
```

With 42TinyJs is it possible to add a c++ class-member-methode as native function. The TinyJS-class provides a methode-template defined as:
```
template<class C>
CScriptVarFunctionNativePtr addNative(const std::string &funcDesc, C *class_ptr, void(C::*class_fnc)(const CFunctionsScopePtr &, void *), void *userdata=0)
```
Example:
```
class MyClass {
public:
   void myCallback(const CFunctionsScopePtr &var, void *userdata);
};
CTinyJS tinyJS();
MyClass callbackClass();
tinyJS.addNative("function nativeClassFnc(arguments)", &callbackClass, &MyClass::myCallback, 0);
```

funcDesc

```











```