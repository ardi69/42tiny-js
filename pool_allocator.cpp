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
#include <iostream>

#include <cassert>
#ifndef ASSERT
#	define ASSERT(X) assert(X)
#endif

namespace TinyJS {

namespace {

struct block {
	block *next;
};
struct block_head {
	block_head *next;
};


class _fixed_size_allocator {
public:
	_fixed_size_allocator(size_t numObjects, size_t objectSize) :
		num_objects(numObjects), object_size(objectSize >= sizeof(block) ? objectSize : sizeof(block)), block_size(sizeof(block_head) + object_size * num_objects),
		head(0), head_of_free_list(0), refs(0) {}

	~_fixed_size_allocator() {
		while (head) {
			char *p = (char *)head;
			head = head->next;
			delete[] p;
		}
	}

	void *_alloc() {
		++refs;
		block *p = head_of_free_list;
		if (p == 0) {
			char *new_block = new char[block_size];
			((block_head *)new_block)->next = head;
			head = (block_head *)new_block;

			p = (block *)(new_block + sizeof(block_head));
			block *end = (block *)(new_block + block_size);
			block *last = p;
			for (block *it = (block *)(((char *)p) + object_size); it < end; last = it, it = (block *)(((char *)it) + object_size))
				last->next = it;
			last->next = 0;
		}
		head_of_free_list = p->next;
		return p;
	}

	bool _free(void *p) {
		if (p == 0) return refs == 0;
		--refs;
		block *dead_object = (block *)p;
		dead_object->next = head_of_free_list;
		head_of_free_list = dead_object;
		return refs == 0;
	}

	size_t num_objects;
	size_t object_size;
	size_t block_size;
	block_head *head;
	block *head_of_free_list;
	int refs;
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
		for (std::vector<_fixed_size_allocator *>::iterator it = pool.begin(); it != pool.end(); ++it)
			if (*it) delete *it;
	}
	_fixed_size_allocator *&getFromPool(size_t size) {
		size = SHIFT_SIZE(size);
		if (size >= pool.size())
			pool.resize(size + 1, 0);
		return pool[size];
	}
	std::vector<_fixed_size_allocator *> pool;

} control;
} /* namespace */

void *fixed_size_allocator::alloc(size_t size) {
	LOCK;
	size = ROUND_SIZE(size);
	_fixed_size_allocator *&allocator = control.getFromPool(size);
	if (!allocator) {
		allocator = new _fixed_size_allocator(64, size);
	}
	return allocator->_alloc();
}

void fixed_size_allocator::free(void *p, size_t size) {
	LOCK;
	size = ROUND_SIZE(size);
	_fixed_size_allocator *&allocator = control.getFromPool(size);
	if (allocator) {
		if (allocator->_free(p)) {
			delete allocator;
			allocator = 0;
		}
	} else {
		ASSERT(0/* free called but not allocator defined*/);
	}
}

}