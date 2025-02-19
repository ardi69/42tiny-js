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

static void scArrayContains(const CFunctionsScopePtr &c, void *data) {
	CScriptVarPtr obj = c->getArgument("obj");
	CScriptVarPtr arr = c->getArgument("this");

	uint32_t l = arr->getLength();
	CScriptVarPtr equal = c->constScriptVar(Undefined);
	for (uint32_t i=0;i<l;i++) {
		equal = obj->mathsOp(arr->getProperty(i), LEX_EQUAL);
		if(equal->toBoolean()) {
			c->setReturnVar(c->constScriptVar(true));
			return;
		}
	}
	c->setReturnVar(c->constScriptVar(false));
}

static void scArrayRemove(const CFunctionsScopePtr &c, void *data) {
	CScriptVarPtr obj = c->getArgument("obj");
	CScriptVarPtr arr = c->getArgument("this");
//	uint32_t l = arr->getLength();
	uint32_t offset = 0;
	for(SCRIPTVAR_CHILDS_it it= lower_bound(arr->Childs.begin(), arr->Childs.end(), "0"); it != arr->Childs.end(); ++it) {
		while(it != arr->Childs.end() && obj->mathsOp(it->getter(), LEX_EQUAL)->toBoolean()) {
			it = arr->Childs.erase(it);
			offset++;
		}
		if(offset && it != arr->Childs.end())
			(*it)->reName(std::to_string((uint32_t)strtoul((*it)->getName().c_str(), 0, 10)-offset) );
	}
	if (offset) {
		arr->setProperty("length", c->newScriptVar(arr->getLength() - offset));
		
	}
}

static void scArrayJoin(const CFunctionsScopePtr &c, void *data) {
	std::string sep = c->getArgument("separator")->toString();
	CScriptVarPtr arr = c->getArgument("this");

	std::ostringstream sstr;
	uint32_t l = arr->getLength();
	for (uint32_t i=0;i<l;i++) {
		if (i>0) sstr << sep;
		sstr << arr->getProperty(i)->toString();
	}

	c->setReturnVar(c->newScriptVar(sstr.str()));
}

static void scArrayFill(const CFunctionsScopePtr &c, void *data) {
	CScriptVarPtr arr = c->getArgument("this");
	uint32_t len = c->getLength(arr);
	CNumber startNumber=0, endNumber=len;
	CScriptVarPtr value = c->getArgument(0);
	CScriptVarPtr startVar = c->getArgument(1);
	CScriptVarPtr endVar = c->getArgument(2);
	c->setReturnVar(arr);
	if(!startVar->isUndefined()) startNumber = startVar->toNumber();
	if(!endVar->isUndefined()) endNumber = endVar->toNumber();
	uint32_t start = (startNumber.sign()<0 ? startNumber+len : startNumber).clamp(0, len).toUInt32();
	uint32_t end = (endNumber.sign()<0 ? endNumber+len : endNumber).clamp(0, len).toUInt32();
	for(; start < end; ++start) c->setProperty(arr, start, value);
}

static void scArrayPop(const CFunctionsScopePtr &c, void *data) {
	CScriptVarPtr arr = c->getArgument("this");
	uint32_t len = c->getLength(arr);
	if(len) {
		c->setReturnVar(c->getPropertyValue(arr, len-1));
		arr->removeChild(std::to_string(len-1));
	} else
		c->setReturnVar(c->constScriptVar(Undefined));
}

static void scArrayPush(const CFunctionsScopePtr &c, void *data) {
	CScriptVarPtr arr = c->getArgument("this");
//	uint32_t len = c->getLength(arr);
	uint32_t args = c->getArgumentsLength();
	for (uint32_t i=0; i<args; ++i)
		c->setProperty(arr, i, c->getArgument(i));
}

namespace {
	class cmp_fnc {
	public:
		cmp_fnc(const CFunctionsScopePtr &Scope, CScriptVarFunctionPtr Fnc) : c(Scope), fnc(Fnc) {}
		bool operator()(CScriptVarLinkPtr a,CScriptVarLinkPtr b) {
			if(a->getVarPtr()->isUndefined()) {
				return false;
			} else if(b->getVarPtr()->isUndefined())
				return true;
			else if(fnc) {
				std::vector<CScriptVarPtr> arguments;
				arguments.push_back(a);
				arguments.push_back(b);
				return c->getContext()->callFunction(fnc, arguments)->toNumber().toInt32() < 0;
			}
			else {
				return a->toString() < b->toString();
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
		SCRIPTVAR_CHILDS_it begin = lower_bound(arr->Childs.begin(), arr->Childs.end(), "0");
		/* in ECMAScript the sort algorithm is not specified
		 * in some cases sort throws a TypeError e.G. an array element is read-only, non-configurable, getter or setter
		 * this will result in a partially sorted array 
		 * in worst case the sort algorithm results in an endless loop (more getters with constant return value)
		 *
		 * 42TinyJS checks before sort if an element read-only, setter or getter and throws immediately a TypeError
		 */
		for(SCRIPTVAR_CHILDS_it it = begin; it != arr->Childs.end(); ++it) {
			if(!(*it)->isWritable()) c->throwError(TypeError, "property "+(*it)->getName()+" is read-only");
			if(!(*it)->isConfigurable()) c->throwError(TypeError, "property "+(*it)->getName()+" is non-configurable");
			if(!(*it)->getVarPtr()->isAccessor()) c->throwError(TypeError, "property "+(*it)->getName()+" is a getter and/or a setter");
		}
		sort(begin, arr->Childs.end(), TinyJS::cmp_fnc(c, cmp_fnc));
		uint32_t idx = 0;
		for(SCRIPTVAR_CHILDS_it it=begin; it != arr->Childs.end(); ++it, ++idx)
			(*it)->reName(std::to_string(idx));
		sort(begin, arr->Childs.end(), cmp_by_name);
	}
	c->setReturnVar(arr);
}


// ----------------------------------------------- Register Functions
extern "C" void _registerFunctions(CTinyJS *tinyJS) {
	tinyJS->addNative("function trace()", scTrace, tinyJS, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function Object.prototype.dump()", scObjectDump, 0, SCRIPTVARLINK_BUILDINDEFAULT);

//	tinyJS->addNative("function Integer.valueOf(str)", scIntegerValueOf, 0, SCRIPTVARLINK_BUILDINDEFAULT); // value of a single character
	tinyJS->addNative("function JSON.stringify(obj, replacer)", scJSONStringify, 0, SCRIPTVARLINK_BUILDINDEFAULT); // convert to JSON. replacer is ignored at the moment

	// Array
	tinyJS->addNative("function Array.prototype.contains(obj)", scArrayContains, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function Array.prototype.remove(obj)", scArrayRemove, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function Array.prototype.join(separator)", scArrayJoin, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function Array.prototype.fill()", scArrayFill, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function Array.prototype.pop()", scArrayPop, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function Array.prototype.push()", scArrayPush, 0, SCRIPTVARLINK_BUILDINDEFAULT);
	tinyJS->addNative("function Array.prototype.sort()", scArraySort, 0, SCRIPTVARLINK_BUILDINDEFAULT);
}

} /* namespace TinyJS */