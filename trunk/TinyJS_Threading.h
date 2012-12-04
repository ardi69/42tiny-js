#ifndef TinyJS_Threading_h__
#define TinyJS_Threading_h__

/*
 * 42TinyJS
 *
 * A fork of TinyJS with the goal to makes a more JavaScript/ECMA compliant engine
 *
 * Authored By Armin Diedering <armin@diedering.de>
 *
 * Copyright (C) 2010-2012 ardisoft
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
class CScriptMutex {
public:
	CScriptMutex();
	~CScriptMutex();
	void Lock() { mutex->Lock(); }
	void UnLock() { mutex->UnLock(); }
	class CScriptMutex_t{
	public:
//		virtual ~CScriptMutex_t()=0;
		virtual void Lock()=0;
		virtual void UnLock()=0;
	};
private:
	CScriptMutex_t *mutex;

};

class CScriptThread {
public:
	CScriptThread();
	virtual ~CScriptThread();
	void Run() { thread->Run(); }
	virtual int ThreadFnc(CScriptThread *This)=0;

	class CScriptThread_t{
	public:
		virtual void Run()=0;
	};
private:
	CScriptThread_t *thread;
};


#endif // TinyJS_Threading_h__
