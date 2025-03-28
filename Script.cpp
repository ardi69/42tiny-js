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


/*
 * This is a simple program showing how to use TinyJS
 */

#include "TinyJS.h"
#include <iostream>
using namespace TinyJS;

void js_print(const CFunctionsScopePtr &v, void *) {
	std::cout << "> " << v->getArgument("text")->toString() << std::endl;
}

void js_dump(const CFunctionsScopePtr &v, void *) {
	v->getContext()->getRoot()->trace(">  ");
}


char *topOfStack;
#define sizeOfStack 1*1024*1024 /* for example 1 MB depend of Compiler-Options */
#define sizeOfSafeStack 50*1024 /* safety area */


void registerFnc(CTinyJS *js) {
	js->addNative("function print(...text)", &js_print, 0);
//	js->addNative("function console.log(text)", &js_print, 0);
//  js->addNative("function dump()", &js_dump, js);
}


int main(int , char **) {
	char dummy;
	topOfStack = &dummy;
	CTinyJS *js = new CTinyJS{ registerFnc };
	/* Add a native function */
	/* Execute out bit of code - we could call 'evaluate' here if
		we wanted something returned */
	js->setStackBase(topOfStack-(sizeOfStack-sizeOfSafeStack));
	try {
		js->execute(" lets_quit = 0; function quit() { lets_quit = 1; } dump = this.dump;");
		js->execute("print(\"Interactive mode... Type quit(); to exit, or print(...); to print something, or dump() to dump the symbol table!\");");
	} catch (CScriptException &e) {
		std::cout << e.toString() << std::endl;
	}
	int lineNumber = 0;
	while (js->evaluate("lets_quit") == "0") {
		std::string buffer;
		if(!std::getline(std::cin, buffer)) break;
		try {
			js->execute(buffer, "console.input", lineNumber++);
		} catch (CScriptException &e) {
			std::cout << e.toString() << std::endl;
		}
	}
	delete js;
	return 0;
}
