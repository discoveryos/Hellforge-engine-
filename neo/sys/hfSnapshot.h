#ifndef __SNAPSHOT_H__
#define __SNAPSHOT_H__

#include "hfSnapshot_Jobs.h"

extern hfCVar net_verboseSnapshot;
#define NET_VERBOSESNAPSHOT_PRINT	if ( net_verboseSnapshot.GetInteger() > 0 ) hfLib::Printf
#define NET_VERBOSESNAPSHOT_PRINT_LEVEL( X, Y )  if ( net_verboseSnapshot.GetInteger() >= ( X ) ) hfLib::Printf( Y )

/*
A snapshot contains a list of objects and their states
*/
class hfSnapShot {
public:
	hfSnapShot();
	hfSnapShot( const hfSnapShot & other );
	~hfSnapShot();

	void operator=( const hfSnapShot & other );

	// clears the snapshot
	void Clear();

	int  GetTime() const { return time; }
	void SetTime( int t ) { time = t; }

	int  GetRecvTime() const { return recvTime; }
	void SetRecvTime( int t ) { recvTime = t; }

	// Loads only sequence and baseSequence values from the compressed stream
	static void PeekDeltaSequence( const char * deltaMem, int deltaSize, int & sequence, int & baseSequence );

	// Reads a new object state packet, which is assumed to be delta compressed against this snapshot
	bool ReadDeltaForJob( const char * deltaMem, int deltaSize, int visIndex, hfSnapShot * templateStates );
	bool ReadDelta( hfFile * file, int visIndex );

	// Writes an object state packet which is delta compressed against the old snapshot
	struct objectBuffer_t {
		objectBuffer_t() : data( NULL ), size( 0 ) { }
		objectBuffer_t( int s ) : data( NULL ), size( s ) { Alloc( s ); }
		objectBuffer_t( const objectBuffer_t & o ) : data( NULL ), size( 0 ) { *this = o; }
		~objectBuffer_t() { _Release(); }
		void Alloc( int size );
		int NumRefs() { return data == NULL ? 0 : data[size]; }
		objectSize_t Size() const { return size; }
		byte * Ptr() { return data == NULL ? NULL : data ; }
		byte & operator[]( int i ) { return data[i]; }
		void operator=( const objectBuffer_t & other );

		// (not making private because of hfSnapshot)
		void _AddRef();
		void _Release();
	private:
		byte *			data;
		objectSize_t	size;		
	};

	struct objectState_t {
		objectState_t() : 
			objectNum( 0 ),
			visMask( MAX_UNSIGNED_TYPE( uint32 ) ),
			stale( false ),
			deleted( false ),
			changedCount( 0 ),
			createdFromTemplate( false ),
			
			expectedSequence( 0 )
			{ }
		void Print( const char * name );

		uint16			objectNum;
		objectBuffer_t	buffer;
		uint32			visMask;
		bool			stale;			// easy way for clients to check if ss obj is stale. Probably temp till client side of vismask system is more fleshed out
		bool			deleted;
		int				changedCount;	// Incremented each time the state changed
		int				expectedSequence;
		bool			createdFromTemplate;
	};

	struct submitDeltaJobsInfo_t {
		objParms_t *		objParms;				// Start of object parms
		int					maxObjParms;			// Max parms (which will dictate how many objects can be processed)
		uint8 *				objMemory;				// Memory that objects were written out to
		objHeader_t *		headers;				// Memory for headers
		int					maxHeaders;
		int					maxObjMemory;			// Max memory (which will dictate when syncs need to occur)
		lzwParm_t *			lzwParms;				// Start of lzw parms
		int					maxDeltaParms;			// Max lzw parms (which will dictate how many syncs we can have)
		
		hfSnapShot *		oldSnap;				// snap we are comparing this snap to (to produce a delta)
		int					visIndex; 
		int					baseSequence;

		hfSnapShot *		templateStates;			// states for new snapObj that arent in old states
		
		lzwInOutData_t *	lzwInOutData;
	};

	void SubmitWriteDeltaToJobs( const submitDeltaJobsInfo_t & submitDeltaJobInfo );

