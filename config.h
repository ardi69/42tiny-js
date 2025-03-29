#ifndef _42TinyJS_config_h__
#define _42TinyJS_config_h__

/*
 * 42TinyJS
 *
 * A fork of TinyJS with the goal to makes a more JavaScript/ECMA compliant engine
 *
 * Authored By Armin Diedering <armin@diedering.de>
 *
 * Copyright (C) 2010-2025 ardisoft
 *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

//////////////////////////////////////////////////////////////////////////

/* POOL-ALLOCATOR
 * ==============
 * To speed-up new & delete 42TinyJS adds an object-pool
 * The pool is activated by default.
 * To deactivate this stuff define NO_POOL_ALLOCATOR
 */
//#define NO_POOL_ALLOCATOR

/*
 * to increasing of speed a spin lock is used, if available
 * if c++11 <atomic> not available or NO_SPINLOCK_IN_POOL_ALLOCATOR is defined
 * a normal mutex lock is used or noting if NO_THREADING defined
 */
//#define NO_SPINLOCK_IN_POOL_ALLOCATOR

//////////////////////////////////////////////////////////////////////////

/* LET-STUFF
 * =========
 * Redeclaration of LET-vars is not allowed in block-scopes.
 * But in the root- and functions-scopes it is currently allowed
 * In future ECMAScript versions this will be also in root-and functions-scopes forbidden
 * To enable the future behavior define PREVENT_REDECLARATION_IN_FUNCTION_SCOPES
 */
//#define PREVENT_REDECLARATION_IN_FUNCTION_SCOPES

//////////////////////////////////////////////////////////////////////////

/* GENERATOR's
 * ===========
 * functions with "yield" in it is detected as Generator.
 * Generator-support needs threading-stuff
 * To disable Generators define NO_GENERATORS
 * NOTE NO_THREADING implies NO_GENERATORS
 */
//#define NO_GENERATORS


//////////////////////////////////////////////////////////////////////////

/* MULTI-THREADING
 * ===============
 * 42TinyJS is basically thread-save.
 * You can run different or the same JS-code simultaneously in different instances of class TinyJS.
 * >>> NOTE: You can NOT run more threads on the SAME instance of class TinyJS <<<
 * The threading-stuff is needed by the pool-allocator (locking) and the generator-/yield-stuff
 * to deactivate threading define NO_THREADING
 * NOTE: if NO_THREADING defined and NO_POOL_ALLOCATOR is undefined you can not run JS-code simultaneously
 */
//#define NO_THREADING

/* with C++2011 (or MS VisualC++ 2012 and above) the C++ 2011 STL-Threading-API is used.
 * You can define NO_CXX_THREADS to use alternate API's
 */
//#define NO_CXX_THREADS

/* if C++ 2011 STL-Threading-API not available
 * - on Windows the windows-threading-API is used by default.
 * - on non-Windows (WIN32 is not defined) it is tried to use the POSIX pthread-API
 * to force the pthread-API define HAVE_PTHREAD (windows needs in this case
 *   a pthread-lib e.g http://http://sourceware.org/pthreads-win32/)
 */
//#define HAVE_PTHREAD

/* you can implement your own custom thread-implementation.
 * to prevent the using of the win- or pthread-API define HAVE_CUSTOM_THREADING_IMPL
 */
//#define HAVE_CUSTOM_THREADING_IMPL



/* for Date we need the time in a resolution of 1 ms
 * on Windows the function "GetSystemTimeAsFileTime" is used
 * on non-Windows (WIN32 is not defined) it is tried to use "gettimeofday"
 * unset HAVE_GETTIMEOFDAY to use a simple time()*1000
 * or define NO_GETTIMEOFDAY
 */
#define HAVE_GETTIMEOFDAY
#ifdef NO_GETTIMEOFDAY
#undef HAVE_GETTIMEOFDAY
#endif


////////////////////////////////////////////////
// DO NOT MAKE CHANGES OF THE FOLLOWING STUFF //
////////////////////////////////////////////////

#if defined(NO_THREADING) && !defined(NO_GENERATORS)
#	define NO_GENERATORS
#pragma message("\n***********************************************************************\n\
* You have defined NO_THREADING and not defined NO_GENERATORS\n\
* NOTE: GENERATORS needs THREADING. Generators/Yield are deactivated\n\
***********************************************************************\n")
#endif

#if defined(NO_POOL_ALLOCATOR) && defined(NO_GENERATORS) && !defined(NO_THREADING)
#	define NO_THREADING
#endif

#if !defined(NO_POOL_ALLOCATOR) && defined(NO_THREADING)
#pragma message("\n***********************************************************************\n\
* You have defined NO_THREADING and not defined NO_POOL_ALLOCATOR\n\
* NOTE: you can not run JS-code simultaneously in different threads\n\
***********************************************************************\n")
#endif

#define isCXX0x(version, minor) (defined(__GXX_EXPERIMENTAL_CXX0X__) && (__GNUC__ > version || __GNUC__ == version && __GNUC_MINOR__ >= minor))

#if __cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__) || _MSC_VER >= 1700 // Visual Studio 2012 and above
#		if !defined(NO_CXX_THREADS) && !defined(NO_THREADING)
#			define HAVE_CXX_THREADS 1
#		endif
#endif

#if !defined(NO_SPINLOCK_IN_POOL_ALLOCATOR) && (__cplusplus >= 201103L || isCXX0x(4,5) || _MSC_VER >= 1700 /* Visual Studio 2012 */)
#	define SPINLOCK_IN_POOL_ALLOCATOR 1
#endif

//#undef HAVE_CXX_THREADS

#endif // _42TinyJS_config_h__
