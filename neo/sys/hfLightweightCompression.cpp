#pragma hdrstop
#include "../hflib/hfprecompiled.h"
#include "hfLightweightCompression.h"

/*
========================
HashIndex
========================
*/
static int HashIndex( int w, int k ) {
	return ( w ^ k ) & hfLZWCompressor::HASH_MASK;
}

/*
========================
hfLZWCompressor::Start
========================
*/
void hfLZWCompressor::Start( uint8 * data_, int maxSize_, bool append ) {
	// Clear hash
	ClearHash();
	
	if ( append ) {
		assert( lzwData->nextCode > LZW_FIRST_CODE );
		
		int originalNextCode = lzwData->nextCode;

		lzwData->nextCode = LZW_FIRST_CODE;
		
		// If we are appending, then fill up the hash
		for ( int i = LZW_FIRST_CODE; i < originalNextCode; i++ ) {
			AddToDict( lzwData->dictionaryW[i], lzwData->dictionaryK[i] );
		}
	
		assert( originalNextCode == lzwData->nextCode );
	} else {
		for ( int i = 0; i < LZW_FIRST_CODE; i++ ) {
			lzwData->dictionaryK[i] = (uint8)i;
			lzwData->dictionaryW[i] = 0xFFFF;
		}

		lzwData->nextCode		= LZW_FIRST_CODE;
		lzwData->codeBits		= LZW_START_BITS;
		lzwData->codeWord		= -1;
		lzwData->tempValue		= 0;
		lzwData->tempBits		= 0;
		lzwData->bytesWritten	= 0;
	}
		
	oldCode		= -1;		// Used by DecompressBlock
	data		= data_;

	blockSize	= 0;
	blockIndex	= 0;
	
	bytesRead	= 0;
	
	maxSize		= maxSize_;
	overflowed	= false;

	savedBytesWritten	= 0;
	savedCodeWord		= 0;
	saveCodeBits		= 0;
	savedTempValue		= 0;
	savedTempBits		= 0;
}

/*
========================
hfLZWCompressor::ReadBits
========================
*/
int hfLZWCompressor::ReadBits( int bits ) {
	int bitsToRead = bits - lzwData->tempBits;
	
	while ( bitsToRead > 0 ) {
		if ( bytesRead >= maxSize ) {
			return -1;
		}
		lzwData->tempValue |= (uint64)data[bytesRead++] << lzwData->tempBits;
		lzwData->tempBits += 8;
		bitsToRead -= 8;
	}
	
	int value = (int)lzwData->tempValue & ( ( 1 << bits ) - 1 );
	lzwData->tempValue >>= bits;
	lzwData->tempBits -= bits;
	
	return value;
}

/*
========================
hfLZWCompressor::WriteBits
========================
*/
void hfLZWCompressor::WriteBits( uint32 value, int bits ) {

	// Queue up bits into temp value
	lzwData->tempValue |= (uint64)value << lzwData->tempBits;
	lzwData->tempBits += bits;
	
	// Flush 8 bits (1 byte) at a time ( leftovers will get caught in hfLZWCompressor::End() )
	while ( lzwData->tempBits >= 8 ) {
		if ( lzwData->bytesWritten >= maxSize ) {
			overflowed = true;
			return;
		}

		data[lzwData->bytesWritten++] = (uint8)( lzwData->tempValue & 255 );
		lzwData->tempValue >>= 8;
		lzwData->tempBits -= 8;
	}
}

/*
========================
hfLZWCompressor::WriteChain
 
The chain is stored backwards, so we have to write it to a buffer then output the buffer in 
reverse. 
========================
*/
int hfLZWCompressor::WriteChain( int code ) {
	byte chain[lzwCompressionData_t::LZW_DICT_SIZE];
	int firstChar = 0;
	int i = 0;
	do {
		assert( i < lzwCompressionData_t::LZW_DICT_SIZE && code < lzwCompressionData_t::LZW_DICT_SIZE && code >= 0 );
		chain[i++] = (byte)lzwData->dictionaryK[code];
		code = lzwData->dictionaryW[code];
	} while ( code != 0xFFFF );
	firstChar = chain[--i];
	for ( ; i >= 0; i-- ) {
		block[blockSize++] = chain[i];
	}
	return firstChar;
}

