#ifndef __HEAP_H__
#define __HEAP_H__

/*
===============================================================================

	Memory Management

===============================================================================
*/

// memory tag names are used to sort allocations for sys_dumpMemory and other reporting functions
enum memTag_t {
#define MEM_TAG( x )	TAG_##x,
#include "sys/hfsys_alloc_tags.h"
	TAG_NUM_TAGS,
};

static const int MAX_TAGS = 256;




void *		Mem_Alloc16( const int size, const memTag_t tag );
void		Mem_Free16( void *ptr );

HF_INLINE void *	Mem_Alloc( const int size, const memTag_t tag ) { return Mem_Alloc16( size, tag ); }
HF_INLINE void		Mem_Free( void *ptr ) { Mem_Free16( ptr ); }

void *		Mem_ClearedAlloc( const int size, const memTag_t tag );
char *		Mem_CopyString( const char *in );

HF_INLINE void *operator new( size_t s ) {
	return Mem_Alloc( s, TAG_NEW );
}
HF_INLINE void operator delete( void *p ) {
	Mem_Free( p );
}
HF_INLINE void *operator new[]( size_t s ) {
	return Mem_Alloc( s, TAG_NEW );
}
HF_INLINE void operator delete[]( void *p ) {
	Mem_Free( p );
}
HF_INLINE void *operator new( size_t s, memTag_t tag ) {
	return Mem_Alloc( s, tag );
}
HF_INLINE void operator delete( void *p, memTag_t tag ) {
	Mem_Free( p );
}
HF_INLINE void *operator new[]( size_t s, memTag_t tag ) {
	return Mem_Alloc( s, tag );
}
HF_INLINE void operator delete[]( void *p, memTag_t tag ) {
	Mem_Free( p );
}

// Define replacements for the PS3 library's aligned new operator.
// Without these, allocations of objects with 32 byte or greater alignment
// may not go through our memory system.

/*
================================================
hfTempArray is an array that is automatically free'd when it goes out of scope.
There is no "cast" operator because these are very unsafe.

The template parameter MUST BE POD!

Compile time asserting POD-ness of the template parameter is complicated due
to our vector classes that need a default constructor but are otherwise
considered POD.
================================================
*/
template < class T >
class hfTempArray {
public:
	hfTempArray( hfTempArray<T> & other );
	hfTempArray( unsigned int num );

	~hfTempArray();

	T & operator []( unsigned int i ) { assert( i < num ); return buffer[i]; }
	const T & operator []( unsigned int i ) const { assert( i < num ); return buffer[i]; }

	T * Ptr() { return buffer; }
	const T* Ptr() const { return buffer; }

	size_t Size( ) const { return num * sizeof( T ); }
	unsigned int Num( ) const { return num; }

	void Zero() { memset( Ptr(), 0, Size() ); }

private:
	T *				buffer;		// Ensure this buffer comes first, so this == &this->buffer
	unsigned int	num;
};

/*
========================
hfTempArray::hfTempArray
========================
*/
template < class T >
HF_INLINE hfTempArray<T>::hfTempArray( hfTempArray<T> & other ) {
	this->num = other.num;
	this->buffer = other.buffer;
	other.num = 0;
	other.buffer = NULL;
}

/*
========================
hfTempArray::hfTempArray
========================
*/
template < class T >
HF_INLINE hfTempArray<T>::hfTempArray( unsigned int num ) {
	this->num = num;
	buffer = (T*)Mem_Alloc( num * sizeof( T ), TAG_TEMP );
}

/*
========================
hfTempArray::~hfTempArray
========================
*/
template < class T >
HF_INLINE hfTempArray<T>::~hfTempArray() {
	Mem_Free( buffer );
}

/*
===============================================================================

	Block based allocator for fixed size objects.

	All objects of the 'type' are properly constructed and destructed when reused.

===============================================================================
*/

#define BLOCK_ALLOC_ALIGNMENT 16

// Define this to force all block allocators to act like normal new/delete allocation
// for tool checking.
//#define	FORCE_DISCRETE_BLOCK_ALLOCS

