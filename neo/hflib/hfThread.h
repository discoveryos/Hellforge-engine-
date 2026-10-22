#ifndef __THREAD_H__
#define __THREAD_H__

/*
================================================
hfSysMutex provides a C++ wrapper to the low level system mutex functions.  A mutex is an
object that can only be locked by one thread at a time.  It's used to prevent two threads
from accessing the same piece of data simultaneously.
================================================
*/
class hfSysMutex {
public:
					hfSysMutex() { Sys_MutexCreate( handle ); }
					~hfSysMutex() { Sys_MutexDestroy( handle ); }

	bool			Lock( bool blocking = true ) { return Sys_MutexLock( handle, blocking ); }
	void			Unlock() { Sys_MutexUnlock( handle ); }

private:
	mutexHandle_t	handle;

					hfSysMutex( const hfSysMutex & s ) {}
	void			operator=( const hfSysMutex & s ) {}
};

/*
================================================
hfScopedCriticalSection is a helper class that automagically locks a mutex when it's created
and unlocks it when it goes out of scope.
================================================
*/
class hfScopedCriticalSection {
public:
	hfScopedCriticalSection( hfSysMutex & m ) : mutex(&m) { mutex->Lock(); }
	~hfScopedCriticalSection() { mutex->Unlock(); }

private:
	hfSysMutex *	mutex;	// NOTE: making this a reference causes a TypeInfo crash
};

/*
================================================
hfSysSignal is a C++ wrapper for the low level system signal functions.  A signal is an object
that a thread can wait on for it to be raised.  It's used to indicate data is available or that
a thread has reached a specific point.
================================================
*/
class hfSysSignal {
public:
	static const int	WAIT_INFINITE = -1;

			hfSysSignal( bool manualReset = false )	{ Sys_SignalCreate( handle, manualReset ); }
			~hfSysSignal()	{ Sys_SignalDestroy( handle ); }

	void	Raise() { Sys_SignalRaise( handle ); }
	void	Clear() { Sys_SignalClear( handle ); }

	// Wait returns true if the object is in a signalled state and
	// returns false if the wait timed out. Wait also clears the signalled
	// state when the signalled state is reached within the time out period.
	bool	Wait( int timeout = WAIT_INFINITE ) { return Sys_SignalWait( handle, timeout ); }

private:
	signalHandle_t		handle;

						hfSysSignal( const hfSysSignal & s ) {}
	void				operator=( const hfSysSignal & s ) {}
};

/*
================================================
hfSysInterlockedInteger is a C++ wrapper for the low level system interlocked integer
routines to atomically increment or decrement an integer.
================================================
*/
class hfSysInterlockedInteger {
public:
						hfSysInterlockedInteger() : value( 0 ) {}

	// atomically increments the integer and returns the new value
	int					Increment() { return Sys_InterlockedIncrement( value ); }

	// atomically decrements the integer and returns the new value
	int					Decrement() { return Sys_InterlockedDecrement( value ); }

	// atomically adds a value to the integer and returns the new value
	int					Add( int v ) { return Sys_InterlockedAdd( value, (interlockedInt_t) v ); }

	// atomically subtracts a value from the integer and returns the new value
	int					Sub( int v ) { return Sys_InterlockedSub( value, (interlockedInt_t) v ); }

	// returns the current value of the integer
	int					GetValue() const { return value; }

	// sets a new value, Note: this operation is not atomic
	void				SetValue( int v ) { value = (interlockedInt_t)v; }

private:
	interlockedInt_t	value;
};

/*
================================================
hfSysInterlockedPointer is a C++ wrapper around the low level system interlocked pointer
routine to atomically set a pointer while retrieving the previous value of the pointer.
================================================
*/
template< typename T >
class hfSysInterlockedPointer {
public:
			hfSysInterlockedPointer() : ptr( NULL ) {}

	// atomically sets the pointer and returns the previous pointer value
	T *		Set( T * newPtr ) { 
				return (T *) Sys_InterlockedExchangePointer( (void * &) ptr, newPtr ); 
			}

	// atomically sets the pointer to 'newPtr' only if the previous pointer is equal to 'comparePtr'
	// ptr = ( ptr == comparePtr ) ? newPtr : ptr
	T *		CompareExchange( T * comparePtr, T * newPtr ) {
				return (T *) Sys_InterlockedCompareExchangePointer( (void * &) ptr, comparePtr, newPtr );
	}

	// returns the current value of the pointer
	T *		Get() const { return ptr; }

private:
	T *		ptr;
};

