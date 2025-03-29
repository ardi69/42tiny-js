Version 0.9.0 (2016-04-11 16:52:59)
===================================

* added versioning (started at version 0.9.0)
* added changelog.md
* changed Makefile for stressless builds
* Script.cpp remove generator test at startup
* some speed-optimation
* Array stuff changed (length is no more a getter)
* many other changes

-------------------------------------------------------------------------------


Version 0.9.1 (2016-04-11 18:00:23)
===================================

* cleanup & fix pool_allocator

-------------------------------------------------------------------------------


Version 0.9.2 (2016-04-13 18:51:24)
===================================

* fix ScriptVar from int64_t
* added: README.md

-------------------------------------------------------------------------------


Version 0.9.3 (2016-04-14 20:10:22)
===================================

* added MSVC 2013 project-files
* added some virtual functions

-------------------------------------------------------------------------------


Version 0.9.4 (2024-04-08 16:53:41)
===================================

* little change on pool_allocator
* wait on keypress on end of Script.exe
* added MSVC 2017 project-files
* added MSVC 2022 project-files
* fix compiler error end warnings with MSVC 2017/2022
* change some functions to const
* change C++ Language with MSVC to ISO Standard C++20

-------------------------------------------------------------------------------


Version 0.9.5 (2024-04-09 21:45:44)
===================================

* fix typos in MSVC sln
* fix for test029.js, test31.js and test032.42.js
* remove c++ array.length

-------------------------------------------------------------------------------


Version 0.9.6 (2024-04-13 01:30:28)
===================================

* added few functions comments
* added exponentiation operator (**)
* added exponentiation assignment (**=)
* added nullish coalescing operator (??)
* added nullish coalescing assignment (??=)
* added optional chaining (?.) e.g. obj.val?.prop or obj.val?.[expr] or obj.func?.(args)

-------------------------------------------------------------------------------


Version 0.9.8 (2024-04-13 12:07:35)
===================================

* fixed test024.js

-------------------------------------------------------------------------------


Version 0.9.9 (2024-04-13 13:56:41)
===================================

* removed cloning stuff
* changed test019.42.js now uses JSON.parse(JSON.stringify(obj1)) instead of obj1.clone()
* added generator function statements and expessions with asterisk
  - as statement: function* generator() { ... }
  - as expression let generator = function*() { ... }
  - or as object member: let obj = { *generator() { ... } }
  Previously, a generator was recognized by it if a yield expression was included. This behavior is no longer supported.
* changed 42tests/test004.js now uses function* fibonacci()

-------------------------------------------------------------------------------


Version 0.10.0 (2025-03-26 10:39)
=================================

* modern times: from version 0.10.0 a compiler with C++17 support is required
* changed: parseFloat, parseInt CNumber.operator=(string_view)
* changed: Left2Right is now a sorted array at compile time (constexpr)
* removed: serialize and unserialize binary (compiled js-code)
* removed: dynamic_cast<CScriptTokenData...> now std::variant is used no RTTI overhead
* added: all CScriptTokenData... now hold in std::shared_ptr
* removed global using namespace std; in all cpp-files
* replaced dynamic_cast with CScriptVarDynamicCast (useing of custom RTTI) ca. 7x faster then dynamic_cast
* removed unneeded deleted copy-constructors
* changed CScriptVarPtr is replaced by std::shared_ptr<CScriptVar>
* changed enum NType to enum class NType
* changed reserved_words_begin is now a sorted array reserved_words at compile time (constexpr)
* changed:str2reserved_begin is now a sorted array reserved_words_by_str at compile time (constexpr)
* changed:tokens2str_begin is now a sorted array tokens2str at compile time (constexpr)
* changed enum TOKENIZE_FLAGS to enum _class_ TOKENIZE_FLAGS
* changed CScriptVarLink now allways hold in shared_ptr
* removed some debuging stuff
* added TinyJS now uses always the namespace TinyJS
* added the lexer now uses a ring buffer system and can read directly from a stream
* changed Now std::function is used instead of a simple function pointer for native functions
* removed No longer required classes CScriptVarFunctionNativeCallback and CScriptVarFunctionNativeClass. All native functionality is now in class CScriptVarFunctionNative
* added CScriptVarString(std::string &&) move constructor
* added String.prototype.padStart(targetLength, padString)
* added String.prototype.padEnd(targetLength, padString)
* added String.prototype.repeat(count)
* added String.prototype.includes(searchString, position)
* added String.prototype.startsWith(searchString, position)
* added String.prototype.endsWith(searchString, length)
* removed octal escapes in string literals
* added template literals
* added ...Rest Parameter
* changed let and const Statements rewriten
* removed non Standard let expression
* removed non Standard conditional catch
* removed non Standard Array Comprehensions
* changed destructuring rewritten
* added Symbol() Symbol.for() and Symbol.keyFor()
* changed rename Array.prototype.contains to Array.prototype.includes
* added Array.prototype.map
* added Array.prototype.shift
* added Array.prototype.unshift
* added Array.prototype.slice
* added Array.prototype.splice
* added Array.prototype.forEach
* added Array.prototype.indexOf
* added Array.prototype.reduce
* added Array.prototype.reduceRight
* removed unneeded files

-------------------------------------------------------------------------------


Version 0.10.1 (2025-03-28 09:00)
=================================

* updated README.md
* changed MSVC 2017 c++ standard to c++17
* changed MSVC 2020 c++ standard to c++20
* fixed   compilation issues with MSVC 2017 and 2022
* removed debugging code from the pool allocato
* updated improved readability of run_tests.cpp and Script.cpp
* updated time_logger.h to use std::chrono::high_resolution_clock
* added   console.log, console.debug, console.info, console.warn and console.error
* added   CTinyJS::setConsole to set log level and output stream
* added   console.time, console.timeLog, console.timeEnd

-------------------------------------------------------------------------------


Version 0.10.2 (2025-03-29 20:18)
=================================

* added overlodaed versions of addNative to register standard functions like int(int,int)
* renamed pool_allocator.h/.cpp to TinyJS_PoolAllocator.h/.cpp
renamed pool_allocator.h/.cpp to TinyJS_PoolAllocator.h/.cpp