/*
================================================
hfBlockAlloc is a block-based allocator for fixed-size objects.

All objects are properly constructed and destructed.
================================================
*/
template<class _type_, int _blockSize_, memTag_t memTag = TAG_BLOCKALLOC>
class hfBlockAlloc {
public:
	HF_INLINE			hfBlockAlloc( bool clear = false );
	HF_INLINE			~hfBlockAlloc();

	// returns total size of allocated memory
	size_t				Allocated() const { return total * sizeof( _type_ ); }

	// returns total size of allocated memory including size of (*this)
	size_t				Size() const { return sizeof( *this ) + Allocated(); }

	HF_INLINE void		Shutdown();
	HF_INLINE void		SetFixedBlocks( int numBlocks );
	HF_INLINE void		FreeEmptyBlocks();

	HF_INLINE _type_ *	Alloc();
	HF_INLINE void		Free( _type_ *element );

	int					GetTotalCount() const { return total; }
	int					GetAllocCount() const { return active; }
	int					GetFreeCount() const { return total - active; }

private:
	union element_t {
		_type_ *		data;	// this is a hack to make sure the save game system marks _type_ as saveable
		element_t *		next;
		byte			buffer[( CONST_MAX( sizeof( _type_ ), sizeof( element_t * ) ) + ( BLOCK_ALLOC_ALIGNMENT - 1 ) ) & ~( BLOCK_ALLOC_ALIGNMENT - 1 )];
	};

	class hfBlock {
	public:
		element_t		elements[_blockSize_];
		hfBlock *		next;
		element_t *		free;		// list with free elements in this block (temp used only by FreeEmptyBlocks)
		int				freeCount;	// number of free elements in this block (temp used only by FreeEmptyBlocks)
	};

	hfBlock *			blocks;
	element_t *			free;
	int					total;
	int					active;
	bool				allowAllocs;
	bool				clearAllocs;

	HF_INLINE void		AllocNewBlock();
};

/*
========================
hfBlockAlloc<_type_,_blockSize_,align_t>::hfBlockAlloc
========================
*/
template<class _type_, int _blockSize_, memTag_t memTag>
HF_INLINE hfBlockAlloc<_type_,_blockSize_,memTag>::hfBlockAlloc( bool clear ) :
	blocks( NULL ),
	free( NULL ),
	total( 0 ),
	active( 0 ),
	allowAllocs( true ),
	clearAllocs( clear )
{
}

/*
========================
hfBlockAlloc<_type_,_blockSize__,align_t>::~hfBlockAlloc
========================
*/
template<class _type_, int _blockSize_, memTag_t memTag>
HF_INLINE hfBlockAlloc<_type_,_blockSize_,memTag>::~hfBlockAlloc() {
	Shutdown();
}

/*
========================
hfBlockAlloc<_type_,_blockSize_,align_t>::Alloc
========================
*/
template<class _type_, int _blockSize_, memTag_t memTag>
HF_INLINE _type_ * hfBlockAlloc<_type_,_blockSize_,memTag>::Alloc() {
#ifdef FORCE_DISCRETE_BLOCK_ALLOCS
	// for debugging tools
	return new _type_;
#else
	if ( free == NULL ) {
		if ( !allowAllocs ) {
			return NULL;
		}
		AllocNewBlock();
	}

	active++;
	element_t * element = free;
	free = free->next;
	element->next = NULL;

	_type_ * t = (_type_ *) element->buffer;
	if ( clearAllocs ) {
		memset( t, 0, sizeof( _type_ ) );
	}
	new ( t ) _type_;
	return t;
#endif
}

/*
========================
hfBlockAlloc<_type_,_blockSize_,align_t>::Free
========================
*/
template<class _type_, int _blockSize_, memTag_t memTag>
HF_INLINE void hfBlockAlloc<_type_,_blockSize_,memTag>::Free( _type_ * t ) {
#ifdef FORCE_DISCRETE_BLOCK_ALLOCS
	// for debugging tools
	delete t;
#else
	if ( t == NULL ) {
		return;
	}

	t->~_type_();

	element_t * element = (element_t *)( t );
	element->next = free;
	free = element;
	active--;
#endif
}

/*
========================
hfBlockAlloc<_type_,_blockSize_,align_t>::Shutdown
========================
*/
template<class _type_, int _blockSize_, memTag_t memTag>
HF_INLINE void hfBlockAlloc<_type_,_blockSize_,memTag>::Shutdown() {
	while( blocks != NULL ) {
		hfBlock * block = blocks;
		blocks = blocks->next;
		Mem_Free( block );
	}
	blocks = NULL;
	free = NULL;
	total = active = 0;
}

