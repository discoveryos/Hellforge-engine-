#pragma hdrstop
#include "hfprecompiled.h"

/*
================================================================================================

	hfBitMsg

================================================================================================
*/

/*
========================
hfBitMsg::CheckOverflow
========================
*/
bool hfBitMsg::CheckOverflow( int numBits ) {
	if ( numBits > GetRemainingWriteBits() ) {
		if ( !allowOverflow ) {
			hfLib::FatalError( "hfBitMsg: overflow without allowOverflow set; maxsize=%i size=%i numBits=%i numRemainingWriteBits=%i",
				GetMaxSize(), GetSize(), numBits, GetRemainingWriteBits() );
		}
		if ( numBits > ( maxSize << 3 ) ) {
			hfLib::FatalError( "hfBitMsg: %i bits is > full message size", numBits );
		}
		hfLib::Printf( "hfBitMsg: overflow\n" );
		BeginWriting();
		overflowed = true;
		return true;
	}
	return false;
}

/*
========================
hfBitMsg::GetByteSpace
========================
*/
byte *hfBitMsg::GetByteSpace( int length ) {
	byte *ptr;

	if ( !writeData ) {
		hfLib::FatalError( "hfBitMsg::GetByteSpace: cannot write to message" );
	}

	// round up to the next byte
	WriteByteAlign();

	// check for overflow
	CheckOverflow( length << 3 );

	ptr = writeData + curSize;
	curSize += length;
	return ptr;
}

#define NBM( x ) (uint64)( ( 1LL << x ) - 1 )
static uint64 maskForNumBits64[33] = {	NBM( 0x00 ), NBM( 0x01 ), NBM( 0x02 ), NBM( 0x03 ),
										NBM( 0x04 ), NBM( 0x05 ), NBM( 0x06 ), NBM( 0x07 ),
										NBM( 0x08 ), NBM( 0x09 ), NBM( 0x0A ), NBM( 0x0B ),
										NBM( 0x0C ), NBM( 0x0D ), NBM( 0x0E ), NBM( 0x0F ),
										NBM( 0x10 ), NBM( 0x11 ), NBM( 0x12 ), NBM( 0x13 ),
										NBM( 0x14 ), NBM( 0x15 ), NBM( 0x16 ), NBM( 0x17 ),
										NBM( 0x18 ), NBM( 0x19 ), NBM( 0x1A ), NBM( 0x1B ),
										NBM( 0x1C ), NBM( 0x1D ), NBM( 0x1E ), NBM( 0x1F ), 0xFFFFFFFF };

/*
========================
hfBitMsg::WriteBits

If the number of bits is negative a sign is included.
========================
*/
void hfBitMsg::WriteBits( int value, int numBits ) {
	if ( !writeData ) {
		hfLib::FatalError( "hfBitMsg::WriteBits: cannot write to message" );
	}

	// check if the number of bits is valid
	if ( numBits == 0 || numBits < -31 || numBits > 32 ) {
		hfLib::FatalError( "hfBitMsg::WriteBits: bad numBits %i", numBits );
	}

	// check for value overflows
	if ( numBits != 32 ) {
		if ( numBits > 0 ) {
			if ( value > ( 1 << numBits ) - 1 ) {
				hfLib::FatalError( "hfBitMsg::WriteBits: value overflow %d %d", 
									  value, numBits );

			} else if ( value < 0 ) {
				hfLib::FatalError( "hfBitMsg::WriteBits: value overflow %d %d", 
									  value, numBits );
			}
		} else {
			const unsigned shift = ( -1 - numBits );
			int r = 1 << shift;
			if ( value > r - 1 ) {
				hfLib::FatalError( "hfBitMsg::WriteBits: value overflow %d %d", 
									  value, numBits );

			} else if ( value < -r ) {
				hfLib::FatalError( "hfBitMsg::WriteBits: value overflow %d %d", 
									  value, numBits );
			}
		}
	}

	if ( numBits < 0 ) {
		numBits = -numBits;
	}

	// check for msg overflow
	if ( CheckOverflow( numBits ) ) {
		return;
	}

	// Merge value with possible previous leftover
	tempValue |= (((int64)value) & maskForNumBits64[numBits] ) << writeBit;
	
	writeBit += numBits;
	
	// Flush 8 bits (1 byte) at a time
	while ( writeBit >= 8 ) {
		writeData[curSize++] = tempValue & 255;
		tempValue >>= 8;
		writeBit -= 8;
	}
	
	// Write leftover now, in case this is the last WriteBits call
	if ( writeBit > 0 ) {
		writeData[curSize] = tempValue & 255;
	}
}

