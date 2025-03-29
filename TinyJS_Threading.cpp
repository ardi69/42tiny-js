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

#include "TinyJS_Threading.h"
#include <exception>
#include <chrono>
#include <cstdio>
#include <cstddef>

#undef HAVE_THREADING
#if !defined(NO_THREADING) && !defined(HAVE_CUSTOM_THREADING_IMPL)
#	define HAVE_THREADING
#	ifdef HAVE_CXX_THREADS
#		include <thread>
#	else
#		if defined(_WIN32) && !defined(HAVE_PTHREAD)
#			include <windows.h>
#			include <process.h>
#		else
#			include <pthread.h>
#			ifndef HAVE_PTHREAD
#				define HAVE_PTHREAD
#			endif
#		endif
#	endif
#endif

#ifdef HAVE_THREADING 

//////////////////////////////////////////////////////////////////////////
// Mutex
//////////////////////////////////////////////////////////////////////////
#ifndef HAVE_CXX_THREADS

#ifndef HAVE_PTHREAD
// simple mutex 4 windows
#	define pthread_mutex_t CRITICAL_SECTION
#	define pthread_mutex_init(m, a)	(InitializeCriticalSection(m), 0)
#	define pthread_mutex_destroy(m)	do {} while(0)
#	define pthread_mutex_lock(m)		EnterCriticalSection(m)
#	define pthread_mutex_unlock(m)	LeaveCriticalSection(m)

#endif
CScriptMutex::CScriptMutex_t::~CScriptMutex_t() {}

class CScriptMutex_impl : public CScriptMutex::CScriptMutex_t {
public:
	CScriptMutex_impl() {
		pthread_mutex_init(&mutex, NULL);
	}
	virtual ~CScriptMutex_impl() {
		pthread_mutex_destroy(&mutex);
	}
	virtual void lock() {
		pthread_mutex_lock(&mutex);
	}
	virtual void unlock() {
		pthread_mutex_unlock(&mutex);
	}
	void *getRealMutex() { return &mutex; }
	pthread_mutex_t mutex;
};


CScriptMutex::CScriptMutex() {
	mutex = new CScriptMutex_impl;
}

CScriptMutex::~CScriptMutex() {
	delete mutex;
}

#endif /* !HAVE_CXX_THREADS */

//////////////////////////////////////////////////////////////////////////
// CondVar
//////////////////////////////////////////////////////////////////////////

#ifndef HAVE_CXX_THREADS

#ifndef HAVE_PTHREAD
// simple conditional Variable 4 windows

class condition_variable_impl
{
public:
	condition_variable_impl() : mNumWaiters(0), mSemaphore(CreateEvent(NULL, FALSE, FALSE, NULL)), mWakeEvent(CreateEvent(NULL, FALSE, FALSE, NULL)) {}
	~condition_variable_impl() { CloseHandle(mSemaphore); CloseHandle(mWakeEvent); }
	void wait(pthread_mutex_t *lock) {
		{
			CScriptUniqueLock<CScriptMutex> quard(mMutex);
			mNumWaiters++;
		}
		pthread_mutex_unlock(lock);
		WaitForSingleObject(mSemaphore, INFINITE);
		mNumWaiters--;
		SetEvent(mWakeEvent);
		pthread_mutex_lock(lock);
	}
	int wait_until(pthread_mutex_t *lock, timespec *ts) {
		{
			CScriptUniqueLock<CScriptMutex> quard(mMutex);
			mNumWaiters++;
		}
		auto rel_time = std::chrono::steady_clock::time_point(std::chrono::seconds(ts->tv_sec) + std::chrono::nanoseconds(ts->tv_nsec)) - std::chrono::steady_clock::now();
		if (rel_time.count() <= 0) return ETIMEDOUT;
		pthread_mutex_unlock(lock);
		auto ret = WaitForSingleObject(mSemaphore, std::chrono::duration_cast<std::chrono::duration<DWORD, std::milli>>(rel_time).count());
		mNumWaiters--;
		SetEvent(mWakeEvent);
		pthread_mutex_lock(lock);
		return ret == WAIT_TIMEOUT ? ETIMEDOUT : 0;
	}
	void notify_one()
	{
		CScriptUniqueLock<CScriptMutex> quard(mMutex);
		if (!mNumWaiters) return;
		SetEvent(mSemaphore);
		WaitForSingleObject(mWakeEvent, INFINITE);
	}
	void notify_all() {
		while (mNumWaiters) notify_one();
	}
private:
	CScriptMutex mMutex;
	std::atomic<int> mNumWaiters;
	HANDLE mSemaphore;
	HANDLE mWakeEvent;
};


