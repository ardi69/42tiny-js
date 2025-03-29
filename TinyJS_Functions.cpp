/*
 * TinyJS
 *
 * A single-file Javascript-alike engine
 *
 * Authored By Gordon Williams <gw@pur3.co.uk>
 *
 * Copyright (C) 2009 Pur3 Ltd
 *

 * 42TinyJS
 *
 * A fork of TinyJS with the goal to makes a more JavaScript/ECMA compliant engine
 *
 * Authored / Changed By Armin Diedering <armin@diedering.de>
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


#include <math.h>
#include <cstdlib>
#include <sstream>
#include <algorithm>
#include <time.h>
#include "TinyJS.h"

namespace TinyJS {

// ----------------------------------------------- Actual Functions

static void scTrace(const CFunctionsScopePtr &c, void * userdata) {
	CTinyJS *js = (CTinyJS*)userdata;
	if(c->getArgumentsLength())
		c->getArgument(0)->trace();
	else
		js->getRoot()->trace("root");
}

static void scObjectDump(const CFunctionsScopePtr &c, void *) {
	c->getArgument("this")->trace("> ");
}

/*
static void scIntegerValueOf(const CFunctionsScopePtr &c, void *) {
	string str = c->getArgument("str")->toString();

	int val = 0;
	if (str.length()==1)
		val = str.operator[](0);
	c->setReturnVar(c->newScriptVar(val));
}
*/
static void scJSONStringify(const CFunctionsScopePtr &c, void *) {
	uint32_t UniqueID = c->getContext()->allocUniqueID();
	bool hasRecursion=false;
	c->setReturnVar(c->newScriptVar(c->getArgument("obj")->getParsableString("", "   ", UniqueID, hasRecursion)));
	c->getContext()->freeUniqueID();
	if(hasRecursion) c->throwError(TypeError, "cyclic object value");
}

static void scArrayIncludes(const CFunctionsScopePtr &c, void *data) {
	CScriptVarPtr obj = c->getArgument("obj");
	CScriptVarPtr arr = c->getArgument("this");
	uint32_t len = arr->getLength();
	uint32_t from = 0;
	if(c->getArgumentsLength() >1) from = c->getArgument(1)->toNumber().clampIndex(len).toUInt32();
	auto begin = lower_bound(arr->Childs.begin(), arr->Childs.end(), from);
	auto end = lower_bound(begin, arr->Childs.end(), len);
	for (auto it = begin; it != end; ++it) {
		if (obj->mathsOp(it->getter(), LEX_EQUAL)->toBoolean()) {
			c->setReturnVar(c->constScriptVar(true));
			return;
		}
	}
	c->setReturnVar(c->constScriptVar(false));
}

static void scArrayRemove(const CFunctionsScopePtr &c, void *data) {
	CScriptVarPtr obj = c->getArgument("obj");
	CScriptVarPtr arr = c->getArgument("this");
	uint32_t len = arr->getLength();
	uint32_t offset = 0;
	for(SCRIPTVAR_CHILDS_it it= lower_bound(arr->Childs.begin(), arr->Childs.end(), static_cast<uint32_t>(0)); it != arr->Childs.end(); ++it) {
		while(it != arr->Childs.end() && obj->mathsOp(it->getter(), LEX_EQUAL)->toBoolean()) {
			it = arr->Childs.erase(it);
			offset++;
		}
		if(offset && it != arr->Childs.end())
			(*it)->reName((*it)->getName().toArrayIndex() - offset);
	}
	if (offset) {
		c->setProperty(arr, "length", c->newScriptVar(len - offset));
		
	}
}

static void scArrayJoin(const CFunctionsScopePtr &c, void *data) {
	auto sepVar = c->getArgument("separator");
	std::string sep = sepVar->isUndefined() ? ", " : c->getArgument("separator")->toString();
	CScriptVarPtr arr = c->getArgument("this");

	std::ostringstream sstr;
	uint32_t l = arr->getLength();
	for (uint32_t i=0;i<l;i++) {
		if (i>0) sstr << sep;
		sstr << c->getPropertyValue(arr, i)->toString();
	}

	c->setReturnVar(c->newScriptVar(sstr.str()));
}