/*
========================
hfBitMsg::WriteString
========================
*/
void hfBitMsg::WriteString( const char * s, int maxLength, bool make7Bit ) {
	if ( !s ) {
		WriteData( "", 1 );
	} else {
		int i, l;
		byte *dataPtr;
		const byte *bytePtr;

		l = hfStr::Length( s );
		if ( maxLength >= 0 && l >= maxLength ) {
			l = maxLength - 1;
		}
		dataPtr = GetByteSpace( l + 1 );
		bytePtr = reinterpret_cast< const byte * >( s );
		if ( make7Bit ) {
			for ( i = 0; i < l; i++ ) {
				if ( bytePtr[i] > 127 ) {
					dataPtr[i] = '.';
				} else {
					dataPtr[i] = bytePtr[i];
				}
			}
		} else {
			for ( i = 0; i < l; i++ ) {
				dataPtr[i] = bytePtr[i];
			}
		}
		dataPtr[i] = '\0';
	}
}

/*
========================
hfBitMsg::WriteData
========================
*/
void hfBitMsg::WriteData( const void *data, int length ) {
	memcpy( GetByteSpace( length ), data, length );
}

/*
========================
hfBitMsg::WriteNetadr
========================
*/
void hfBitMsg::WriteNetadr( const netadr_t adr ) {
	WriteData( adr.ip, 4 );
	WriteUShort( adr.port );
	WriteByte( adr.type );
}

/*
========================
hfBitMsg::WriteDeltaDict
========================
*/
bool hfBitMsg::WriteDeltaDict( const hfDict &dict, const hfDict *base ) {
	int i;
	const hfKeyValue *kv, *basekv;
	bool changed = false;

	if ( base != NULL ) {

		for ( i = 0; i < dict.GetNumKeyVals(); i++ ) {
			kv = dict.GetKeyVal( i );
			basekv = base->FindKey( kv->GetKey() );
			if ( basekv == NULL || basekv->GetValue().Icmp( kv->GetValue() ) != 0 ) {
				WriteString( kv->GetKey() );
				WriteString( kv->GetValue() );
				changed = true;
			}
		}

		WriteString( "" );

		for ( i = 0; i < base->GetNumKeyVals(); i++ ) {
			basekv = base->GetKeyVal( i );
			kv = dict.FindKey( basekv->GetKey() );
			if ( kv == NULL ) {
				WriteString( basekv->GetKey() );
				changed = true;
			}
		}

		WriteString( "" );

	} else {

		for ( i = 0; i < dict.GetNumKeyVals(); i++ ) {
			kv = dict.GetKeyVal( i );
			WriteString( kv->GetKey() );
			WriteString( kv->GetValue() );
			changed = true;
		}
		WriteString( "" );

		WriteString( "" );

	}

	return changed;
}

/*
========================
hfBitMsg::ReadBits

If the number of bits is negative a sign is included.
========================
*/
int hfBitMsg::ReadBits( int numBits ) const {
	int		value;
	int		valueBits;
	int		get;
	int		fraction;
	bool	sgn;

	if ( !readData ) {
		hfLib::FatalError( "hfBitMsg::ReadBits: cannot read from message" );
	}

	// check if the number of bits is valid
	if ( numBits == 0 || numBits < -31 || numBits > 32 ) {
		hfLib::FatalError( "hfBitMsg::ReadBits: bad numBits %i", numBits );
	}

	value = 0;
	valueBits = 0;

	if ( numBits < 0 ) {
		numBits = -numBits;
		sgn = true;
	} else {
		sgn = false;
	}

	// check for overflow
	if ( numBits > GetRemainingReadBits() ) {
		return -1;
	}

	while ( valueBits < numBits ) {
		if ( readBit == 0 ) {
			readCount++;
		}
		get = 8 - readBit;
		if ( get > (numBits - valueBits) ) {
			get = (numBits - valueBits);
		}
		fraction = readData[readCount - 1];
		fraction >>= readBit;
		fraction &= ( 1 << get ) - 1;
		value |= fraction << valueBits;

		valueBits += get;
		readBit = ( readBit + get ) & 7;
	}

	if ( sgn ) {
		if ( value & ( 1 << ( numBits - 1 ) ) ) {
			value |= -1 ^ ( ( 1 << numBits ) - 1 );
		}
	}

	return value;
}