#	define pthread_condattr_t struct {}
#	define pthread_condattr_init(a) do { (void*)(a); } while(0)
#	define pthread_condattr_destroy(a) do {} while(0)
#	define CLOCK_MONOTONIC 1
#	define pthread_condattr_setclock(a, c) do {} while(0)
#	define pthread_cond_t condition_variable_impl
#	define pthread_cond_init(c, a) do {} while(0)
#	define pthread_cond_destroy(c) do {} while(0)
#	define pthread_cond_wait(c, m) (c)->wait(m)
#	define pthread_cond_timedwait(c, m, t) (c)->wait_until(m, t)
#	define pthread_cond_signal(c) (c)->notify_one()
#	define pthread_cond_broadcast(c) (c)->notify_all()
#endif

class CScriptCondVar_impl : public CScriptCondVar::CScriptCondVar_t {
public:
	CScriptCondVar_impl(CScriptCondVar *_this) : This(_this) {
		pthread_condattr_t attr;
		pthread_condattr_init(&attr);
		pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
		pthread_cond_init(&cond, &attr);
		pthread_condattr_destroy(&attr);
	}
	virtual ~CScriptCondVar_impl() {
		pthread_cond_destroy(&cond);
	}
	CScriptCondVar *This;
	virtual void notify_one() override {
		pthread_cond_signal(&cond);
	}
	virtual void notify_all() override {
		pthread_cond_broadcast(&cond);
	}
	virtual void wait(CScriptUniqueLock<CScriptMutex> &lock) override {
		pthread_cond_wait(&cond, (pthread_mutex_t *)lock.mutex.getRealMutex());
	}
	virtual int wait_for(CScriptUniqueLock<CScriptMutex> &lock, std::chrono::duration<long, std::milli> rel_time) override {
		return wait_until(lock, std::chrono::steady_clock::now() + rel_time);
	}
	virtual int wait_until(CScriptUniqueLock<CScriptMutex> &lock, std::chrono::steady_clock::time_point abs_time) override {
		struct timespec ts;
		auto duration = abs_time.time_since_epoch();
		ts.tv_sec = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
		ts.tv_nsec = std::chrono::duration_cast<std::chrono::duration<long, std::nano>>(duration % std::chrono::seconds(1)).count();
		return pthread_cond_timedwait(&cond, (pthread_mutex_t *)lock.mutex.getRealMutex(), &ts) == ETIMEDOUT ? 1 : 0;
	}
	virtual int wait_until(CScriptUniqueLock<CScriptMutex> &lock, std::chrono::system_clock::time_point abs_time) override {
		return wait_for(lock, std::chrono::duration_cast<std::chrono::duration<long, std::milli>>(abs_time - std::chrono::system_clock::now()));
	}
	pthread_cond_t cond;
};

CScriptCondVar::CScriptCondVar_t::~CScriptCondVar_t() {}
CScriptCondVar::CScriptCondVar() {
	condVar = new CScriptCondVar_impl(this);
}
CScriptCondVar::~CScriptCondVar() {
	delete condVar;
}

#endif /*HAVE_CXX_THREADS*/



//////////////////////////////////////////////////////////////////////////
// Threading
//////////////////////////////////////////////////////////////////////////


#ifdef HAVE_CXX_THREADS
#	define pthread_attr_t int
#	define pthread_attr_init(a) do {} while(0)
#	define pthread_attr_destroy(a) do {} while(0)
#	define pthread_t std::thread
#	define pthread_create(t, attr, fnc, a) *(t) = std::thread(fnc, a);
#	define pthread_join(t, v) t.join();
#	define sched_yield std::this_thread::yield
#elif !defined(HAVE_PTHREAD)
// simple pthreads 4 windows
#	define pthread_attr_t SIZE_T
#	define pthread_attr_init(attr) (*attr=0)
#	define pthread_attr_destroy(a) do {} while(0)
#	define pthread_attr_setstacksize(attr, stack) *attr=stack;