/*
================================================
hfSysThread is an abstract base class, to be extended by classes implementing the
hfSysThread::Run() method. 

	class hfMyThread : public hfSysThread {
	public:
		virtual int Run() {
			// run thread code here
			return 0;
		}
		// specify thread data here
	};

	hfMyThread thread;
	thread.Start( "myThread" );

A worker thread is a thread that waits in place (without consuming CPU)
until work is available. A worker thread is implemented as normal, except that, instead of
calling the Start() method, the StartWorker() method is called to start the thread.
Note that the Sys_CreateThread function does not support the concept of worker threads.

	class hfMyWorkerThread : public hfSysThread {
	public:
		virtual int Run() {
			// run thread code here
			return 0;
		}
		// specify thread data here
	};

	hfMyWorkerThread thread;
	thread.StartThread( "myWorkerThread" );
 
	// main thread loop
	for ( ; ; ) {
		// setup work for the thread here (by modifying class data on the thread)
		thread.SignalWork();           // kick in the worker thread
		// run other code in the main thread here (in parallel with the worker thread)
		thread.WaitForThread();        // wait for the worker thread to finish
		// use results from worker thread here
	}

In the above example, the thread does not continuously run in parallel with the main Thread,
but only for a certain period of time in a very controlled manner. Work is set up for the
Thread and then the thread is signalled to process that work while the main thread continues.
After doing other work, the main thread can wait for the worker thread to finish, if it has not
finished already. When the worker thread is done, the main thread can safely use the results
from the worker thread.

Note that worker threads are useful on all platforms but they do not map to the SPUs on the PS3.
================================================
*/
class hfSysThread {
public:
					hfSysThread();
	virtual			~hfSysThread();

	const char *	GetName() const { return name.c_str(); }
	uintptr_t		GetThreadHandle() const { return threadHandle; }
	bool			IsRunning() const { return isRunning; }
	bool			IsTerminating() const { return isTerminating; }

	//------------------------
	// Thread Start/Stop/Wait
	//------------------------

	bool			StartThread( const char * name, core_t core, 
								 xthreadPriority priority = THREAD_NORMAL,
								 int stackSize = DEFAULT_THREAD_STACK_SIZE );

	bool			StartWorkerThread( const char * name, core_t core, 
									   xthreadPriority priority = THREAD_NORMAL,
									   int stackSize = DEFAULT_THREAD_STACK_SIZE );

	void			StopThread( bool wait = true );

	// This can be called from multiple other threads. However, in the case
	// of a worker thread, the work being "done" has little meaning if other
	// threads are continuously signalling more work.
	void			WaitForThread();

	//------------------------
	// Worker Thread
	//------------------------

	// Signals the thread to notify work is available.
	// This can be called from multiple other threads.
	void			SignalWork();

	// Returns true if the work is done without waiting.
	// This can be called from multiple other threads. However, the work
	// being "done" has little meaning if other threads are continuously
	// signalling more work.
	bool			IsWorkDone();

protected:
	// The routine that performs the work.
	virtual int		Run();

private:
	hfStr			name;
	uintptr_t		threadHandle;
	bool			isWorker;
	bool			isRunning;
	volatile bool	isTerminating;
	volatile bool	moreWorkToDo;
	hfSysSignal		signalWorkerDone;
	hfSysSignal		signalMoreWorkToDo;
	hfSysMutex		signalMutex;

	static int		ThreadProc( hfSysThread * thread );

					hfSysThread( const hfSysThread & s ) {}
	void			operator=( const hfSysThread & s ) {}
};

/*
================================================
hfSysWorkerThreadGroup implements a group of worker threads that 
typically crunch through a collection of similar tasks.

	class hfMyWorkerThread : public hfSysThread {
	public:
		virtual int Run() {
			// run thread code here
			return 0;
		}
		// specify thread data here
	};

	hfSysWorkerThreadGroup<hfMyWorkerThread> workers( "myWorkers", 4 );
	for ( ; ; ) {
		for ( int i = 0; i < workers.GetNumThreads(); i++ ) {
			// workers.GetThread( i )-> // setup work for this thread
		}
		workers.SignalWorkAndWait();
		// use results from the worker threads here
	}

The concept of worker thread Groups is probably most useful for tools and compilers.
For instance, the AAS Compiler is using a worker thread group. Although worker threads
will work well on the PC, Mac and the 360, they do not directly map to the PS3,
in that the worker threads won't automatically run on the SPUs.
================================================
*/
template<class threadType>
class hfSysWorkerThreadGroup {
public:
					hfSysWorkerThreadGroup( const char * name, int numThreads,
											xthreadPriority priority = THREAD_NORMAL,
											int stackSize = DEFAULT_THREAD_STACK_SIZE );

	virtual			~hfSysWorkerThreadGroup();

	int				GetNumThreads() const { return threadList.Num(); }
	threadType &	GetThread( int i ) { return *threadList[i]; }