/*
========================
hfBlockAlloc<_type_,_blockSize_,align_t>::SetFixedBlocks
========================
*/
template<class _type_, int _blockSize_, memTag_t memTag>
HF_INLINE void hfBlockAlloc<_type_,_blockSize_,memTag>::SetFixedBlocks( int numBlocks ) {
	int currentNumBlocks = 0;
	for ( hfBlock * block = blocks; block != NULL; block = block->next ) {
		currentNumBlocks++;
	}
	for ( int i = currentNumBlocks; i < numBlocks; i++ ) {
		AllocNewBlock();
	}
	allowAllocs = false;
}

/*
========================
hfBlockAlloc<_type_,_blockSize_,align_t>::AllocNewBlock
========================
*/
template<class _type_, int _blockSize_, memTag_t memTag>
HF_INLINE void hfBlockAlloc<_type_,_blockSize_,memTag>::AllocNewBlock() {
	hfBlock * block = (hfBlock *)Mem_Alloc( sizeof( hfBlock ), memTag );
	block->next = blocks;
	blocks = block;
	for ( int i = 0; i < _blockSize_; i++ ) {
		block->elements[i].next = free;
		free = &block->elements[i];
		assert( ( ( (UINT_PTR)free ) & ( BLOCK_ALLOC_ALIGNMENT - 1 ) ) == 0 );
	}
	total += _blockSize_;
}

/*
========================
hfBlockAlloc<_type_,_blockSize_,align_t>::FreeEmptyBlocks
========================
*/
template<class _type_, int _blockSize_, memTag_t memTag>
HF_INLINE void hfBlockAlloc<_type_,_blockSize_,memTag>::FreeEmptyBlocks() {
	// first count how many free elements are in each block
	// and build up a free chain per block
	for ( hfBlock * block = blocks; block != NULL; block = block->next ) {
		block->free = NULL;
		block->freeCount = 0;
	}
	for ( element_t * element = free; element != NULL; ) {
		element_t * next = element->next;
		for ( hfBlock * block = blocks; block != NULL; block = block->next ) {
			if ( element >= block->elements && element < block->elements + _blockSize_ ) {
				element->next = block->free;
				block->free = element;
				block->freeCount++;
				break;
			}
		}
		// if this assert fires, we couldn't find the element in any block
		assert( element->next != next );
		element = next;
	}
	// now free all blocks whose free count == _blockSize_
	hfBlock * prevBlock = NULL;
	for ( hfBlock * block = blocks; block != NULL; ) {
		hfBlock * next = block->next;
		if ( block->freeCount == _blockSize_ ) {
			if ( prevBlock == NULL ) {
				assert( blocks == block );
				blocks = block->next;
			} else {
				assert( prevBlock->next == block );
				prevBlock->next = block->next;
			}
			Mem_Free( block );
			total -= _blockSize_;
		} else {
			prevBlock = block;
		}
		block = next;
	}
	// now rebuild the free chain
	free = NULL;
	for ( hfBlock * block = blocks; block != NULL; block = block->next ) {
		for ( element_t * element = block->free; element != NULL; ) {
			element_t * next = element->next;
			element->next = free;
			free = element;
			element = next;
		}
	}
}

/*
==============================================================================

	Dynamic allocator, simple wrapper for normal allocations which can
	be interchanged with hfDynamicBlockAlloc.

	No constructor is called for the 'type'.
	Allocated blocks are always 16 byte aligned.

==============================================================================
*/

template<class type, int baseBlockSize, int minBlockSize>
class hfDynamicAlloc {
public:
									hfDynamicAlloc();
									~hfDynamicAlloc();

	void							Init();
	void							Shutdown();
	void							SetFixedBlocks( int numBlocks ) {}
	void							SetLockMemory( bool lock ) {}
	void							FreeEmptyBaseBlocks() {}

	type *							Alloc( const int num );
	type *							Resize( type *ptr, const int num );
	void							Free( type *ptr );
	const char *					CheckMemory( const type *ptr ) const;

