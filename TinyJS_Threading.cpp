#include "TinyJS_Threading.h"

#undef HAVE_THREADING
#if !defined(NO_THREADING) && !defined(HAVE_CUSTOUM_THREADING_IMPL)
#	define HAVE_THREADING
#	if defined(WIN32) && !defined(HAVE_PTHREAD)
#		include <windows.h>
#	else
#		include <pthread.h>
#	endif
#endif

#ifdef HAVE_THREADING 

class CScriptMutex_impl : public CScriptMutex::CScriptMutex_t {
public:
	CScriptMutex_impl() {
#ifdef HAVE_PTHREAD
		pthread_mutex_init(&mutex, NULL);
#else
		mutex = CreateMutex(NULL, false, NULL);
#endif
	}
	~CScriptMutex_impl() {
#ifdef HAVE_PTHREAD
		pthread_mutex_destroy(&mutex);
#else
		CloseHandle(mutex);
#endif
	}
	void lock() {
#ifdef HAVE_PTHREAD
		pthread_mutex_lock(&mutex);
#else
		WaitForSingleObject(mutex, INFINITE);
#endif
	}
	void unlock() {
#ifdef HAVE_PTHREAD
		pthread_mutex_unlock(&mutex);
#else
		ReleaseMutex(mutex);
#endif
	}
#ifdef HAVE_PTHREAD
	pthread_mutex_t mutex;
#else
	HANDLE mutex;
#endif
};

CScriptMutex::CScriptMutex() {
	mutex = new CScriptMutex_impl;
}

CScriptMutex::~CScriptMutex() {
	delete mutex;
}

class CScriptThread_impl : public CScriptThread::CScriptThread_t {
public:
	CScriptThread_impl(CScriptThread *_this) : running(false), This(_this) {}
	~CScriptThread_impl() {}
	void Run() {
#ifdef HAVE_PTHREAD
		typedef void *(__cdecl *ThreadFnc_t) (void *);
		pthread_create(&thread, NULL, (ThreadFnc_t)ThreadFnc, This);
#else
		HANDLE thread;
#endif

	}
	static int ThreadFnc(CScriptThread *This) {
		return This->ThreadFnc(This);
	}
	bool running;
	CScriptThread *This;
#ifdef HAVE_PTHREAD
	pthread_t thread;
#else
	HANDLE thread;
#endif
};



CScriptThread::CScriptThread() {
	thread = new CScriptThread_impl(this);
}
CScriptThread::~CScriptThread() {
	delete thread;
}
#endif // HAVE_THREADING