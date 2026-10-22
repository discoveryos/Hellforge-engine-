#ifndef __SWF_BITSTREAM_H__
#define __SWF_BITSTREAM_H__

class hfSWFBitStream {
public:
					hfSWFBitStream();
					hfSWFBitStream( const byte * data, uint32 len, bool copy ) { free = false; Load( data, len, copy ); }
					~hfSWFBitStream() { Free(); }

	hfSWFBitStream & operator=( hfSWFBitStream & other );

	void			Load( const byte * data, uint32 len, bool copy );
	void			Free();
	const byte *	Ptr() { return startp; }

	uint32			Length() const { return (uint32)( endp - startp ); }
	uint32			Tell() const { return (uint32)( readp - startp ); }
	void			Seek( int32 offset ) { readp += offset; }
	void			Rewind() { readp = startp; }

	void			ResetBits();

	int				ReadS( unsigned int numBits );
	unsigned int	ReadU( unsigned int numBits );
	bool			ReadBool();

	const byte *	ReadData( int size );

	template< typename T >
	void			ReadLittle( T & val );

	uint8			ReadU8();
	uint16			ReadU16();
	uint32			ReadU32();
	int16			ReadS16();
	int32			ReadS32();
	uint32			ReadEncodedU32();
	float			ReadFixed8();
	float			ReadFixed16();
	float			ReadFloat();
	double			ReadDouble();
	const char *	ReadString();

	void			ReadRect( swfRect_t & rect );
	void			ReadMatrix( swfMatrix_t & matrix );
	void			ReadColorXFormRGBA( swfColorXform_t & cxf );
	void			ReadColorRGB( swfColorRGB_t & color );
	void			ReadColorRGBA( swfColorRGBA_t & color );
	void			ReadGradient( swfGradient_t & grad, bool rgba );
	void			ReadMorphGradient( swfGradient_t & grad );

private:
	bool			free;

	const byte *	startp;
	const byte *	endp;
	const byte *	readp;

	uint64			currentBit;
	uint64			currentByte;

	int				ReadInternalS( uint64 & regCurrentBit, uint64 & regCurrentByte, unsigned int numBits );
	unsigned int	ReadInternalU( uint64 & regCurrentBit, uint64 & regCurrentByte, unsigned int numBits );
};

/*
========================
hfSWFBitStream::ResetBits
========================
*/
HF_INLINE void hfSWFBitStream::ResetBits() {
	currentBit = 0;
	currentByte = 0;
}

/*
========================
hfSWFBitStream::ReadLittle
========================
*/
template< typename T >
void hfSWFBitStream::ReadLittle( T & val ) {
	val = *(T *)ReadData( sizeof( val ) );
	hfSwap::Little( val );
}

/*
========================
Wrappers for the most basic types
========================
*/
HF_INLINE bool   hfSWFBitStream::ReadBool() { return ( ReadU( 1 ) != 0 ); }
HF_INLINE uint8  hfSWFBitStream::ReadU8() { ResetBits(); return *readp++; }
HF_INLINE uint16 hfSWFBitStream::ReadU16() { ResetBits(); readp += 2; return ( readp[-2] | ( readp[-1] << 8 ) ); }
HF_INLINE uint32 hfSWFBitStream::ReadU32() { ResetBits(); readp += 4; return ( readp[-4] | ( readp[-3] << 8 ) | ( readp[-2] << 16 ) | ( readp[-1] << 24 ) ); }
HF_INLINE int16  hfSWFBitStream::ReadS16() { ResetBits(); readp += 2; return ( readp[-2] | ( readp[-1] << 8 ) ); }
HF_INLINE int32  hfSWFBitStream::ReadS32() { ResetBits(); readp += 4; return ( readp[-4] | ( readp[-3] << 8 ) | ( readp[-2] << 16 ) | ( readp[-1] << 24 ) ); }
HF_INLINE float  hfSWFBitStream::ReadFixed8() { ResetBits(); readp += 2; return SWFFIXED8( ( readp[-2] | ( readp[-1] << 8 ) ) ); }
HF_INLINE float  hfSWFBitStream::ReadFixed16() { ResetBits(); readp += 4; return SWFFIXED16( ( readp[-4] | ( readp[-3] << 8 ) | ( readp[-2] << 16 ) | ( readp[-1] << 24 ) ) ); }
HF_INLINE float  hfSWFBitStream::ReadFloat() { ResetBits(); readp += 4; uint32 i = ( readp[-4] | ( readp[-3] << 8 ) | ( readp[-2] << 16 ) | ( readp[-1] << 24 ) ); return (float &)i; }

HF_INLINE double hfSWFBitStream::ReadDouble() {
	const byte * swfIsRetarded = ReadData( 8 );
	byte buffer[8];
	buffer[0] = swfIsRetarded[4];
	buffer[1] = swfIsRetarded[5];
	buffer[2] = swfIsRetarded[6];
	buffer[3] = swfIsRetarded[7];
	buffer[4] = swfIsRetarded[0];
	buffer[5] = swfIsRetarded[1];
	buffer[6] = swfIsRetarded[2];
	buffer[7] = swfIsRetarded[3];
	double d = *(double *)buffer;
	hfSwap::Little( d );
	return d;
}

#endif // !__SWF_BITSTREAM_H__