static void scArrayFill(const CFunctionsScopePtr &c, void *data) {
	CScriptVarPtr arr = c->getArgument("this");
	uint32_t len = c->getLength(arr);
	uint32_t args = c->getArgumentsLength();
	CScriptVarPtr value = c->getArgument(0);
	uint32_t start=0, end=len;
	if (args > 1) start = c->getArgument(1)->toNumber().clampIndex(len).toUInt32();
	if (args > 2) end = c->getArgument(2)->toNumber().clampIndex(len).toUInt32();
	for(; start < end; ++start) c->setProperty(arr, start, value);
	c->setReturnVar(arr);
}

static void scArrayPop(const CFunctionsScopePtr &c, void *data) {
	CScriptVarPtr arr = c->getArgument("this");
	uint32_t len = c->getLength(arr);
	if(len) {
		c->setReturnVar(c->getPropertyValue(arr, len-1));
		c->removeOwnProperty(arr, len - 1);
		c->setProperty(arr, "length", c->newScriptVar(len - 1));
	} else
		c->setReturnVar(c->constScriptVar(Undefined));
}

static void scArrayPush(const CFunctionsScopePtr &c, void *data) {
	CScriptVarPtr arr = c->getArgument("this");
	uint32_t len = c->getLength(arr);
	uint32_t args = c->getArgumentsLength();
	bool rangeError = len > (std::numeric_limits<uint32_t>::max() - args);
	if (rangeError) args = std::numeric_limits<uint32_t>::max() - len;
	for (uint32_t src = 0, dst = len; src < args; ++src, ++dst)
		c->setProperty(arr, dst, c->getArgument(src));
	c->setProperty(arr, "length", c->newScriptVar(len + args));
	if (rangeError) c->throwError(RangeError, "Failed to set the 'length' property on 'Array': Invalid array length");
}

namespace {
	class cmp_fnc {
	public:
		cmp_fnc(const CFunctionsScopePtr &Scope, CScriptVarFunctionPtr Fnc) : c(Scope), fnc(Fnc) {}
		bool operator()(CScriptVarLinkPtr a,CScriptVarLinkPtr b) {
			auto _a = a.getter();
			auto _b = b.getter();
			if(_a->getVarPtr()->isUndefined()) {
				return false;
			} else if(_b->getVarPtr()->isUndefined())
				return true;
			else if(fnc) {
				std::vector<CScriptVarPtr> arguments;
				arguments.push_back(_a);
				arguments.push_back(_b);
				return c->getContext()->callFunction(fnc, arguments)->toNumber().toInt32() < 0;
			}
			else {
				return _a->toString() < _b->toString();
			}
		}
		const CFunctionsScopePtr &c;
		CScriptVarFunctionPtr fnc;
	};
	bool cmp_by_name(const CScriptVarLinkPtr &a, const CScriptVarLinkPtr &b) {
		return a < b->getName();
	}
}
static void scArraySort(const CFunctionsScopePtr &c, void *data) {
	CScriptVarPtr arr = c->getArgument("this");
	CScriptVarFunctionPtr cmp_fnc; 
	uint32_t len = arr->getLength();
	if(len > 1) {
		int args = c->getArgumentsLength();
		if(args) {
			cmp_fnc = c->getArgument(0);
			if(!cmp_fnc) c->throwError(TypeError, "invalid Array.prototype.sort argument");
		}
		SCRIPTVAR_CHILDS_it begin = lower_bound(arr->Childs.begin(), arr->Childs.end(), static_cast<uint32_t>(0));
		SCRIPTVAR_CHILDS_it end = lower_bound(begin, arr->Childs.end(), len);
		if(begin != end) {

			/* in ECMAScript the sort algorithm is not specified
			 * in some cases sort throws a TypeError e.G. an array element is read-only, non-configurable, getter or setter
			 * this will result in a partially sorted array
			 * in worst case the sort algorithm results in an endless loop (more getters with constant return value)
			 *
			 * 42TinyJS checks before sort if an element read-only, setter or getter and throws immediately a TypeError
			 */
			uint32_t lastIndex = (*(end - 1))->getName().toIndex();
			uint32_t countOfEntries = static_cast<uint32_t>(std::distance(begin, end));
			uint32_t countOfHoles = lastIndex + 1 - countOfEntries;
			uint32_t isExtensible = arr->isExtensible();
			uint32_t idx = 0;
			for (SCRIPTVAR_CHILDS_it it = begin; it != end; ++it, ++idx) {
				if (countOfHoles && !isExtensible && (*it)->getName().toIndex() != idx)
					c->throwError(TypeError, "Cannot add property " + std::to_string(idx) + ", array is not extensible");
				if (idx >= countOfEntries - countOfHoles && (*it)->getName().toIndex() > lastIndex - countOfHoles && !(*it)->isConfigurable())
					c->throwError(TypeError, "Cannot delete property '" + (*it)->getName().toString() + "' array entry is not configurable");
				if (!(*it)->isWritable()) c->throwError(TypeError, "Cannot assign to read-only property '" + (*it)->getName().toString() + "'");
				if ((*it)->getVarPtr()->isAccessor()) c->throwError(TypeError, "Cannot sort an array containing accessors, e.g., '" + (*it)->getName().toString() + "'");
			}
			//	if (!(*it)->isConfigurable()) c->throwError(TypeError, "property " + (*it)->getName() + " is non-configurable");

			sort(begin, end, TinyJS::cmp_fnc(c, cmp_fnc));
			idx = 0;
			for (SCRIPTVAR_CHILDS_it it = begin; it != end; ++it, ++idx)
				(*it)->reName(idx);
			//sort(begin, arr->Childs.end(), cmp_by_name);
		}
	}
	c->setReturnVar(arr);
}

