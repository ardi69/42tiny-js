/*
 * 42TinyJS
 *
 * A fork of TinyJS with the goal to makes a more JavaScript/ECMA compliant engine
 *
 * Authored By Armin Diedering <armin@diedering.de>
 *
 * Copyright (C) 2010 ardisoft
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
#include <vector>
#include <algorithm>

struct block {
	block* next;
};
struct block_head {
	block_head* next;
};

static void set_next(void* p, void* next) {
	static_cast<block*>(p)->next = static_cast<block*>(next);
}

static void* get_next(void* p) {
	return static_cast<block*>(p)->next;
}

fixed_size_allocator::fixed_size_allocator( size_t numObjects, size_t objectSize, const char *for_class )
{
	num_objects = numObjects;
	object_size = objectSize >= sizeof(block) ? objectSize : sizeof(block);

	head_of_free_list = head = 0;

#ifdef DEBUG_POOL_ALLOCATOR
	if(for_class) name = for_class;
	allocs=
	frees=
	max =
	current=
	blocks = 0;
#endif
}

fixed_size_allocator::~fixed_size_allocator()
{
	while(head) {
		char *p = (char*)head;
		head = head->next;
		delete [] p;
	}
#ifdef DEBUG_POOL_ALLOCATOR
#	ifndef LOG_POOL_ALLOCATOR_MEMORY_USAGE
	if(allocs != frees) {
#	endif
	fprintf(stderr, "allocator [%s](%d) destroyed\n", name.c_str(), object_size);
	fprintf(stderr, "  allocs:%i, ", allocs);
	fprintf(stderr, "frees:%i, ", frees);
	fprintf(stderr, "max:%i, ", max);
	fprintf(stderr, "blocks:%i\n", blocks);
	if(allocs != frees) fprintf(stderr, "************ %i x not freed ************\n", allocs-frees);
	fprintf(stderr, "\n");
#	ifndef LOG_POOL_ALLOCATOR_MEMORY_USAGE
	}
#	endif
#endif
}

void* fixed_size_allocator::alloc( size_t ) {
#ifdef DEBUG_POOL_ALLOCATOR
	allocs++;current++;
	if(current>max)max=current;
#endif
	void* p = head_of_free_list;
	if(p)  {
		head_of_free_list = get_next(p);
	} else {

		char* new_block = new char[sizeof(block_head) + num_objects * object_size];
		((block_head*)new_block)->next = head;
		head = (block_head*)new_block;
		new_block += sizeof(block_head);
		for(std::size_t i = object_size; i < (num_objects - 1) * object_size; i += object_size) {
			set_next(&new_block[i], &new_block[i + object_size]);
		}
		set_next(&new_block[(num_objects - 1) * object_size], 0);
		p = new_block;
		head_of_free_list = &new_block[object_size];

#ifdef DEBUG_POOL_ALLOCATOR
		blocks++;
#endif
	}
	return p;
}
#include <assert.h>
#ifndef ASSERT
#	define ASSERT(X) assert(X)
#endif

void fixed_size_allocator::free( void* p, size_t ) {
	if(p == 0) return;
#ifdef DEBUG_POOL_ALLOCATOR
	frees++;current--;
#endif
	block* dead_object = static_cast<block*>(p);

	dead_object->next = static_cast<block*>(head_of_free_list);
	head_of_free_list = dead_object;
}
typedef std::vector<fixed_size_allocator*> allocator_pool_t;
typedef allocator_pool_t::iterator allocator_pool_it;

static class _allocator_pool
{
public:
	_allocator_pool() : last_allocate_allocator(0), last_free_allocator(0) {
#ifdef LOG_POOL_ALLOCATOR_MEMORY_USAGE
	last_ok = last_access = 0;
#endif
	}
	~_allocator_pool() {
		for(allocator_pool_it it = allocator_pool.begin(); it!=allocator_pool.end(); it++)
			delete *it;
#ifdef LOG_POOL_ALLOCATOR_MEMORY_USAGE
		if(last_access) fprintf(stderr, "last_ok:%i(%i)=%i%%\n", last_ok, last_access, (last_ok*100)/last_access);
#endif
	}
	allocator_pool_t allocator_pool;
	fixed_size_allocator *last_allocate_allocator;
	fixed_size_allocator *last_free_allocator;
#ifdef LOG_POOL_ALLOCATOR_MEMORY_USAGE
	int last_ok;
	int last_access;
#endif
}allocator_pool;

static bool compare_allocator_pool(fixed_size_allocator *allocator, size_t Size) {
	return allocator->objectSize() < Size;
}

fixed_size_allocator *fixed_size_allocator::get(size_t size, bool for_alloc, const char *for_class) {
#ifdef LOG_POOL_ALLOCATOR_MEMORY_USAGE
	allocator_pool.last_access++;
#endif
	if(allocator_pool.last_allocate_allocator && allocator_pool.last_allocate_allocator->object_size==size) {
#ifdef LOG_POOL_ALLOCATOR_MEMORY_USAGE
		allocator_pool.last_ok++;
#endif
		return allocator_pool.last_allocate_allocator;
	}
	else if(allocator_pool.last_free_allocator && allocator_pool.last_free_allocator->object_size==size) {
#ifdef LOG_POOL_ALLOCATOR_MEMORY_USAGE
		allocator_pool.last_ok++;
#endif
		return allocator_pool.last_free_allocator;
	}
	allocator_pool_it it = lower_bound(allocator_pool.allocator_pool.begin(), allocator_pool.allocator_pool.end(), size, compare_allocator_pool);
//	allocator_pool_it it = allocator_pool.allocator_pool.find(size);
	if(it != allocator_pool.allocator_pool.end() && (*it)->objectSize() == size)
		return (for_alloc ? allocator_pool.last_allocate_allocator : allocator_pool.last_free_allocator) = *it;
	else if(for_alloc) {
		allocator_pool.allocator_pool.insert(it, allocator_pool.last_allocate_allocator=new fixed_size_allocator(64, size, for_class));
		return allocator_pool.last_allocate_allocator;
	} else
		ASSERT(0/* free called but not allocator defined*/);
	return 0;
}
