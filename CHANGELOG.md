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