#	define pthread_t HANDLE
#	define pthread_create(t, attr, fnc, a) *(t) = CreateThread(NULL, attr ? *((pthread_attr_t*)attr) : 0, (LPTHREAD_START_ROUTINE)fnc, a, 0, NULL)
#	define pthread_join(t, v) WaitForSingleObject(t, INFINITE), GetExitCodeThread(t,(LPDWORD)v), CloseHandle(t)
#	define sched_yield() Sleep(0)
#endif

class CScriptThread_impl : public CScriptThread::CScriptThread_t {
public:
	CScriptThread_impl(CScriptThread *_this) : retvar((void*)-1), activ(false), running(false), started(false), This(_this) {

	}
	virtual ~CScriptThread_impl() {

	}
	virtual int Run(uint32_t timeout_ms) override {
		if(started) return 0;
		activ = true;
//		pthread_attr_t attribute;
//		pthread_attr_init(&attribute);
//		pthread_attr_setstacksize(&attribute,1024);
		pthread_create(&thread, NULL /*&attribute*/, (void*(*)(void*))ThreadFnc, this);
//		pthread_attr_destroy(&attribute);
		auto endtime = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
		while (!started) {
			sched_yield();
			if (endtime < std::chrono::steady_clock::now()) return 1;
		}
		return 0;
	}
	virtual int Stop(bool Wait) override {
		activ = false;
		if(Wait && started) {
			pthread_join(thread, &retvar);
			return retValue();
		}
		return -1;
	}
	virtual int retValue() override { return (int)((ptrdiff_t)retvar); }
	virtual bool isActiv() override { return activ; }
	virtual bool isRunning() override { return running; }
	virtual bool isStarted() override { return started; }
	static void *ThreadFnc(CScriptThread_impl *This) {
		This->running = This->started = true;
		This->retvar = (void*)((ptrdiff_t)This->This->ThreadFnc());
		This->running = false;
		This->This->ThreadFncFinished();
		return This->retvar;
	}
	void *retvar;
	std::atomic<bool> activ;
	std::atomic<bool> running;
	std::atomic<bool> started;
	CScriptThread *This;
	pthread_t thread;
};

CScriptThread::CScriptThread() {
	thread = new CScriptThread_impl(this);
}
CScriptThread::~CScriptThread() {
	delete thread;
}
void CScriptThread::ThreadFncFinished() {}

void CScriptThread::yield() {
	sched_yield();
}

CScriptCoroutine::StopIteration_t CScriptCoroutine::StopIteration;

bool CScriptCoroutine::next()
{
	CScriptUniqueLock<CScriptMutex> lock(mutex);
	if(!isStarted()) {
		Run();
		if(mainNotifies-- == 0) mainCondVar.wait(lock);
	} else if(isRunning()) {
		++coroutineNotifies;
		coroutineCondVar.notify_one();
		if(mainNotifies-- == 0) mainCondVar.wait(lock);
	} else
		return false;
	if(!isRunning()) return false;
	return true;
}
bool CScriptCoroutine::yield_no_throw() {
	CScriptUniqueLock<CScriptMutex> lock(mutex);
	++mainNotifies;
	mainCondVar.notify_one();
	if(coroutineNotifies-- == 0) coroutineCondVar.wait(lock);
	return isActiv();
}

int CScriptCoroutine::ThreadFnc() {
	int ret=-1;
	try {
		ret = Coroutine();
	} catch(StopIteration_t &) {
		return 0;
	} catch(std::exception & e) {
		printf("CScriptCoroutine has received an uncaught exception: %s\n", e.what());
		return -1;
	} catch(...) {
		printf("CScriptCoroutine has received an uncaught and unknown exception\n");
		return -1;
	}
	return ret;
}
void CScriptCoroutine::ThreadFncFinished() {
	++mainNotifies;
	mainCondVar.notify_one();
}


#endif // HAVE_THREADING

