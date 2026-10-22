#ifndef __COMPRESSOR_H__
#define __COMPRESSOR_H__

/*
===============================================================================

	hfCompressor is a layer ontop of hfFile which provides lossless data
	compression. The compressor can be used as a regular file and multiple
	compressors can be stacked ontop of each other.

===============================================================================
*/

class hfCompressor : public hfFile {
public:
							// compressor allocation
	static hfCompressor *	AllocNoCompression();
	static hfCompressor *	AllocBitStream();
	static hfCompressor *	AllocRunLength();
	static hfCompressor *	AllocRunLength_ZeroBased();
	static hfCompressor *	AllocHuffman();
	static hfCompressor *	AllocArithmetic();
	static hfCompressor *	AllocLZSS();
	static hfCompressor *	AllocLZSS_WordAligned();
	static hfCompressor *	AllocLZW();

							// initialization
	virtual void			Init( hfFile *f, bool compress, int wordLength ) = 0;
	virtual void			FinishCompress() = 0;
	virtual float			GetCompressionRatio() const = 0;

							// common hfFile interface
	virtual const char *	GetName() = 0;
	virtual const char *	GetFullPath() = 0;
	virtual int				Read( void *outData, int outLength ) = 0;
	virtual int				Write( const void *inData, int inLength ) = 0;
	virtual int				Length() = 0;
	virtual HF_TIME_T			Timestamp() = 0;
	virtual int				Tell() = 0;
	virtual void			ForceFlush() = 0;
	virtual void			Flush() = 0;
	virtual int				Seek( long offset, fsOrigin_t origin ) = 0;
};

#endif /* !__COMPRESSOR_H__ */