	int								GetNumBaseBlocks() const { return 0; }
	int								GetBaseBlockMemory() const { return 0; }
	int								GetNumUsedBlocks() const { return numUsedBlocks; }
	int								GetUsedBlockMemory() const { return usedBlockMemory; }
	int								GetNumFreeBlocks() const { return 0; }
	int								GetFreeBlockMemory() const { return 0; }
	int								GetNumEmptyBaseBlocks() const { return 0; }

private:
	int								numUsedBlocks;			// number of used blocks
	int								usedBlockMemory;		// total memory in used blocks

	int								numAllocs;
	int								numResizes;
	int								numFrees;

	void							Clear();
};

template<class type, int baseBlockSize, int minBlockSize>
hfDynamicAlloc<type, baseBlockSize, minBlockSize>::hfDynamicAlloc() {
	Clear();
}

template<class type, int baseBlockSize, int minBlockSize>
hfDynamicAlloc<type, baseBlockSize, minBlockSize>::~hfDynamicAlloc() {
	Shutdown();
}

template<class type, int baseBlockSize, int minBlockSize>
void hfDynamicAlloc<type, baseBlockSize, minBlockSize>::Init() {
}

template<class type, int baseBlockSize, int minBlockSize>
void hfDynamicAlloc<type, baseBlockSize, minBlockSize>::Shutdown() {
	Clear();
}

template<class type, int baseBlockSize, int minBlockSize>
type *hfDynamicAlloc<type, baseBlockSize, minBlockSize>::Alloc( const int num ) {
	numAllocs++;
	if ( num <= 0 ) {
		return NULL;
	}
	numUsedBlocks++;
	usedBlockMemory += num * sizeof( type );
	return Mem_Alloc16( num * sizeof( type ), TAG_BLOCKALLOC );
}

template<class type, int baseBlockSize, int minBlockSize>
type *hfDynamicAlloc<type, baseBlockSize, minBlockSize>::Resize( type *ptr, const int num ) {

	numResizes++;

	if ( ptr == NULL ) {
		return Alloc( num );
	}

	if ( num <= 0 ) {
		Free( ptr );
		return NULL;
	}

	assert( 0 );
	return ptr;
}

template<class type, int baseBlockSize, int minBlockSize>
void hfDynamicAlloc<type, baseBlockSize, minBlockSize>::Free( type *ptr ) {
	numFrees++;
	if ( ptr == NULL ) {
		return;
	}
	Mem_Free16( ptr );
}

template<class type, int baseBlockSize, int minBlockSize>
const char *hfDynamicAlloc<type, baseBlockSize, minBlockSize>::CheckMemory( const type *ptr ) const {
	return NULL;
}

template<class type, int baseBlockSize, int minBlockSize>
void hfDynamicAlloc<type, baseBlockSize, minBlockSize>::Clear() {
	numUsedBlocks = 0;
	usedBlockMemory = 0;
	numAllocs = 0;
	numResizes = 0;
	numFrees = 0;
}


/*
==============================================================================

	Fast dynamic block allocator.

	No constructor is called for the 'type'.
	Allocated blocks are always 16 byte aligned.

==============================================================================
*/

#include "containers/hfBTree.h"

//#define DYNAMIC_BLOCK_ALLOC_CHECK

template<class type>
class hfDynamicBlock {
public:
	type *							GetMemory() const { return (type *)( ( (byte *) this ) + sizeof( hfDynamicBlock<type> ) ); }
	int								GetSize() const { return abs( size ); }
	void							SetSize( int s, bool isBaseBlock ) { size = isBaseBlock ? -s : s; }
	bool							IsBaseBlock() const { return ( size < 0 ); }

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	int								id[3];
	void *							allocator;
#endif

	int								size;					// size in bytes of the block
	hfDynamicBlock<type> *			prev;					// previous memory block
	hfDynamicBlock<type> *			next;					// next memory block
	hfBTreeNode<hfDynamicBlock<type>,int> *node;			// node in the B-Tree with free blocks
};

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_ = TAG_BLOCKALLOC>
class hfDynamicBlockAlloc {
public:
									hfDynamicBlockAlloc();
									~hfDynamicBlockAlloc();

	void							Init();
	void							Shutdown();
	void							SetFixedBlocks( int numBlocks );
	void							SetLockMemory( bool lock );
	void							FreeEmptyBaseBlocks();