static void scArrayMap(const CFunctionsScopePtr &c, void *data) {
	CScriptVarPtr arr = c->getArgument("this");
	uint32_t len = arr->getLength();
	CScriptVarFunctionPtr fnc = c->getArgument("fnc");
	CScriptVarPtr thisArg; if(c->getArgumentsLength() > 1) thisArg = c->getArgument("thisArg");
	if (!fnc) c->throwError(TypeError, "invalid Array.prototype.map argument");
	CScriptVarPtr newArr = c->newScriptVar(Array);
	if (len > 0) {
		SCRIPTVAR_CHILDS_it begin = lower_bound(arr->Childs.begin(), arr->Childs.end(), static_cast<uint32_t>(0));
		SCRIPTVAR_CHILDS_it end = lower_bound(begin, arr->Childs.end(), len);
		for (SCRIPTVAR_CHILDS_it it = begin; it != end; ++it) {
			std::vector<CScriptVarPtr> arguments{ (*it).getter(), c->newScriptVar((*it)->getName().toIndex()), arr};
			//c->setProperty(newArr, (*it)->getName(), c->getContext()->callFunction(fnc, arguments, thisArg));
			newArr->addChild((*it)->getName(), c->getContext()->callFunction(fnc, arguments, thisArg)); // addChild is used because fastern than srtProperty
		}
	}
	c->setReturnVar(newArr);
}

static void scArrayShift(const CFunctionsScopePtr &c, void *data) {
	CScriptVarPtr arr = c->getArgument("this");
	uint32_t len = arr->getLength();
	if (len == 0) {
		c->setReturnVar(c->constScriptVar(Undefined));
		return;
	}
	
	CScriptVarPtr first = c->getPropertyValue(arr, static_cast<uint32_t>(0));
	for (uint32_t i = 1; i < len; ++i) {
		auto v = arr->findChild(i);
		if (v)
			c->setProperty(arr, i - 1, c->getPropertyValue(arr, i));
		else
			c->removeOwnProperty(arr, i - 1);
	}
	c->removeOwnProperty(arr, len - 1); // remove last element
	c->setProperty(arr, "length", c->newScriptVar(len - 1));
	c->setReturnVar(first);
}


static void scArrayUnshift(const CFunctionsScopePtr &c, void *data) {
	CScriptVarPtr arr = c->getArgument("this");
	uint32_t len = arr->getLength();
	uint32_t args = c->getArgumentsLength();
	if (args) {
		bool rangeError = len > (std::numeric_limits<uint32_t>::max() - args);
		if (rangeError) args = std::numeric_limits<uint32_t>::max() - len;

		if (len > 0) for (auto src = len - 1, dst = src + args; dst >= args; --src, --dst)
			c->setProperty(arr, dst, c->getPropertyValue(arr, src));
		for (auto i = 0U; i < args; ++i)
			c->setProperty(arr, i, c->getArgument(i));
		c->setProperty(arr, "length", c->newScriptVar(len + args));
		if (rangeError) c->throwError(RangeError, "Failed to set the 'length' property on 'Array': Invalid array length");
	}
	c->setReturnVar(c->newScriptVar(len + args));
}

