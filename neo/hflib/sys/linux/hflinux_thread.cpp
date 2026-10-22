#include "../hfprecompiled.h"

#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <sched.h>
#include <sys/prctl.h>

typedef struct {
	xthread_t		function;
	void *			parms;
	const char *	name;
	core_t			core;
} threadInfo_t;

// set the thread name for debugging purposes
void Sys_SetCurrentThreadName( const char *name ) {
#if defined(__linux__)
	prctl( PR_SET_NAME, name, 0, 0, 0 );
#endif
}

static void * ThreadProc( void *parms ) {
	threadInfo_t * info = (threadInfo_t *)parms;

	Sys_SetCurrentThreadName( info->name );

	info->function( info->parms );

	return NULL;
}

/*
================
Sys_GetCurrentThreadID
================
*/
uintptr_t Sys_GetCurrentThreadID() {
	return (uintptr_t)pthread_self();
}

/*
================
Sys_CreateThread
================
*/
uintptr_t Sys_CreateThread( xthread_t function, void *parms, xthreadPriority priority, 
							const char *name, core_t core, int stackSize, bool suspended ) {
	pthread_attr_t attr;
	pthread_t thread;
	threadInfo_t * info;

	info = (threadInfo_t *)Mem_Alloc( sizeof( *info ), TAG_THREAD );
	info->function = function;
	info->parms = parms;
	info->name = name;
	info->core = core;

	pthread_attr_init( &attr );
	if ( stackSize > 0 ) {
		pthread_attr_setstacksize( &attr, stackSize );
	}

	if ( pthread_create( &thread, &attr, ThreadProc, info ) != 0 ) {
		Mem_Free( info );
		common->FatalError( "Sys_CreateThread: failed to create thread" );
	}

	pthread_attr_destroy( &attr );

	return (uintptr_t)thread;
}

/*
================
Sys_WaitForThread
================
*/
void Sys_WaitForThread( uintptr_t threadHandle ) {
	pthread_join( (pthread_t)threadHandle, NULL );
}

/*
================
Sys_DestroyThread
================
*/
void Sys_DestroyThread( uintptr_t threadHandle ) {
	pthread_detach( (pthread_t)threadHandle );
}

/*
================
Sys_SignalCreate
================
A Win32 event: a state bit guarded by a mutex and a condition variable.
A condition variable alone cannot be cleared, cannot be polled, and has no
memory of being signalled, which is why the state lives here instead.
================
*/
void Sys_SignalCreate( signalHandle_t & handle, bool manualReset ) {
	pthread_mutex_init( &handle.mutex, NULL );
	pthread_cond_init( &handle.cond, NULL );
	handle.signaled = false;
	handle.manualReset = manualReset;
}

/*
================
Sys_SignalDestroy
================
*/
void Sys_SignalDestroy( signalHandle_t & handle ) {
	pthread_cond_destroy( &handle.cond );
	pthread_mutex_destroy( &handle.mutex );
}

/*
================
Sys_SignalRaise
================
*/
void Sys_SignalRaise( signalHandle_t & handle ) {
	pthread_mutex_lock( &handle.mutex );
	handle.signaled = true;
	pthread_cond_broadcast( &handle.cond );
	pthread_mutex_unlock( &handle.mutex );
}

/*
================
Sys_SignalClear
================
*/
void Sys_SignalClear( signalHandle_t & handle ) {
	pthread_mutex_lock( &handle.mutex );
	handle.signaled = false;
	pthread_mutex_unlock( &handle.mutex );
}

