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


#ifndef time_logger_h__
#define time_logger_h__
#if WITH_TIME_LOGGER

#include <cstdint>
#include <iostream>
#include <iomanip>
#include <string>
#include <chrono>

class TimeLogger {
public:
	TimeLogger(const char *Name, bool Start=false, const char *extName=0) 
		: 
	name(Name), start_time(std::chrono::high_resolution_clock::now()), sum_time(0), calls(0) {
		if (Start) start_time = std::chrono::high_resolution_clock::now();
		if(extName) name = name + "[" + extName + "]";
	}
	TimeLogger(const char *Name, const char *extName, bool Start=false) 
		: 
	name(Name), start_time(), sum_time(0), calls(0) {
		if (Start) start_time = std::chrono::high_resolution_clock::now();
		if (extName) name = name + "[" + extName + "]";
	}
	~TimeLogger() {
		printLog();
	}
	void startTimer() {
		start_time = std::chrono::high_resolution_clock::now();
	}
	void stopTimer() {
		if(!start_time.time_since_epoch().count()) return;
		sum_time += std::chrono::high_resolution_clock::now() - start_time;
		calls++;
		start_time = std::chrono::high_resolution_clock::time_point();
	}
	void printLog() {
		if(start_time.time_since_epoch().count()) stopTimer();
		if (calls >= 1) {
			std::cout << "Timer(" << name << ") = "
				<< std::chrono::duration_cast<std::chrono::seconds>(sum_time).count() << "."
				<< std::setfill('0') << std::setw(6) << std::chrono::duration_cast<std::chrono::microseconds>(sum_time % std::chrono::seconds(1)).count() << std::setw(1)
				<< " sec";
			if (calls > 1)
				std::cout << " (called " << calls << "times) -> " << std::chrono::duration_cast<std::chrono::microseconds>(sum_time % std::chrono::seconds(1)).count() << " microsec per call";
			std::cout << std::endl;
		}
		calls = 0; sum_time = sum_time.zero();
	}
private:
	std::string name;
	std::chrono::high_resolution_clock::time_point start_time;
	std::chrono::high_resolution_clock::duration sum_time;
	uint32_t calls;
};

class _TimeLoggerHelper {
public:
	_TimeLoggerHelper(TimeLogger &Tl) : tl(Tl) { tl.startTimer(); }
	~_TimeLoggerHelper() { tl.stopTimer(); }
private:
	TimeLogger &tl;
};
#	define TimeLoggerCreate(a, ...) TimeLogger a##_TimeLogger(#a,##__VA_ARGS__)
#	define TimeLoggerStart(a) a##_TimeLogger.startTimer()
#	define TimeLoggerStop(a) a##_TimeLogger.stopTimer()
#	define TimeLoggerLogprint(a) a##_TimeLogger.printLog()
#	define TimeLoggerHelper(a) _TimeLoggerHelper a##_helper(a##_TimeLogger)
#else /* _DEBUG */
#	define TimeLoggerCreate(...)
#	define TimeLoggerStart(...) do{}while(0)
#	define TimeLoggerStop(...) do{}while(0)
#	define TimeLoggerLogprint(a) do{}while(0)
#	define TimeLoggerHelper(a)  do{}while(0)
#endif /* _DEBUG */



#endif // time_logger_h__