static void scArraySlice(const CFunctionsScopePtr &c, void *data) {
	CScriptVarPtr arr = c->getArgument("this");
	auto argumentsLength = c->getArgumentsLength();
	int32_t len = arr->getLength();
	int32_t start = 0, end = (int32_t)len;
	if (argumentsLength > 0)	start = c->getArgument(0)->toNumber().clampIndex(len).toUInt32();
	if (argumentsLength > 1)	end = c->getArgument(1)->toNumber().clampIndex(len).toUInt32();
	CScriptVarPtr newArr = c->newScriptVar(Array);
	for (int32_t src = start, dst = 0; src < end; ++src, ++dst) {
		newArr->addChild(dst, c->getPropertyValue(arr, src)); // addChild is used because fastern than srtProperty
		//c->setProperty(newArr, i, c->getPropertyValue(arr, start + i));
	}
	c->setReturnVar(newArr);
}

static void scArraySplice(const CFunctionsScopePtr &c, void *data) {
	CScriptVarPtr arr = c->getArgument("this");
	uint32_t len = arr->getLength();
	uint32_t args = c->getArgumentsLength();
	uint32_t start = 0, delCount = 0;
	if (args > 0) start = c->getArgument(0)->toNumber().clampIndex(len).toUInt32();
	if (args > 1) delCount = c->getArgument(1)->toNumber().clamp(0, len - start).toUInt32();

	CScriptVarPtr removed = c->newScriptVar(Array);
	for (uint32_t i = 0; i < delCount; ++i) {
		auto v = arr->findChild(start + i);
		if (v) {
			// c->setProperty(removed, i, c->getPropertyValue(arr, start + i));
			removed->addChild(i, v.getter()); // addChild is used because fastern than srtProperty
		}
	}
	args = args > 2 ? args - 2 : 0;
	if (len) { // Ensure the length is non-zero
		if (args < delCount) { // If the number of elements to add is less than the number to delete
			for (uint32_t dst = start + args, src = start + delCount; src < len; ++dst, ++src) {
				// Shift remaining elements downwards to fill in the deleted space
				c->setProperty(arr, dst, c->getPropertyValue(arr, src));
			}
			for (uint32_t dst = len + args - delCount; dst < len; ++dst) {
				// Remove any remaining elements after the deletion
				c->removeOwnProperty(arr, dst);
			}
		} else if (args > delCount) { // If the number of elements to add is greater than the number to delete
			uint32_t shift = args - delCount; // Calculate how many extra elements are being added
			for (uint32_t src = len, dst = src + shift; src > start + delCount; --src, --dst) {
				// Move elements from the end to make space for the added elements
				c->setProperty(arr, dst - 1, c->getPropertyValue(arr, src - 1));
			}
		}
	}

	// Insert the new elements at the specified starting position
	for (uint32_t src = 0, dst = start; src < args; ++src, ++dst) {
		c->setProperty(arr, dst, c->getArgument(src + 2)); // Set the new arguments starting from position 'start'
	}
	c->setProperty(arr, "length", c->newScriptVar(len + args - delCount));
	c->setReturnVar(removed);
}

static void scArrayForEach(const CFunctionsScopePtr &c, void *data) {
	CScriptVarPtr arr = c->getArgument("this");
	CScriptVarFunctionPtr callback = c->getArgument("callback");
	if (!callback) c->throwError(TypeError, "forEach requires a callback function");
	uint32_t len = arr->getLength();

	SCRIPTVAR_CHILDS_it begin = lower_bound(arr->Childs.begin(), arr->Childs.end(), static_cast<uint32_t>(0));
	SCRIPTVAR_CHILDS_it end = lower_bound(begin, arr->Childs.end(), len);
	for (auto it = begin; it != end; ++it) {
		auto &name = (*it)->getName();
		std::vector<CScriptVarPtr> args = { it->getter(), c->newScriptVar((*it)->getName().toArrayIndex()), arr};
		c->getContext()->callFunction(callback, args);
	}
}

static void scArrayIndexOf(const CFunctionsScopePtr &c, void *data) {
	CScriptVarPtr arr = c->getArgument("this");
	CScriptVarPtr search = c->getArgument(0);
	uint32_t len = arr->getLength();

	SCRIPTVAR_CHILDS_it begin = lower_bound(arr->Childs.begin(), arr->Childs.end(), static_cast<uint32_t>(0));
	SCRIPTVAR_CHILDS_it end = lower_bound(begin, arr->Childs.end(), len);
	for (auto it = begin; it != end; ++it) {
		if (search->mathsOp((*it).getter(), LEX_EQUAL)->toBoolean()) {
			c->setReturnVar(c->newScriptVar((*it)->getName().toArrayIndex()));
			return;
		}
	}
	c->setReturnVar(c->newScriptVar(-1));
}

