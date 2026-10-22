#pragma hdrstop
#include "hfprecompiled.h"

/*
================================================================================================
Contains the vartious ThreadingClass implementations.
================================================================================================
*/

/*
================================================================================================

	hfSysThread

================================================================================================
*/

/*
========================
hfSysThread::hfSysThread
========================
*/
hfSysThread::hfSysThread() :
		threadHandle( 0 ),
		isWorker( false ),
		isRunning( false ),
		isTerminating( false ),
		moreWorkToDo( false ),
		signalWorkerDone( true ) {
}

/*
========================
hfSysThread::~hfSysThread
========================
*/
hfSysThread::~hfSysThread() {
	StopThread( true );
	if ( threadHandle ) {
		Sys_DestroyThread( threadHandle );
	}
}

/*
========================
hfSysThread::StartThread
========================
*/
bool hfSysThread::StartThread( const char * name_, core_t core, xthreadPriority priority, int stackSize ) {
	if ( isRunning ) {
		return false;
	}

	name = name_;

	isTerminating = false;

	if ( threadHandle ) {
		Sys_DestroyThread( threadHandle );
	}

	threadHandle = Sys_CreateThread( (xthread_t)ThreadProc, this, priority, name, core, stackSize, false );

	isRunning = true;
	return true;
}

/*
========================
hfSysThread::StartWorkerThread
========================
*/
bool hfSysThread::StartWorkerThread( const char * name_, core_t core, xthreadPriority priority, int stackSize ) {
	if ( isRunning ) {
		return false;
	}

	isWorker = true;

	bool result = StartThread( name_, core, priority, stackSize );

	signalWorkerDone.Wait( hfSysSignal::WAIT_INFINITE );

	return result;
}

/*
========================
hfSysThread::StopThread
========================
*/
void hfSysThread::StopThread( bool wait ) {
	if ( isWorker ) {
		// The isRunning check has to be inside the lock: the worker clears
		// isRunning and raises signalWorkerDone as one locked step on its way
		// out, so if this thread reads isRunning first it also gets to clear
		// the signal first and the worker's final raise lands afterwards.
		// Reading it outside the lock lets a raise slip between the check and
		// the Clear(), which then waits on a signal nobody will ever send.
		signalMutex.Lock();
		if ( !isRunning ) {
			signalMutex.Unlock();
			return;
		}
		moreWorkToDo = true;
		signalWorkerDone.Clear();
		isTerminating = true;
		signalMoreWorkToDo.Raise();
		signalMutex.Unlock();
	} else {
		if ( !isRunning ) {
			return;
		}
		isTerminating = true;
	}
	if ( wait ) {
		WaitForThread();
	}
}

/*
========================
hfSysThread::WaitForThread
========================
*/
void hfSysThread::WaitForThread() {
	if ( isWorker ) {
		signalWorkerDone.Wait( hfSysSignal::WAIT_INFINITE );
	} else if ( isRunning ) {
		Sys_DestroyThread( threadHandle );
		threadHandle = 0;
	}
}

/*
========================
hfSysThread::SignalWork
========================
*/
void hfSysThread::SignalWork() {
	if ( isWorker ) {
		signalMutex.Lock();
		moreWorkToDo = true;
		signalWorkerDone.Clear();
		signalMoreWorkToDo.Raise();
		signalMutex.Unlock();
	}
}

/*
========================
hfSysThread::IsWorkDone
========================
*/
bool hfSysThread::IsWorkDone() {
	if ( isWorker ) {
		// a timeout of 0 will return immediately with true if signaled
		if ( signalWorkerDone.Wait( 0 ) ) {
			return true;
		}
	}
	return false;
}

/*
========================
hfSysThread::ThreadProc
========================
*/
int hfSysThread::ThreadProc( hfSysThread * thread ) {
	int retVal = 0;

	try {
		if ( thread->isWorker ) {
			for( ; ; ) {
				thread->signalMutex.Lock();
				if ( thread->moreWorkToDo ) {
					thread->moreWorkToDo = false;
					thread->signalMoreWorkToDo.Clear();
					thread->signalMutex.Unlock();
				} else {
					thread->signalWorkerDone.Raise();
					thread->signalMutex.Unlock();
					thread->signalMoreWorkToDo.Wait( hfSysSignal::WAIT_INFINITE );
					continue;
				}

				if ( thread->isTerminating ) {
					break;
				}

				retVal = thread->Run();
			}
		} else {
			retVal = thread->Run();
		}
	} catch ( hfException & ex ) {
		hfLib::Warning( "Fatal error in thread %s: %s", thread->GetName(), ex.GetError() );
		// We don't handle threads terminating unexpectedly very well, so just terminate the whole process
		_exit( 0 );
	}

	if ( thread->isWorker ) {
		// publishing "stopped" and clearing isRunning under the same lock
		// makes the exit observable exactly once, in one order, to StopThread
		thread->signalMutex.Lock();
		thread->isRunning = false;
		thread->signalWorkerDone.Raise();
		thread->signalMutex.Unlock();
	} else {
		thread->isRunning = false;
	}

	return retVal;
}

/*
========================
hfSysThread::Run
========================
*/
int hfSysThread::Run() {
	// The Run() is not pure virtual because on destruction of a derived class
	// the virtual function pointer will be set to NULL before the hfSysThread
	// destructor actually stops the thread.
	return 0;
}

/*
================================================================================================

	test

================================================================================================
*/

/*
================================================
hfMyThread test class.
================================================
*/
class hfMyThread : public hfSysThread {
public:
	virtual int Run() {
		// run threaded code here
		return 0;
	}
	// specify thread data here
};

/*
========================
TestThread
========================
*/
void TestThread() {
	hfMyThread thread;
	thread.StartThread( "myThread", CORE_ANY );
}

/*
========================
TestWorkers
========================
*/
void TestWorkers() {
	hfSysWorkerThreadGroup<hfMyThread> workers( "myWorkers", 4 );
	for ( ; ; ) {
		for ( int i = 0; i < workers.GetNumThreads(); i++ ) {
			// workers.GetThread( i )-> // setup work for this thread
		}
		workers.SignalWorkAndWait();
	}
}