	type *							Alloc( const int num );
	type *							Resize( type *ptr, const int num );
	void							Free( type *ptr );
	const char *					CheckMemory( const type *ptr ) const;

	int								GetNumBaseBlocks() const { return numBaseBlocks; }
	int								GetBaseBlockMemory() const { return baseBlockMemory; }
	int								GetNumUsedBlocks() const { return numUsedBlocks; }
	int								GetUsedBlockMemory() const { return usedBlockMemory; }
	int								GetNumFreeBlocks() const { return numFreeBlocks; }
	int								GetFreeBlockMemory() const { return freeBlockMemory; }
	int								GetNumEmptyBaseBlocks() const;

private:
	hfDynamicBlock<type> *			firstBlock;				// first block in list in order of increasing address
	hfDynamicBlock<type> *			lastBlock;				// last block in list in order of increasing address
	hfBTree<hfDynamicBlock<type>,int,4>freeTree;			// B-Tree with free memory blocks
	bool							allowAllocs;			// allow base block allocations
	bool							lockMemory;				// lock memory so it cannot get swapped out

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	int								blockId[3];
#endif

	int								numBaseBlocks;			// number of base blocks
	int								baseBlockMemory;		// total memory in base blocks
	int								numUsedBlocks;			// number of used blocks
	int								usedBlockMemory;		// total memory in used blocks
	int								numFreeBlocks;			// number of free blocks
	int								freeBlockMemory;		// total memory in free blocks

	int								numAllocs;
	int								numResizes;
	int								numFrees;
	
	memTag_t						tag;

