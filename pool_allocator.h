/*
 * 42TinyJS
 *
 * A fork of TinyJS with the goal to makes a more JavaScript/ECMA compliant engine
 *
 * Authored By Armin Diedering <armin@diedering.de>
 *
 * Copyright (C) 2010-2015 ardisoft
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


#ifndef pool_allocator_h__
#define pool_allocator_h__

#include <typeinfo>
#include <stdint.h>
#include <string>
#include "config.h"
#ifndef NO_THREADING
#	if SPINLOCK_IN_POOL_ALLOCATOR
#		include "TinyJS_Threading.h"
#	endif
#endif
/************************************************************************
 * TinyJS must many many times allocates and frees objects
 * To prevent memory fragmentation and speed ups allocates & frees, i have
 * added to 42TinyJS a pool_allocator. This allocator allocates every 64 objects
 * as a pool of objects. Is an object needed it can faster allocated from this pool as 
 * from the heap.
 ************************************************************************/ 

//#define DEBUG_POOL_ALLOCATOR
//#define LOG_POOL_ALLOCATOR_MEMORY_USAGE
#if !defined(DEBUG_POOL_ALLOCATOR) && (defined(_DEBUG) || defined(LOG_POOL_ALLOCATOR_MEMORY_USAGE))
#	define DEBUG_POOL_ALLOCATOR
#endif

struct block_head;
class __fixed_size_allocator {
public:
	~__fixed_size_allocator();
	static void *alloc(size_t,const char* for_class=0);
	static void free(void *, size_t);
	size_t objectSize() { return object_size; }
private:
	__fixed_size_allocator(size_t num_objects, size_t object_size, const char* for_class); 
	__fixed_size_allocator(const __fixed_size_allocator&);
	__fixed_size_allocator& operator=(const __fixed_size_allocator&);
	void *_alloc(size_t);
	bool _free(void* p, size_t);
	size_t num_objects;
	size_t object_size;
	void *head_of_free_list;
	block_head *head;
	int refs;
#ifdef DEBUG_POOL_ALLOCATOR
	// Debug
	std::string name;
	int allocs;
	int frees;
	int current;
	int max;
	int blocks;
#endif
};
//**************************************************************************************
namespace fixed_size_allocator {
#ifdef DEBUG_POOL_ALLOCATOR
	void *alloc(size_t size, const char* for_class);
#else
	void *alloc(size_t size);
#endif
	void free(void *p, size_t size);

}
template<typename T, int num_objects=64>
class fixed_size_object {
public:
	static void* operator new(size_t size) {
#ifdef DEBUG_POOL_ALLOCATOR
		return fixed_size_allocator::alloc(size, typeid(T).name());
#else
		return fixed_size_allocator::alloc(size);
#endif
	}
	static void* operator new(size_t size, void* p) {
		return p;
	}
	static void operator delete(void* p, size_t size) {
		fixed_size_allocator::free(p, size);
	}
private:
};

#endif // pool_allocator_h__
