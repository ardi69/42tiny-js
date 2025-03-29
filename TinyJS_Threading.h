#ifndef TinyJS_Threading_h__
#define TinyJS_Threading_h__
#include "config.h"
#ifndef NO_THREADING
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

#ifdef HAVE_CXX_THREADS

#	include <mutex>
#	include <condition_variable>
typedef std::mutex CScriptMutex;
#define CScriptUniqueLock std::unique_lock 
typedef std::condition_variable CScriptCondVar;

#else

#	include<chrono>

class CScriptMutex {
public:
	CScriptMutex();
	~CScriptMutex();
	void lock() { mutex->lock(); }
	void unlock() { mutex->unlock(); }
	void *getRealMutex() { return mutex->getRealMutex(); }
	class CScriptMutex_t{
	public:
		virtual ~CScriptMutex_t();
		virtual void lock()=0;
		virtual void unlock()=0;
		virtual void *getRealMutex()=0;
	};
private:
	CScriptMutex_t *mutex;

};

template<class LockClass>
class CScriptUniqueLock {
public:
	CScriptUniqueLock(LockClass &Mutex) : mutex(Mutex) { mutex.lock(); }
	~CScriptUniqueLock() { mutex.unlock(); }
	LockClass &mutex;
};

class CScriptCondVar {
public:
	CScriptCondVar();
	~CScriptCondVar();
	void notify_one() { condVar->notify_one(); }
	void notify_all() { condVar->notify_all(); }
	void wait(CScriptUniqueLock<CScriptMutex> &Lock) { condVar->wait(Lock); }
	class CScriptCondVar_t {
	public:
		virtual ~CScriptCondVar_t();
		virtual void notify_one() = 0;
		virtual void notify_all() = 0;
		virtual void wait(CScriptUniqueLock<CScriptMutex> &lock) = 0;
		virtual int wait_for(CScriptUniqueLock<CScriptMutex> &lock, std::chrono::duration<long, std::milli> rel_time) = 0;
		virtual int wait_until(CScriptUniqueLock<CScriptMutex> &lock, std::chrono::system_clock::time_point abs_time) = 0;
		virtual int wait_until(CScriptUniqueLock<CScriptMutex> &lock, std::chrono::steady_clock::time_point abs_time) = 0;
	};
private:
	CScriptCondVar_t *condVar;
};

#endif /*HAVE_CXX_THREADS*/

class CScriptThread {
public:
	CScriptThread();
	virtual ~CScriptThread();
	static void yield();
	int Run(uint32_t timeout_ms = 800) { return thread->Run(timeout_ms); }
	int Stop(bool Wait=true) { return thread->Stop(Wait); }
	int retValue() { return thread->retValue(); }
	virtual int ThreadFnc()=0;
	virtual void ThreadFncFinished();
	bool isActiv() { return thread->isActiv(); }
	bool isRunning() { return thread->isRunning(); }
	bool isStarted() { return thread->isStarted(); }
	class CScriptThread_t{
	public:
		virtual ~CScriptThread_t(){}
		virtual int Run(uint32_t timeout_ms)=0;
		virtual int Stop(bool Wait)=0;
		virtual bool isActiv()=0;
		virtual bool isRunning()=0;
		virtual bool isStarted()=0;
		virtual int retValue()=0;
	};
private:
	CScriptThread_t *thread;
};

class CScriptCoroutine : protected CScriptThread {
public:
	CScriptCoroutine() : mainNotifies(0), coroutineNotifies(0) {}
	typedef struct{} StopIteration_t;
	static StopIteration_t StopIteration;
	bool next(); // returns true if coroutine is running
protected:
	virtual int Coroutine()=0;
	void yield() { if(!yield_no_throw()) throw StopIteration; }
	bool yield_no_throw();
private:
	virtual int ThreadFnc();
	virtual void ThreadFncFinished();
	//void notify_and_wait(CScriptCondVar &notifyCondVar, CScriptCondVar &waitCondVar, bool &waitFlag);
	CScriptMutex mutex;
	CScriptCondVar mainCondVar, coroutineCondVar;
	volatile int mainNotifies, coroutineNotifies;
};

#include <atomic>
class CScriptSpinLock {
public:
	CScriptSpinLock() { flag.clear(std::memory_order_release); }
	void lock() { while (flag.test_and_set(std::memory_order_acquire)) { CScriptThread::yield(); } }
	void unlock() { flag.clear(std::memory_order_release); }
private:
	std::atomic_flag flag;
};

#endif // NO_THREADING
#endif // TinyJS_Threading_h__