	bool WriteDelta( hfSnapShot & old, int visIndex, hfFile * file, int maxLength, int optimalLength = 0 );

	// Adds an object to the state, overwrites any existing object with the same number
	objectState_t * S_AddObject( int objectNum, uint32 visMask, const hfBitMsg & msg, const char * tag = NULL ) { return S_AddObject( objectNum, visMask, msg.GetReadData(), msg.GetSize(), tag ); }
	objectState_t * S_AddObject( int objectNum, uint32 visMask, const byte * buffer, int size, const char * tag = NULL ) { return S_AddObject( objectNum, visMask, (const char *)buffer, size, tag ); }
	objectState_t * S_AddObject( int objectNum, uint32 visMask, const char * buffer, int size, const char * tag = NULL );
	bool CopyObject( const hfSnapShot & oldss, int objectNum, bool forceStale = false );
	int CompareObject( const hfSnapShot * oldss, int objectNum, int start=0, int end=0, int oldStart=0 );

	// returns the number of objects in this snapshot
	int NumObjects() const { return objectStates.Num(); }

	// Returns the object number of the specified object, also fills the bitmsg
	int GetObjectMsgByIndex( int i, hfBitMsg & msg, bool ignoreIfStale = false ) const;

	// returns true if the object was found in the snapshot
	bool GetObjectMsgByID( int objectNum, hfBitMsg & msg, bool ignoreIfStale = false ) { return GetObjectMsgByIndex( FindObjectIndexByID( objectNum ), msg, ignoreIfStale ) == objectNum; }

	// returns the object index or -1 if it's not found
	int FindObjectIndexByID( int objectNum ) const;
	
	// returns the object by id, or NULL if not found
	objectState_t *	FindObjectByID( int objectNum ) const;

	// Returns whether or not an object is stale
	bool ObjectIsStaleByIndex( int i ) const;

	int ObjectChangedCountByIndex( int i ) const;

	// clears the empty states from the snapshot snapshot
	void CleanupEmptyStates();

	void PrintReport();

	void UpdateExpectedSeq( int newSeq );

	void			ApplyToExistingState( int objId, hfBitMsg & msg );
	objectState_t *	GetTemplateState( int objNum, hfSnapShot * templateStates, objectState_t * newState = NULL );

	void	RemoveObject( int objId );

private:

	hfList< objectState_t *, TAG_IDLIB_LIST_SNAPSHOT>							objectStates;
	hfBlockAlloc< objectState_t, 16, TAG_NETWORKING >	allocatedObjs;

	int													time;
	int													recvTime;

	int				BinarySearch( int objectNum ) const;
	objectState_t &	FindOrCreateObjectByID( int objectNum );					// objIndex is optional parm for returning the index of the obj

	void			SubmitObjectJob(	const submitDeltaJobsInfo_t &	submitDeltaJobsInfo,		// Struct containing parameters originally passed in to SubmitWriteDeltaToJobs
										objectState_t *					newState,					// New obj state (can be NULL, which means deleted)
										objectState_t *					oldState,					// Old obj state (can be NULL, which means new)
										objParms_t *&					baseObjParm,				// Starting obj parm of current stream
										objParms_t *&					curObjParm,					// Current obj parm of current stream
										objHeader_t *&					curHeader,					// Current header dest
										uint8 *&						curObjDest,					// Current write pos of current obj
										lzwParm_t *&					curlzwParm  );				// Current delta parm for next lzw job
	void SubmitLZWJob( 
		const submitDeltaJobsInfo_t &	writeDeltaInfo,		// Struct containing parameters originally passed in to SubmitWriteDeltaToJobs
		objParms_t *&					baseObjParm,		// Pointer to the first obj parm for the current stream
		objParms_t *&					curObjParm,			// Current obj parm
		lzwParm_t *&					curlzwParm,			// Current delta parm
		bool							saveDictionary		// If true, this is the first of several calls which will be appended
	);		
	
	void WriteObject( hfFile * file, int visIndex, objectState_t * newState, objectState_t * oldState, int & lastobjectNum );
	void FreeObjectState( int index );
};

#endif // __SNAPSHOT_H__