static void scArrayReduce(const CFunctionsScopePtr &c, void *data) {
	CScriptVarPtr arr = c->getArgument("this");
	uint32_t len = arr->getLength();
	CScriptVarFunctionPtr callback = c->getArgument("callback");
	if (!callback) c->throwError(TypeError, "Reduce requires a callback function");

	auto begin = lower_bound(arr->Childs.begin(), arr->Childs.end(), static_cast<uint32_t>(0));
	auto end = lower_bound(begin, arr->Childs.end(), len);
	if (begin != end) {
		CScriptVarPtr accumulator;// = c->getArgument(1);
		if (c->getArgumentsLength() > 1)
			accumulator = c->getArgument(1);
		else {
			accumulator = begin++->getter();
		}
		for (auto it = begin; it != end; ++it) {
			std::vector<CScriptVarPtr> args{ accumulator, it->getter(), c->newScriptVar((*it)->getName().toArrayIndex()), arr };
			accumulator = c->getContext()->callFunction(callback, args);
		}
		c->setReturnVar(accumulator);
	} else if (c->getArgumentsLength() > 1)
		c->setReturnVar(c->getArgument(1));
	else
		c->throwError(TypeError, "Reduce of empty array with no initiel value");
}

static void scArrayReduceRight(const CFunctionsScopePtr &c, void *data) {
	CScriptVarPtr arr = c->getArgument("this");
	uint32_t len = arr->getLength();
	CScriptVarFunctionPtr callback = c->getArgument("callback");
	if (!callback) c->throwError(TypeError, "Reduce requires a callback function");

	auto end = std::make_reverse_iterator(lower_bound(arr->Childs.begin(), arr->Childs.end(), static_cast<uint32_t>(0)));
	auto begin = std::make_reverse_iterator(lower_bound(end.base(), arr->Childs.end(), len));
	if (begin != end) {
		CScriptVarPtr accumulator;// = c->getArgument(1);
		if (c->getArgumentsLength() > 1)
			accumulator = c->getArgument(1);
		else {
			accumulator = begin++->getter();
		}
		for (auto it = begin; it != end; ++it) {
			std::vector<CScriptVarPtr> args{ accumulator, it->getter(), c->newScriptVar((*it)->getName().toArrayIndex()), arr };
			accumulator = c->getContext()->callFunction(callback, args);
		}
		c->setReturnVar(accumulator);
	} else if (c->getArgumentsLength() > 1)
		c->setReturnVar(c->getArgument(1));
	else
		c->throwError(TypeError, "Reduce of empty array with no initiel value");
}


// ----------------------------------------------- Register Functions
extern "C" void _registerFunctions(CTinyJS *tinyJS) {
	tinyJS->addNative("function trace()", scTrace, tinyJS, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function Object.prototype.dump()", scObjectDump, 0, SCRIPTVARLINK_BUILDINDEFAULT);

//	tinyJS->addNative("function Integer.valueOf(str)", scIntegerValueOf, 0, SCRIPTVARLINK_BUILDINDEFAULT); // value of a single character
	tinyJS->addNative("function JSON.stringify(obj, replacer)", scJSONStringify, 0, SCRIPTVARLINK_BUILDINDEFAULT); // convert to JSON. replacer is ignored at the moment

	// Array
	tinyJS->addNative("function Array.prototype.includes(obj)", scArrayIncludes, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function Array.prototype.remove(obj)", scArrayRemove, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function Array.prototype.join(separator)", scArrayJoin, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function Array.prototype.fill()", scArrayFill, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function Array.prototype.pop()", scArrayPop, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function Array.prototype.push()", scArrayPush, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function Array.prototype.sort()", scArraySort, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function Array.prototype.map(fnc, thisArg)", scArrayMap, 0, SCRIPTVARLINK_BUILDINDEFAULT);

	tinyJS->addNative("function Array.prototype.shift()", scArrayShift, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function Array.prototype.unshift()", scArrayUnshift, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function Array.prototype.slice()", scArraySlice, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function Array.prototype.splice()", scArraySplice, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function Array.prototype.forEach(callback)", scArrayForEach, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function Array.prototype.indexOf()", scArrayIndexOf, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function Array.prototype.reduce(callback, initialValue)", scArrayReduce, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function Array.prototype.reduceRight(callback, initialValue)", scArrayReduceRight, 0, SCRIPTVARLINK_BUILDINDEFAULT);

}

} /* namespace TinyJS */