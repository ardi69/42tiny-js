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

#include "pool_allocator.h"
#include <set>
#include <vector>
#include <algorithm>
#include <stdio.h>

#include <assert.h>
#ifndef ASSERT
#	define ASSERT(X) assert(X)
#endif

#include "time_logger.h"

TimeLoggerCreate(alloc, false);
TimeLoggerCreate(free, false);

namespace {

	struct block {
		block* next;
	};
	struct block_head {
		block_head* next;
	};


	class _fixed_size_allocator {
	public:
		_fixed_size_allocator( size_t numObjects, size_t objectSize ) :
			num_objects(numObjects), object_size(objectSize >= sizeof(block) ? objectSize : sizeof(block)), block_size(sizeof(block_head) + object_size*num_objects),
			head(0), head_of_free_list(0), refs(0)
	#ifdef DEBUG_POOL_ALLOCATOR
			, allocs(0), frees(0), current(0), max(0), blocks(0)
	#endif
		{
		}

		~_fixed_size_allocator()
		{
			while(head) {
				char *p = (char*)head;
				head = head->next;
				delete [] p;
			}
	#ifdef DEBUG_POOL_ALLOCATOR
	#	ifndef LOG_POOL_ALLOCATOR_MEMORY_USAGE
			if(refs) {
	#	endif
				fprintf(stderr, "allocator %d byte (", object_size);
				const char *sep = "";
				for(std::set<const char*>::iterator it = names.begin(); it!=names.end(); ++it, sep=", ")
					fprintf(stderr, "%s%s", sep, *it);
				fprintf(stderr, ") destroyed\n");
				fprintf(stderr, "allocs:%i, ", allocs);
				fprintf(stderr, "frees:%i, ", frees);
				fprintf(stderr, "max:%i, ", max);
				fprintf(stderr, "blocks:%i\n", blocks);
				if(refs) fprintf(stderr, "************ %i x not freed ************\n", refs);
				fprintf(stderr, "\n");
	#	ifndef LOG_POOL_ALLOCATOR_MEMORY_USAGE
			}
	#	endif
	#endif
		}

	#ifdef DEBUG_POOL_ALLOCATOR
		void* _alloc( size_t size, const char *for_class ) {
			ASSERT(size <= object_size);
			std::set<const char*>::iterator it = names.find(for_class);
			if(it == names.end()) names.insert(for_class);
	#else
		void* _alloc() {
	#endif
			++refs;
	#ifdef DEBUG_POOL_ALLOCATOR
			++allocs; ++current;
			if(current > max) max = current;
	#endif
			block* p = head_of_free_list;
			if(p == 0) {
				char* new_block = new char[block_size];
				((block_head*)new_block)->next = head;
				head = (block_head*)new_block;

				p = (block*) (new_block + sizeof(block_head));
				block *end = (block*) (new_block + block_size);
				block *last = p;
				for(block *it = (block*)(((char*)p)+object_size); it < end; last=it, it = (block*)(((char*)it)+object_size))
					last->next = it;
				last->next = 0;
	#ifdef DEBUG_POOL_ALLOCATOR
				++blocks;
	#endif
			}
			head_of_free_list = p->next;
			return p;
		}


	#ifdef DEBUG_POOL_ALLOCATOR
		bool _free( void* p, size_t size) {
			ASSERT(size <= object_size);
	#else
		bool _free( void* p) {
	#endif
			if(p == 0) return refs==0;
			--refs;
	#ifdef DEBUG_POOL_ALLOCATOR
			ASSERT(refs>=0);
			++frees; --current;
	#endif
			block* dead_object = (block*) p;
			dead_object->next = head_of_free_list;
			head_of_free_list = dead_object;
			return refs==0;
		}

		size_t num_objects;
		size_t object_size;
		size_t block_size;
		block_head *head;
		block *head_of_free_list;
		int refs;
	#ifdef DEBUG_POOL_ALLOCATOR
		// Debug
		std::set<const char*> names;
		int allocs;
		int frees;
		int current;
		int max;
		int blocks;
	#endif
	};


	#ifdef NO_THREADING
	#	define LOCK do{}while(0)
	#elif SPINLOCK_IN_POOL_ALLOCATOR
	static CScriptSpinLock locker;
	#	define LOCK CScriptUniqueLock<CScriptSpinLock> lock(locker)
	#else
	static CScriptMutex locker;
	#	define LOCK CScriptUniqueLock<CScriptMutex> lock(locker)
	#endif

#	define ROUND_SIZE(size) ((size+3) & ~3)
#	define SHIFT_SIZE(size) (size >> 2)
	static struct _fixed_size_allocator_init_control {
		~_fixed_size_allocator_init_control() {
			for (std::vector<_fixed_size_allocator*>::iterator it = pool.begin(); it != pool.end(); ++it)
				if (*it) delete *it;
		}
		_fixed_size_allocator *& getFromPool(size_t size) {
			size = SHIFT_SIZE(size);
			if (size >= pool.size())
				pool.resize(size + 1, 0);
			return pool[size];
		}
		std::vector<_fixed_size_allocator*> pool;

	} control;
} /* namespace */

#ifdef DEBUG_POOL_ALLOCATOR
void * fixed_size_allocator::alloc(size_t size ,const char* for_class) {
#else
void * fixed_size_allocator::alloc(size_t size) {
#endif
	TimeLoggerHelper(alloc);
	LOCK;
	size = ROUND_SIZE(size);
	_fixed_size_allocator *&allocator = control.getFromPool(size);
	if(!allocator) {
		allocator = new _fixed_size_allocator(64, size);
	}
#ifdef DEBUG_POOL_ALLOCATOR
	return allocator->_alloc(size, for_class);
#else
	return allocator->_alloc();
#endif
}

void fixed_size_allocator::free(void *p, size_t size) {
	TimeLoggerHelper(free);
	LOCK;
	size = ROUND_SIZE(size);
	_fixed_size_allocator *&allocator = control.getFromPool(size);
	if(allocator) {
#ifdef DEBUG_POOL_ALLOCATOR
		if(allocator->_free(p, size)) {
#else
		if(allocator->_free(p)) {
#endif
			delete allocator;
			allocator = 0;
		}
	} else {
		ASSERT(0/* free called but not allocator defined*/);
	}
}