	void							Clear();
	hfDynamicBlock<type> *			AllocInternal( const int num );
	hfDynamicBlock<type> *			ResizeInternal( hfDynamicBlock<type> *block, const int num );
	void							FreeInternal( hfDynamicBlock<type> *block );
	void							LinkFreeInternal( hfDynamicBlock<type> *block );
	void							UnlinkFreeInternal( hfDynamicBlock<type> *block );
	void							CheckMemory() const;
};

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_>
hfDynamicBlockAlloc<type, baseBlockSize, minBlockSize, _tag_>::hfDynamicBlockAlloc() {
	tag = _tag_;
	Clear();
}

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_>
hfDynamicBlockAlloc<type, baseBlockSize, minBlockSize, _tag_>::~hfDynamicBlockAlloc() {
	Shutdown();
}

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_>
void hfDynamicBlockAlloc<type, baseBlockSize, minBlockSize, _tag_>::Init() {
	freeTree.Init();
}

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_>
void hfDynamicBlockAlloc<type, baseBlockSize, minBlockSize, _tag_>::Shutdown() {
	hfDynamicBlock<type> *block;

	for ( block = firstBlock; block != NULL; block = block->next ) {
		if ( block->node == NULL ) {
			FreeInternal( block );
		}
	}

	for ( block = firstBlock; block != NULL; block = firstBlock ) {
		firstBlock = block->next;
		assert( block->IsBaseBlock() );
		if ( lockMemory ) {
			//hfLib::sys->UnlockMemory( block, block->GetSize() + (int)sizeof( hfDynamicBlock<type> ) );
		}
		Mem_Free16( block );
	}

	freeTree.Shutdown();

	Clear();
}

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_>
void hfDynamicBlockAlloc<type, baseBlockSize, minBlockSize, _tag_>::SetFixedBlocks( int numBlocks ) {
	hfDynamicBlock<type> *block;

	for ( int i = numBaseBlocks; i < numBlocks; i++ ) {
		block = ( hfDynamicBlock<type> * ) Mem_Alloc16( baseBlockSize, _tag_ );
		if ( lockMemory ) {
			//hfLib::sys->LockMemory( block, baseBlockSize );
		}
#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
		memcpy( block->id, blockId, sizeof( block->id ) );
		block->allocator = (void*)this;
#endif
		block->SetSize( baseBlockSize - (int)sizeof( hfDynamicBlock<type> ), true );
		block->next = NULL;
		block->prev = lastBlock;
		if ( lastBlock ) {
			lastBlock->next = block;
		} else {
			firstBlock = block;
		}
		lastBlock = block;
		block->node = NULL;

		FreeInternal( block );

		numBaseBlocks++;
		baseBlockMemory += baseBlockSize;
	}

	allowAllocs = false;
}

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_>
void hfDynamicBlockAlloc<type, baseBlockSize, minBlockSize, _tag_>::SetLockMemory( bool lock ) {
	lockMemory = lock;
}

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_>
void hfDynamicBlockAlloc<type, baseBlockSize, minBlockSize, _tag_>::FreeEmptyBaseBlocks() {
	hfDynamicBlock<type> *block, *next;

	for ( block = firstBlock; block != NULL; block = next ) {
		next = block->next;

		if ( block->IsBaseBlock() && block->node != NULL && ( next == NULL || next->IsBaseBlock() ) ) {
			UnlinkFreeInternal( block );
			if ( block->prev ) {
				block->prev->next = block->next;
			} else {
				firstBlock = block->next;
			}
			if ( block->next ) {
				block->next->prev = block->prev;
			} else {
				lastBlock = block->prev;
			}
			if ( lockMemory ) {
				//hfLib::sys->UnlockMemory( block, block->GetSize() + (int)sizeof( hfDynamicBlock<type> ) );
			}
			numBaseBlocks--;
			baseBlockMemory -= block->GetSize() + (int)sizeof( hfDynamicBlock<type> );
			Mem_Free16( block );
		}
	}

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	CheckMemory();
#endif
}

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_>
int hfDynamicBlockAlloc<type, baseBlockSize, minBlockSize, _tag_>::GetNumEmptyBaseBlocks() const {
	int numEmptyBaseBlocks;
	hfDynamicBlock<type> *block;

	numEmptyBaseBlocks = 0;
	for ( block = firstBlock; block != NULL; block = block->next ) {
		if ( block->IsBaseBlock() && block->node != NULL && ( block->next == NULL || block->next->IsBaseBlock() ) ) {
			numEmptyBaseBlocks++;
		}
	}
	return numEmptyBaseBlocks;
}

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_>
type *hfDynamicBlockAlloc<type, baseBlockSize, minBlockSize, _tag_>::Alloc( const int num ) {
	hfDynamicBlock<type> *block;

	numAllocs++;

	if ( num <= 0 ) {
		return NULL;
	}

	block = AllocInternal( num );
	if ( block == NULL ) {
		return NULL;
	}
	block = ResizeInternal( block, num );
	if ( block == NULL ) {
		return NULL;
	}

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	CheckMemory();
#endif

	numUsedBlocks++;
	usedBlockMemory += block->GetSize();

	return block->GetMemory();
}

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_>
type *hfDynamicBlockAlloc<type, baseBlockSize, minBlockSize, _tag_>::Resize( type *ptr, const int num ) {

	numResizes++;

	if ( ptr == NULL ) {
		return Alloc( num );
	}

	if ( num <= 0 ) {
		Free( ptr );
		return NULL;
	}

	hfDynamicBlock<type> *block = ( hfDynamicBlock<type> * ) ( ( (byte *) ptr ) - (int)sizeof( hfDynamicBlock<type> ) );

	usedBlockMemory -= block->GetSize();

	block = ResizeInternal( block, num );
	if ( block == NULL ) {
		return NULL;
	}

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	CheckMemory();
#endif

	usedBlockMemory += block->GetSize();

	return block->GetMemory();
}

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_>
void hfDynamicBlockAlloc<type, baseBlockSize, minBlockSize, _tag_>::Free( type *ptr ) {

	numFrees++;

	if ( ptr == NULL ) {
		return;
	}

	hfDynamicBlock<type> *block = ( hfDynamicBlock<type> * ) ( ( (byte *) ptr ) - (int)sizeof( hfDynamicBlock<type> ) );

	numUsedBlocks--;
	usedBlockMemory -= block->GetSize();

	FreeInternal( block );

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	CheckMemory();
#endif
}

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_>
const char *hfDynamicBlockAlloc<type, baseBlockSize, minBlockSize, _tag_>::CheckMemory( const type *ptr ) const {
	hfDynamicBlock<type> *block;

	if ( ptr == NULL ) {
		return NULL;
	}

	block = ( hfDynamicBlock<type> * ) ( ( (byte *) ptr ) - (int)sizeof( hfDynamicBlock<type> ) );

	if ( block->node != NULL ) {
		return "memory has been freed";
	}

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	if ( block->id[0] != 0x11111111 || block->id[1] != 0x22222222 || block->id[2] != 0x33333333 ) {
		return "memory has invalid id";
	}
	if ( block->allocator != (void*)this ) {
		return "memory was allocated with different allocator";
	}
#endif

	/* base blocks can be larger than baseBlockSize which can cause this code to fail
	hfDynamicBlock<type> *base;
	for ( base = firstBlock; base != NULL; base = base->next ) {
		if ( base->IsBaseBlock() ) {
			if ( ((int)block) >= ((int)base) && ((int)block) < ((int)base) + baseBlockSize ) {
				break;
			}
		}
	}
	if ( base == NULL ) {
		return "no base block found for memory";
	}
	*/

	return NULL;
}

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_>
void hfDynamicBlockAlloc<type, baseBlockSize, minBlockSize, _tag_>::Clear() {
	firstBlock = lastBlock = NULL;
	allowAllocs = true;
	lockMemory = false;
	numBaseBlocks = 0;
	baseBlockMemory = 0;
	numUsedBlocks = 0;
	usedBlockMemory = 0;
	numFreeBlocks = 0;
	freeBlockMemory = 0;
	numAllocs = 0;
	numResizes = 0;
	numFrees = 0;

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	blockId[0] = 0x11111111;
	blockId[1] = 0x22222222;
	blockId[2] = 0x33333333;
#endif
}

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_>
hfDynamicBlock<type> *hfDynamicBlockAlloc<type, baseBlockSize, minBlockSize, _tag_>::AllocInternal( const int num ) {
	hfDynamicBlock<type> *block;
	int alignedBytes = ( num * sizeof( type ) + 15 ) & ~15;

	block = freeTree.FindSmallestLargerEqual( alignedBytes );
	if ( block != NULL ) {
		UnlinkFreeInternal( block );
	} else if ( allowAllocs ) {
		int allocSize = Max( baseBlockSize, alignedBytes + (int)sizeof( hfDynamicBlock<type> ) );
		block = ( hfDynamicBlock<type> * ) Mem_Alloc16( allocSize, _tag_ );
		if ( lockMemory ) {
			//hfLib::sys->LockMemory( block, baseBlockSize );
		}
#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
		memcpy( block->id, blockId, sizeof( block->id ) );
		block->allocator = (void*)this;
#endif
		block->SetSize( allocSize - (int)sizeof( hfDynamicBlock<type> ), true );
		block->next = NULL;
		block->prev = lastBlock;
		if ( lastBlock ) {
			lastBlock->next = block;
		} else {
			firstBlock = block;
		}
		lastBlock = block;
		block->node = NULL;

		numBaseBlocks++;
		baseBlockMemory += allocSize;
	}

	return block;
}

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_>
hfDynamicBlock<type> *hfDynamicBlockAlloc<type, baseBlockSize, minBlockSize, _tag_>::ResizeInternal( hfDynamicBlock<type> *block, const int num ) {
	int alignedBytes = ( num * sizeof( type ) + 15 ) & ~15;

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	assert( block->id[0] == 0x11111111 && block->id[1] == 0x22222222 && block->id[2] == 0x33333333 && block->allocator == (void*)this );
#endif

	// if the new size is larger
	if ( alignedBytes > block->GetSize() ) {

		hfDynamicBlock<type> *nextBlock = block->next;

		// try to annexate the next block if it's free
		if ( nextBlock && !nextBlock->IsBaseBlock() && nextBlock->node != NULL &&
				block->GetSize() + (int)sizeof( hfDynamicBlock<type> ) + nextBlock->GetSize() >= alignedBytes ) {

			UnlinkFreeInternal( nextBlock );
			block->SetSize( block->GetSize() + (int)sizeof( hfDynamicBlock<type> ) + nextBlock->GetSize(), block->IsBaseBlock() );
			block->next = nextBlock->next;
			if ( nextBlock->next ) {
				nextBlock->next->prev = block;
			} else {
				lastBlock = block;
			}
		} else {
			// allocate a new block and copy
			hfDynamicBlock<type> *oldBlock = block;
			block = AllocInternal( num );
			if ( block == NULL ) {
				return NULL;
			}
			memcpy( block->GetMemory(), oldBlock->GetMemory(), oldBlock->GetSize() );
			FreeInternal( oldBlock );
		}
	}

	// if the unused space at the end of this block is large enough to hold a block with at least one element
	if ( block->GetSize() - alignedBytes - (int)sizeof( hfDynamicBlock<type> ) < Max( minBlockSize, (int)sizeof( type ) ) ) {
		return block;
	}

	hfDynamicBlock<type> *newBlock;

	newBlock = ( hfDynamicBlock<type> * ) ( ( (byte *) block ) + (int)sizeof( hfDynamicBlock<type> ) + alignedBytes );
#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	memcpy( newBlock->id, blockId, sizeof( newBlock->id ) );
	newBlock->allocator = (void*)this;
#endif
	newBlock->SetSize( block->GetSize() - alignedBytes - (int)sizeof( hfDynamicBlock<type> ), false );
	newBlock->next = block->next;
	newBlock->prev = block;
	if ( newBlock->next ) {
		newBlock->next->prev = newBlock;
	} else {
		lastBlock = newBlock;
	}
	newBlock->node = NULL;
	block->next = newBlock;
	block->SetSize( alignedBytes, block->IsBaseBlock() );

	FreeInternal( newBlock );

	return block;
}

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_>
void hfDynamicBlockAlloc<type, baseBlockSize, minBlockSize, _tag_>::FreeInternal( hfDynamicBlock<type> *block ) {

	assert( block->node == NULL );

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	assert( block->id[0] == 0x11111111 && block->id[1] == 0x22222222 && block->id[2] == 0x33333333 && block->allocator == (void*)this );
#endif

	// try to merge with a next free block
	hfDynamicBlock<type> *nextBlock = block->next;
	if ( nextBlock && !nextBlock->IsBaseBlock() && nextBlock->node != NULL ) {
		UnlinkFreeInternal( nextBlock );
		block->SetSize( block->GetSize() + (int)sizeof( hfDynamicBlock<type> ) + nextBlock->GetSize(), block->IsBaseBlock() );
		block->next = nextBlock->next;
		if ( nextBlock->next ) {
			nextBlock->next->prev = block;
		} else {
			lastBlock = block;
		}
	}

	// try to merge with a previous free block
	hfDynamicBlock<type> *prevBlock = block->prev;
	if ( prevBlock && !block->IsBaseBlock() && prevBlock->node != NULL ) {
		UnlinkFreeInternal( prevBlock );
		prevBlock->SetSize( prevBlock->GetSize() + (int)sizeof( hfDynamicBlock<type> ) + block->GetSize(), prevBlock->IsBaseBlock() );
		prevBlock->next = block->next;
		if ( block->next ) {
			block->next->prev = prevBlock;
		} else {
			lastBlock = prevBlock;
		}
		LinkFreeInternal( prevBlock );
	} else {
		LinkFreeInternal( block );
	}
}

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_>
HF_INLINE void hfDynamicBlockAlloc<type, baseBlockSize, minBlockSize, _tag_>::LinkFreeInternal( hfDynamicBlock<type> *block ) {
	block->node = freeTree.Add( block, block->GetSize() );
	numFreeBlocks++;
	freeBlockMemory += block->GetSize();
}

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_>
HF_INLINE void hfDynamicBlockAlloc<type, baseBlockSize, minBlockSize, _tag_>::UnlinkFreeInternal( hfDynamicBlock<type> *block ) {
	freeTree.Remove( block->node );
	block->node = NULL;
	numFreeBlocks--;
	freeBlockMemory -= block->GetSize();
}

template<class type, int baseBlockSize, int minBlockSize, memTag_t _tag_>
void hfDynamicBlockAlloc<type, baseBlockSize, minBlockSize, _tag_>::CheckMemory() const {
	hfDynamicBlock<type> *block;

	for ( block = firstBlock; block != NULL; block = block->next ) {
		// make sure the block is properly linked
		if ( block->prev == NULL ) {
			assert( firstBlock == block );
		} else {
			assert( block->prev->next == block );
		}
		if ( block->next == NULL ) {
			assert( lastBlock == block );
		} else {
			assert( block->next->prev == block );
		}
	}
}

#endif /* !__HEAP_H__ */

