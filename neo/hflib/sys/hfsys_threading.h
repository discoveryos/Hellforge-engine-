#ifndef __SYS_THREADING_H__
#define __SYS_THREADING_H__

#ifndef __TYPEINFOGEN__

/*
================================================================================================

	Platform specific mutex, signal, atomic integer, memory barrier, and thread local storage.

================================================================================================
*/

#if defined(_WIN32)

	typedef CRITICAL_SECTION		mutexHandle_t;
	typedef HANDLE					signalHandle_t;
	typedef LONG					interlockedInt_t;

	// _ReadWriteBarrier() does not translate to any instructions but keeps the compiler
	// from reordering read and write instructions across the barrier.
	// MemoryBarrier() inserts and CPU instruction that keeps the CPU from reordering reads and writes.
	#pragma intrinsic(_ReadWriteBarrier)
	#define SYS_MEMORYBARRIER		_ReadWriteBarrier(); MemoryBarrier()

	class hfSysThreadLocalStorage {
	public:
		hfSysThreadLocalStorage() { 
			tlsIndex = TlsAlloc();
		}
		hfSysThreadLocalStorage( const ptrdiff_t &val ) {
			tlsIndex = TlsAlloc();
			TlsSetValue( tlsIndex, (LPVOID)val );
		}
		~hfSysThreadLocalStorage() {
			TlsFree( tlsIndex );
		}
		operator ptrdiff_t() {
			return (ptrdiff_t)TlsGetValue( tlsIndex );
		}
		const ptrdiff_t & operator = ( const ptrdiff_t &val ) {
			TlsSetValue( tlsIndex, (LPVOID)val );
			return val;
		}	
		DWORD	tlsIndex;
	};

#define HF_TLS hfSysThreadLocalStorage

#elif defined(HF_LINUX)

	#include <pthread.h>
	#include <stdint.h>

	typedef pthread_mutex_t			mutexHandle_t;

	// a condition variable alone cannot represent an event: it has no state to
	// clear, nothing to poll, and forgets every broadcast that arrives with no
	// waiter. this is a Win32-shaped event (state + cond + mutex).
	struct hfLinuxSignal {
		pthread_cond_t	cond;
		pthread_mutex_t	mutex;
		bool			signaled;
		bool			manualReset;
	};
	typedef hfLinuxSignal			signalHandle_t;
	typedef int32_t					interlockedInt_t;

	#define SYS_MEMORYBARRIER		__sync_synchronize()
	#define _ReadWriteBarrier()		__asm__ __volatile__("" ::: "memory")

	class hfSysThreadLocalStorage {
	public:
		hfSysThreadLocalStorage() { 
			pthread_key_create( &key, NULL );
		}
		hfSysThreadLocalStorage( const ptrdiff_t &val ) {
			pthread_key_create( &key, NULL );
			pthread_setspecific( key, (void*)val );
		}
		~hfSysThreadLocalStorage() {
			pthread_key_delete( key );
		}
		operator ptrdiff_t() {
			return (ptrdiff_t)pthread_getspecific( key );
		}
		const ptrdiff_t & operator = ( const ptrdiff_t &val ) {
			pthread_setspecific( key, (void*)val );
			return val;
		}	
		pthread_key_t	key;
	};

#define HF_TLS hfSysThreadLocalStorage

#endif /* _WIN32 / HF_LINUX */

#endif // __TYPEINFOGEN__

/*
================================================================================================

	Platform independent threading functions.

================================================================================================
*/

enum core_t {
	CORE_ANY = -1,
	CORE_0A,
	CORE_0B,
	CORE_1A,
	CORE_1B,
	CORE_2A,
	CORE_2B
};

typedef unsigned int (*xthread_t)( void * );

enum xthreadPriority {
	THREAD_LOWEST,
	THREAD_BELOW_NORMAL,
	THREAD_NORMAL,
	THREAD_ABOVE_NORMAL,
	THREAD_HIGHEST
};

#define DEFAULT_THREAD_STACK_SIZE		( 256 * 1024 )

// on win32, the threadID is NOT the same as the threadHandle
uintptr_t			Sys_GetCurrentThreadID();

// returns a threadHandle
uintptr_t			Sys_CreateThread( xthread_t function, void *parms, xthreadPriority priority, 
									  const char *name, core_t core, int stackSize = DEFAULT_THREAD_STACK_SIZE, 
									  bool suspended = false );

void				Sys_WaitForThread( uintptr_t threadHandle );
void				Sys_DestroyThread( uintptr_t threadHandle );
void				Sys_SetCurrentThreadName( const char *name );

void				Sys_SignalCreate( signalHandle_t & handle, bool manualReset );
void				Sys_SignalDestroy( signalHandle_t & handle );
void				Sys_SignalRaise( signalHandle_t & handle );
void				Sys_SignalClear( signalHandle_t & handle );
bool				Sys_SignalWait( signalHandle_t & handle, int timeout );

void				Sys_MutexCreate( mutexHandle_t & handle );
void				Sys_MutexDestroy( mutexHandle_t & handle );
bool				Sys_MutexLock( mutexHandle_t & handle, bool blocking );
void				Sys_MutexUnlock( mutexHandle_t & handle );

interlockedInt_t	Sys_InterlockedIncrement( interlockedInt_t & value );
interlockedInt_t	Sys_InterlockedDecrement( interlockedInt_t & value );

interlockedInt_t	Sys_InterlockedAdd( interlockedInt_t & value, interlockedInt_t i );
interlockedInt_t	Sys_InterlockedSub( interlockedInt_t & value, interlockedInt_t i );

interlockedInt_t	Sys_InterlockedExchange( interlockedInt_t & value, interlockedInt_t exchange );
interlockedInt_t	Sys_InterlockedCompareExchange( interlockedInt_t & value, interlockedInt_t comparand, interlockedInt_t exchange );

void *				Sys_InterlockedExchangePointer( void * & ptr, void * exchange );
void *				Sys_InterlockedCompareExchangePointer( void * & ptr, void * comparand, void * exchange );

void				Sys_Yield();

const int MAX_CRITICAL_SECTIONS		= 4;

enum {
	CRITICAL_SECTION_ZERO = 0,
	CRITICAL_SECTION_ONE,
	CRITICAL_SECTION_TWO,
	CRITICAL_SECTION_THREE
};

#endif	// !__SYS_THREADING_H__