/*
================
Sys_SignalWait
================
timeout: -1 wait forever, 0 poll, otherwise milliseconds.
Returns true when the wait was satisfied. A successful wait consumes the
signal for an auto-reset handle, which is what the thread handshake in
hfThread.cpp is written against: signalWorkerDone is manual-reset (a stop
stays observed by every later waiter) and signalMoreWorkToDo is auto-reset
(one wake per piece of work).
================
*/
bool Sys_SignalWait( signalHandle_t & handle, int timeout ) {
	pthread_mutex_lock( &handle.mutex );

	if ( timeout > 0 ) {
		struct timespec deadline;
		clock_gettime( CLOCK_REALTIME, &deadline );
		deadline.tv_sec += timeout / 1000;
		deadline.tv_nsec += (long)( timeout % 1000 ) * 1000000L;
		if ( deadline.tv_nsec >= 1000000000L ) {
			deadline.tv_sec += 1;
			deadline.tv_nsec -= 1000000000L;
		}
		while ( !handle.signaled ) {
			if ( pthread_cond_timedwait( &handle.cond, &handle.mutex, &deadline ) != 0 ) {
				break;
			}
		}
	} else if ( timeout < 0 ) {
		while ( !handle.signaled ) {
			pthread_cond_wait( &handle.cond, &handle.mutex );
		}
	}
	// timeout == 0 polls the state without ever blocking

	bool result = handle.signaled;
	if ( result && !handle.manualReset ) {
		handle.signaled = false;
	}
	pthread_mutex_unlock( &handle.mutex );
	return result;
}

/*
================
Sys_MutexCreate
================
*/
void Sys_MutexCreate( mutexHandle_t & handle ) {
	pthread_mutex_init( &handle, NULL );
}

/*
================
Sys_MutexDestroy
================
*/
void Sys_MutexDestroy( mutexHandle_t & handle ) {
	pthread_mutex_destroy( &handle );
}

/*
================
Sys_MutexLock
================
*/
bool Sys_MutexLock( mutexHandle_t & handle, bool blocking ) {
	if ( blocking ) {
		return pthread_mutex_lock( &handle ) == 0;
	}
	return pthread_mutex_trylock( &handle ) == 0;
}

/*
================
Sys_MutexUnlock
================
*/
void Sys_MutexUnlock( mutexHandle_t & handle ) {
	pthread_mutex_unlock( &handle );
}

/*
================
Sys_InterlockedIncrement
================
*/
interlockedInt_t Sys_InterlockedIncrement( interlockedInt_t & value ) {
	return __sync_add_and_fetch( &value, 1 );
}

/*
================
Sys_InterlockedDecrement
================
*/
interlockedInt_t Sys_InterlockedDecrement( interlockedInt_t & value ) {
	return __sync_sub_and_fetch( &value, 1 );
}

/*
================
Sys_InterlockedAdd
================
*/
interlockedInt_t Sys_InterlockedAdd( interlockedInt_t & value, interlockedInt_t i ) {
	return __sync_add_and_fetch( &value, i );
}

/*
================
Sys_InterlockedSub
================
*/
interlockedInt_t Sys_InterlockedSub( interlockedInt_t & value, interlockedInt_t i ) {
	return __sync_sub_and_fetch( &value, i );
}

/*
================
Sys_InterlockedExchange
================
*/
interlockedInt_t Sys_InterlockedExchange( interlockedInt_t & value, interlockedInt_t exchange ) {
	return __sync_lock_test_and_set( &value, exchange );
}

/*
================
Sys_InterlockedCompareExchange
================
*/
interlockedInt_t Sys_InterlockedCompareExchange( interlockedInt_t & value, interlockedInt_t comparand, interlockedInt_t exchange ) {
	return __sync_val_compare_and_swap( &value, comparand, exchange );
}

/*
================
Sys_InterlockedExchangePointer
================
*/
void * Sys_InterlockedExchangePointer( void * & ptr, void * exchange ) {
	return __sync_lock_test_and_set( &ptr, exchange );
}

/*
================
Sys_InterlockedCompareExchangePointer
================
*/
void * Sys_InterlockedCompareExchangePointer( void * & ptr, void * comparand, void * exchange ) {
	return __sync_val_compare_and_swap( &ptr, comparand, exchange );
}

/*
================
Sys_Yield
================
*/
void Sys_Yield() {
	sched_yield();
}