/*
========================
hfLZWCompressor::DecompressBlock
========================
*/
void hfLZWCompressor::DecompressBlock() {
	assert( blockIndex == blockSize );		// Make sure we've read all we can
	
	blockIndex = 0;
	blockSize = 0;
	
	int firstChar = -1;
	while ( blockSize < LZW_BLOCK_SIZE - lzwCompressionData_t::LZW_DICT_SIZE ) {
		assert( lzwData->codeBits <= lzwCompressionData_t::LZW_DICT_BITS );

		int code = ReadBits( lzwData->codeBits );
		if ( code == -1 ) {
			break;
		}

		if ( oldCode == -1 ) {
			assert( code < 256 );
			block[blockSize++] = (uint8)code;
			oldCode = code;
			firstChar = code;
			continue;
		}

		if ( code >= lzwData->nextCode ) {
			assert( code == lzwData->nextCode );
			firstChar = WriteChain( oldCode );
			block[blockSize++] = (uint8)firstChar;
		} else {
			firstChar = WriteChain( code );
		}
		AddToDict( oldCode, firstChar );
		if ( BumpBits() ) {
			oldCode = -1;
		} else {
			oldCode = code;
		}
	}
}

/*
========================
hfLZWCompressor::ReadByte
========================
*/
int hfLZWCompressor::ReadByte( bool ignoreOverflow ) {
	if ( blockIndex == blockSize ) {
		DecompressBlock();
	}

	if ( blockIndex == blockSize ) { //-V581 DecompressBlock() updates these values, the if() isn't redundant
		if ( !ignoreOverflow ) {
			overflowed = true;
			assert( !"hfLZWCompressor::ReadByte overflowed!" );
		}
		return -1;
	}
	
	return block[blockIndex++];
}


/*
========================
hfLZWCompressor::WriteByte
========================
*/
void hfLZWCompressor::WriteByte( uint8 value ) {
	int code = Lookup( lzwData->codeWord, value );
	if ( code >= 0 ) {
		lzwData->codeWord = code;
	} else {
		WriteBits( lzwData->codeWord, lzwData->codeBits );
		if ( !BumpBits() ) {
			AddToDict( lzwData->codeWord, value );
		}
		lzwData->codeWord = value;
	}

	if ( lzwData->bytesWritten >= maxSize - ( lzwData->codeBits + lzwData->tempBits + 7 ) / 8 ) {
		overflowed = true;	// At any point, if we can't perform an End call, then trigger an overflow
		return;
	}
}

/*
========================
hfLZWCompressor::Lookup 
========================
*/
int hfLZWCompressor::Lookup( int w, int k ) {
	if ( w == -1 ) {
		return k;
	} else {
		int i = HashIndex( w, k );
		
		for ( int j = hash[i]; j != 0xFFFF; j = nextHash[j] ) {
			assert( j < lzwCompressionData_t::LZW_DICT_SIZE );
			if ( lzwData->dictionaryK[j] == k && lzwData->dictionaryW[j] == w ) { 
				return j;
			}
		}
	}
	return -1;
}

/*
========================
hfLZWCompressor::AddToDict 
========================
*/
int hfLZWCompressor::AddToDict( int w, int k ) {
	assert( w < 0xFFFF - 1 );
	assert( k < 256 );
	assert( lzwData->nextCode < lzwCompressionData_t::LZW_DICT_SIZE );
	
	lzwData->dictionaryK[lzwData->nextCode] = (uint8)k;
	lzwData->dictionaryW[lzwData->nextCode] = (uint16)w;
	int i = HashIndex( w, k );
	nextHash[lzwData->nextCode] = hash[i];
	hash[i] = (uint16)lzwData->nextCode;
	return lzwData->nextCode++;
}

/*
========================
hfLZWCompressor::BumpBits
 
Possibly increments codeBits.
	return: bool	- true, if the dictionary was cleared. 
========================
*/
bool hfLZWCompressor::BumpBits() {
	if ( lzwData->nextCode == ( 1 << lzwData->codeBits ) ) {
		lzwData->codeBits ++;
		if ( lzwData->codeBits > lzwCompressionData_t::LZW_DICT_BITS ) {
			lzwData->nextCode = LZW_FIRST_CODE;
			lzwData->codeBits = LZW_START_BITS;
			ClearHash();
			return true;
		}
	}
	return false;
}