/*
========================
hfBitMsg::ReadString
========================
*/
int hfBitMsg::ReadString( char * buffer, int bufferSize ) const {
	int	l, c;
	
	ReadByteAlign();
	l = 0;
	while( 1 ) {
		c = ReadByte();
		if ( c <= 0 || c >= 255 ) {
			break;
		}
		// translate all fmt spec to avoid crash bugs in string routines
		if ( c == '%' ) {
			c = '.';
		}

		// we will read past any excessively long string, so
		// the following data can be read, but the string will
		// be truncated
		if ( l < bufferSize - 1 ) {
			buffer[l] = c;
			l++;
		}
	}
	
	buffer[l] = 0;
	return l;
}

/*
========================
hfBitMsg::ReadString
========================
*/
int hfBitMsg::ReadString( hfStr & str ) const {
	ReadByteAlign();

	int cnt = 0;
	for ( int i = readCount; i < curSize; i++ ) {
		if ( readData[i] == 0 ) {
			break;
		}
		cnt++;
	}

	str.Clear();
	str.Append( (const char *)readData + readCount, cnt );
	readCount += cnt + 1;

	return str.Length();
}

/*
========================
hfBitMsg::ReadData
========================
*/
int hfBitMsg::ReadData( void *data, int length ) const {
	int cnt;

	ReadByteAlign();
	cnt = readCount;

	if ( readCount + length > curSize ) {
		if ( data ) {
			memcpy( data, readData + readCount, GetRemainingData() );
		}
		readCount = curSize;
	} else {
		if ( data ) {
			memcpy( data, readData + readCount, length );
		}
		readCount += length;
	}

	return ( readCount - cnt );
}

/*
========================
hfBitMsg::ReadNetadr
========================
*/
void hfBitMsg::ReadNetadr( netadr_t *adr ) const {
	ReadData( adr->ip, 4 );
	adr->port = ReadUShort();
	adr->type = ( netadrtype_t ) ReadByte();
}

/*
========================
hfBitMsg::ReadDeltaDict
========================
*/
bool hfBitMsg::ReadDeltaDict( hfDict &dict, const hfDict *base ) const {
	char		key[MAX_STRING_CHARS];
	char		value[MAX_STRING_CHARS];
	bool		changed = false;

	if ( base != NULL ) {
		dict = *base;
	} else {
		dict.Clear();
	}

	while( ReadString( key, sizeof( key ) ) != 0 ) {
		ReadString( value, sizeof( value ) );
		dict.Set( key, value );
		changed = true;
	}

	while( ReadString( key, sizeof( key ) ) != 0 ) {
		dict.Delete( key );
		changed = true;
	}

	return changed;
}

/*
========================
hfBitMsg::DirToBits
========================
*/
int hfBitMsg::DirToBits( const hfVec3 &dir, int numBits ) {
	int max, bits;
	float bias;

	assert( numBits >= 6 && numBits <= 32 );
	assert( dir.LengthSqr() - 1.0f < 0.01f );

	numBits /= 3;
	max = ( 1 << ( numBits - 1 ) ) - 1;
	bias = 0.5f / max;

	bits = IEEE_FLT_SIGNBITSET( dir.x ) << ( numBits * 3 - 1 );
	bits |= ( hfMath::Ftoi( ( hfMath::Fabs( dir.x ) + bias ) * max ) ) << ( numBits * 2 );
	bits |= IEEE_FLT_SIGNBITSET( dir.y ) << ( numBits * 2 - 1 );
	bits |= ( hfMath::Ftoi( ( hfMath::Fabs( dir.y ) + bias ) * max ) ) << ( numBits * 1 );
	bits |= IEEE_FLT_SIGNBITSET( dir.z ) << ( numBits * 1 - 1 );
	bits |= ( hfMath::Ftoi( ( hfMath::Fabs( dir.z ) + bias ) * max ) ) << ( numBits * 0 );
	return bits;
}

/*
========================
hfBitMsg::BitsToDir
========================
*/
hfVec3 hfBitMsg::BitsToDir( int bits, int numBits ) {
	static float sign[2] = { 1.0f, -1.0f };
	int max;
	float invMax;
	hfVec3 dir;

	assert( numBits >= 6 && numBits <= 32 );

	numBits /= 3;
	max = ( 1 << ( numBits - 1 ) ) - 1;
	invMax = 1.0f / max;

	dir.x = sign[( bits >> ( numBits * 3 - 1 ) ) & 1] * ( ( bits >> ( numBits * 2 ) ) & max ) 
					* invMax;

	dir.y = sign[( bits >> ( numBits * 2 - 1 ) ) & 1] * ( ( bits >> ( numBits * 1 ) ) & max ) 
					* invMax;

	dir.z = sign[( bits >> ( numBits * 1 - 1 ) ) & 1] * ( ( bits >> ( numBits * 0 ) ) & max ) 
					* invMax;

	dir.NormalizeFast();
	return dir;
}

