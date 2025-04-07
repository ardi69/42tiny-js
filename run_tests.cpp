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


/*
 * This is a program to run all the tests in the tests folder...
 */

#include "TinyJS.h"
#include <fstream>
#include <filesystem>
#ifdef _WIN32
#   include <windows.h>
#   define waitKeypress() system("pause")
#else
#   define waitKeypress() do { printf("press Enter (end)"); getchar(); } while(0)
#	define SetConsoleOutputCP(cp) do {} while(0)
#	define OutputDebugString(s) do {} while(0)
#endif


#ifndef WITH_TIME_LOGGER
#define WITH_TIME_LOGGER 1
#endif
#include "time_logger.h"


using namespace TinyJS;

void js_print(const CFunctionsScopePtr & v, void *) {
	printf("> %s\n", v->getArgument("text")->toString().c_str());
}
bool run_test(const char *filename) {
	printf("TEST %s ", filename);
	OutputDebugString((std::string("TEST ") + filename + "\n").c_str());

	if (!std::filesystem::exists(filename)) {
		printf("Unable to open file! '%s'\n", filename);
		return false;
	}
	
	CTinyJS s;
	s.addNative("function print(text)", &js_print, 0);
	s.getRoot()->addChild("result", s.newScriptVar(0));
	TimeLoggerCreate(Test, true, filename);
	try {
		CScriptTokenizer tokens(0, filename);
		s.execute(tokens);
	} catch (CScriptException &e) {
		std::cout << e.toString() << std::endl;
	}
	bool pass = s.getRoot()->findChild("result")->toBoolean();
	TimeLoggerLogprint(Test);

	if (pass)
		printf("PASS\n");
	else {
		std::string fn = std::string(filename) + ".fail.txt";

		std::ofstream f(fn, std::fstream::out | std::fstream::binary);
		if (f) {
			f << s.getRoot()->CScriptVar::getParsableString();
			f.close();
		}
		std::cout << "FAIL - symbols written to " << fn << "\n";
	}
	return pass;
}

int main(int argc, char **argv) {
	// Windows: setzt den Zeichensatz der Konsole auf (UTF-8) für korrekte Darstellung von Sonderzeichen.
	SetConsoleOutputCP(65001);
	printf("TinyJS test runner\n");
	printf("USAGE:\n");
	printf("   ./run_tests [-k] tests/test001.js [tests/42tests/test002.js]   : run tests\n");
	printf("   ./run_tests [-k]                                               : run all tests\n");
	printf("   -k needs press enter at the end of runs\n");
	bool runAll = true, needKeyPress = false;
	for (int arg_num = 1; arg_num < argc; arg_num++) {
		if (argv[arg_num][0] == '-') {
			if (strcmp(argv[arg_num], "-k") == 0)
				needKeyPress = true;
		} else {
			run_test(argv[arg_num]);
			runAll = false;
		}
	}
	if (runAll) {
		int count = 0;
		int passed = 0;
		std::string mask[] = { "tests/test", "tests/42tests/test" };
		TimeLoggerCreate(Tests, true);
		//for(int i=0; i<100;++i)
		for (int js42 = 0; js42 < 2; js42++) {
			for (int test_num = 1; test_num < 1000; ++test_num) {
				std::string num = std::to_string(test_num);
				if (num.size() < 3) num = std::string(3 - num.size(), '0') + num;
				std::string fn = mask[js42] + num + ".42.js";
				if (!std::filesystem::exists(fn)) {
					fn = mask[js42] + num + ".js";
					if (!std::filesystem::exists(fn))
						break;
				}
				if (run_test(fn.c_str()))
					passed++;
				count++;
			}
		}
		printf("Done. %d tests, %d pass, %d fail\n", count, passed, count - passed);
		TimeLoggerLogprint(Tests);
	}
	if (needKeyPress) waitKeypress();
	return 0;
}
