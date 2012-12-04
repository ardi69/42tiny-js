
#include "TinyJS_Threading.h"

#define HAVE_PTHREAD

#ifdef HAVE_PTHREAD
#include <pthread.h>
#elif defined(WIN32) /* !Pthread but Windows */
#include <windows.h>
#endif



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
	void Lock() {
#ifdef HAVE_PTHREAD
		pthread_mutex_lock(&mutex);
#else
		WaitForSingleObject(mutex, INFINITE);
#endif
	}
	void UnLock() {
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