/*
========================
hfLZWCompressor::End
========================
*/
int hfLZWCompressor::End() {
	assert( lzwData->tempBits < 8 );
	assert( lzwData->bytesWritten < maxSize - ( lzwData->codeBits + lzwData->tempBits + 7 ) / 8 );

	assert( ( Length() > 0 ) == ( lzwData->codeWord != -1 ) );

	if ( lzwData->codeWord != -1 ) {
		WriteBits( lzwData->codeWord, lzwData->codeBits );
	}

	if ( lzwData->tempBits > 0 ) {
		if ( lzwData->bytesWritten >= maxSize ) {
			overflowed = true;
			return -1;
		}
		data[lzwData->bytesWritten++] = (uint8)lzwData->tempValue & ( ( 1 << lzwData->tempBits ) - 1 );
	}
	
	return Length() > 0 ? Length() : -1;		// Total bytes written (or failure)
}

/*
========================
hfLZWCompressor::Save
========================
*/
void hfLZWCompressor::Save() { 
	assert( !overflowed );
	// Check and make sure we are at a good spot (can call End)
	assert( lzwData->bytesWritten < maxSize - ( lzwData->codeBits + lzwData->tempBits + 7 ) / 8 );

	savedBytesWritten	= lzwData->bytesWritten;
	savedCodeWord		= lzwData->codeWord;
	saveCodeBits		= lzwData->codeBits;
	savedTempValue		= lzwData->tempValue;
	savedTempBits		= lzwData->tempBits;
}

/*
========================
hfLZWCompressor::Restore
========================
*/
void hfLZWCompressor::Restore() { 
	lzwData->bytesWritten	= savedBytesWritten; 
	lzwData->codeWord		= savedCodeWord;
	lzwData->codeBits		= saveCodeBits;
	lzwData->tempValue		= savedTempValue;
	lzwData->tempBits		= savedTempBits;
}

/*
========================
hfLZWCompressor::ClearHash
========================
*/
void hfLZWCompressor::ClearHash() { 
	memset( hash, 0xFF, sizeof( hash ) ); 
}

/*
========================
hfZeroRunLengthCompressor
Simple zero based run length encoder/decoder
========================
*/

void hfZeroRunLengthCompressor::Start( uint8 * dest_, hfLZWCompressor * comp_, int maxSize_ ) {
	zeroCount	= 0;
	dest		= dest_;
	comp		= comp_;
	compressed	= 0;
	maxSize		= maxSize_;
}

bool hfZeroRunLengthCompressor::WriteRun() {
	if ( zeroCount > 0 ) {
		assert( zeroCount <= 255 );
		if ( compressed + 2 > maxSize ) {
			maxSize = -1;
			return false;
		}
		if ( comp != NULL ) {
			comp->WriteByte( 0 );
			comp->WriteByte( (uint8)zeroCount );
		} else {
			*dest++ = 0;
			*dest++ = (uint8)zeroCount;
		}
		compressed += 2;
		zeroCount = 0;
	}
	return true;
}

bool hfZeroRunLengthCompressor::WriteByte( uint8 value ) {
	if ( value != 0 || zeroCount >= 255 ) {
		if ( !WriteRun() ) {
			maxSize = -1;
			return false;
		}
	}
	
	if ( value != 0 ) {
		if ( compressed + 1 > maxSize ) {
			maxSize = -1;
			return false;
		}
		if ( comp != NULL ) {
			comp->WriteByte( value );
		} else {
			*dest++ = value; 
		}
		compressed++;
	} else {
		zeroCount++;
	}
	
	return true;
}

byte hfZeroRunLengthCompressor::ReadByte() {
	// See if we need to possibly read more data
	if ( zeroCount == 0 ) {
		int value = ReadInternal();
		if ( value == -1 ) {
			assert( 0 );
		}
		if ( value != 0 ) {
			return (byte)value;	// Return non zero values immediately
		}
		// Read the number of zeroes
		zeroCount = ReadInternal();
	}
	
	assert( zeroCount > 0 );
	
	zeroCount--;
	return 0;
}

void hfZeroRunLengthCompressor::ReadBytes( byte * dest, int count ) {
	for ( int i = 0; i < count; i++ ) {
		*dest++ = ReadByte();
	}
}

void hfZeroRunLengthCompressor::WriteBytes( uint8 * src, int count ) {
	for ( int i = 0; i < count; i++ ) {
		WriteByte( *src++ );
	}
}

int hfZeroRunLengthCompressor::End() {
	WriteRun();
	if ( maxSize == -1 ) {
		return -1;
	}
	return compressed;
}

int hfZeroRunLengthCompressor::ReadInternal() {
	compressed++;
	if ( comp != NULL ) {
		return comp->ReadByte();
	}
	return *dest++;
}