	void			SignalWorkAndWait();

private:
	hfList<threadType *, TAG_THREAD>	threadList;
	bool					runOneThreadInline;	// use the signalling thread as one of the threads
	bool					singleThreaded;		// set to true for debugging
};

/*
========================
hfSysWorkerThreadGroup<threadType>::hfSysWorkerThreadGroup
========================
*/
template<class threadType>
HF_INLINE hfSysWorkerThreadGroup<threadType>::hfSysWorkerThreadGroup( const char * name, 
			int numThreads, xthreadPriority priority, int stackSize ) {
	runOneThreadInline = ( numThreads < 0 );
	singleThreaded = false;
	numThreads = abs( numThreads );
	for( int i = 0; i < numThreads; i++ ) {
		threadType *thread = new (TAG_THREAD) threadType;
		thread->StartWorkerThread( va( "%s_worker%i", name, i ), (core_t) i, priority, stackSize );
		threadList.Append( thread );
	}
}

/*
========================
hfSysWorkerThreadGroup<threadType>::~hfSysWorkerThreadGroup
========================
*/
template<class threadType>
HF_INLINE hfSysWorkerThreadGroup<threadType>::~hfSysWorkerThreadGroup() {
	threadList.DeleteContents();
}

/*
========================
hfSysWorkerThreadGroup<threadType>::SignalWorkAndWait
========================
*/
template<class threadType>
HF_INLINE void hfSysWorkerThreadGroup<threadType>::SignalWorkAndWait() {
	if ( singleThreaded ) {
		for( int i = 0; i < threadList.Num(); i++ ) {
			threadList[ i ]->Run();
		}
		return;
	}
	for( int i = 0; i < threadList.Num() - runOneThreadInline; i++ ) {
		threadList[ i ]->SignalWork();
	}
	if ( runOneThreadInline ) {
		threadList[ threadList.Num() - 1 ]->Run();
	}
	for ( int i = 0; i < threadList.Num() - runOneThreadInline; i++ ) {
		threadList[ i ]->WaitForThread();
	}
}

/*
================================================
hfSysThreadSynchronizer, allows a group of threads to 
synchronize with each other half-way through execution.

	hfSysThreadSynchronizer sync;

	class hfMyWorkerThread : public hfSysThread {
	public:
		virtual int Run() {
			// perform first part of the work here
			sync.Synchronize( threadNum );	// synchronize all threads
			// perform second part of the work here
			return 0;
		}
		// specify thread data here
		unsigned int threadNum;
	};

	hfSysWorkerThreadGroup<hfMyWorkerThread> workers( "myWorkers", 4 );
	for ( int i = 0; i < workers.GetNumThreads(); i++ ) {
		workers.GetThread( i )->threadNum = i;
	}

	for ( ; ; ) {
		for ( int i = 0; i < workers.GetNumThreads(); i++ ) {
			// workers.GetThread( i )-> // setup work for this thread
		}
		workers.SignalWorkAndWait();
		// use results from the worker threads here
	}

================================================
*/
class hfSysThreadSynchronizer {
public:
	static const int	WAIT_INFINITE = -1;

	HF_INLINE	void			SetNumThreads( unsigned int num );
	HF_INLINE	void			Signal( unsigned int threadNum );
	HF_INLINE	bool			Synchronize( unsigned int threadNum, int timeout = WAIT_INFINITE );

private:
	hfList< hfSysSignal *, TAG_THREAD >		signals;
	hfSysInterlockedInteger		busyCount;
};

/*
========================
hfSysThreadSynchronizer::SetNumThreads
========================
*/
HF_INLINE void hfSysThreadSynchronizer::SetNumThreads( unsigned int num ) {
	assert( busyCount.GetValue() == signals.Num() );
	if ( (int)num != signals.Num() ) {
		signals.DeleteContents();
		signals.SetNum( (int)num );
		for ( unsigned int i = 0; i < num; i++ ) {
			signals[i] = new (TAG_THREAD) hfSysSignal();
		}
		busyCount.SetValue( num );
		SYS_MEMORYBARRIER;
	}
}

/*
========================
hfSysThreadSynchronizer::Signal
========================
*/
HF_INLINE void hfSysThreadSynchronizer::Signal( unsigned int threadNum ) {
	if ( busyCount.Decrement() == 0 ) {
		busyCount.SetValue( (unsigned int) signals.Num() );
		SYS_MEMORYBARRIER;
		for ( int i = 0; i < signals.Num(); i++ ) {
			signals[i]->Raise();
		}
	}
}

/*
========================
hfSysThreadSynchronizer::Synchronize
========================
*/
HF_INLINE bool hfSysThreadSynchronizer::Synchronize( unsigned int threadNum, int timeout ) {
	return signals[threadNum]->Wait( timeout );
}

#endif // !__THREAD_H__

