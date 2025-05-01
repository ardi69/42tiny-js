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


#ifndef pool_allocator_h__
#define pool_allocator_h__

#include <typeinfo>
#include <stdint.h>
#include <string>
#include "config.h"
#ifndef NO_THREADING
#	include "TinyJS_Threading.h"
#endif
/************************************************************************
 * TinyJS must many many times allocates and frees objects
 * To prevent memory fragmentation and speed ups allocates & frees, i have
 * added to 42TinyJS a pool_allocator. This allocator allocates every 64 objects
 * as a pool of objects. Is an object needed it can faster allocated from this pool as 
 * from the heap.
 ************************************************************************/ 

namespace TinyJS {

namespace fixed_size_allocator {

void *alloc(size_t size);
void free(void *p, size_t size);

} /* namespace fixed_size_allocator */

template <typename T>
class SmallObjectAllocator {
public:
	using value_type = T;
	static constexpr std::size_t MaxObjectSize = 256;

	SmallObjectAllocator() noexcept = default;
	template <class U> SmallObjectAllocator(const SmallObjectAllocator<U> &) noexcept {}

	T *allocate(std::size_t n) {
		if (n == 0) return nullptr;
		auto size = n * sizeof(T);
		if (size <= MaxObjectSize) {
			return static_cast<T *>(TinyJS::fixed_size_allocator::alloc(size));
		} else {
			return static_cast<T *>(::operator new(size));
		}
	}

	void deallocate(T *p, std::size_t n) noexcept {
		if (!p) return;

		auto size = n * sizeof(T);
		if (size <= MaxObjectSize) {
			TinyJS::fixed_size_allocator::free(p, size);
		} else {
			::operator delete(p);
		}
	}

	// C++17-konform: Rebind für andere Typen (nicht zwingend, aber hilfreich für einige STL-Container)
	template <class U>
	struct rebind {
		using other = SmallObjectAllocator<U>;
	};
};

// Gleichheit: Alle Instanzen dieses Allocators gelten als gleich (kein Zustand)
template <class T, class U>
constexpr bool operator==(const SmallObjectAllocator<T> &, const SmallObjectAllocator<U> &) noexcept { return true; }

template <class T, class U>
constexpr bool operator!=(const SmallObjectAllocator<T> &, const SmallObjectAllocator<U> &) noexcept { return false; }


} /* namespace TinyJS */
#endif // pool_allocator_h__
