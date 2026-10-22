#pragma hdrstop
#include "../hflib/hfprecompiled.h"

/*
================================================================================================
Contains the WaveFile implementation.
================================================================================================
*/

#include "hfWaveFile.h"

/*
========================
hfWaveFile::Open

Returns true if the Open was successful and the file matches the expected format. If this 
returns false, there is no need to call Close.
========================
*/
bool hfWaveFile::Open( const char * filename ) {
	Close();

	if ( filename == NULL || filename[0] == 0 ) {
		return false;
	}

	if ( file == NULL ) {
		file = fileSystem->OpenFileReadMemory( filename );
		if ( file == NULL ) {
			return false;
		}
	}

	if ( file->Length() == 0 ) {
		Close();
		return false;
	}

	struct header_t {
		uint32 id;
		uint32 size;
		uint32 format;
	} header;

	file->Read( &header, sizeof( header ) );
	hfSwap::Big( header.id );
	hfSwap::Little( header.size );
	hfSwap::Big( header.format );

	if ( header.id != 'RIFF' || header.format != 'WAVE' || header.size < 4 ) {
		Close();
		hfLib::Warning( "Header is not RIFF WAVE in %s", filename );
		return false;
	}

	uint32 riffSize = header.size + 8;
	uint32 offset = sizeof( header );

	// Scan the file collecting chunks
	while ( offset < riffSize ) {
		struct chuckHeader_t {
			uint32 id;
			uint32 size;
		} chunkHeader;
		if ( file->Read( &chunkHeader, sizeof( chunkHeader ) ) != sizeof( chunkHeader ) ) {
			// It seems like some tools have extra data after the last chunk for no apparent reason
			// so don't treat this as an error
			return true;
		}
		hfSwap::Big( chunkHeader.id );
		hfSwap::Little( chunkHeader.size );
		offset += sizeof( chunkHeader );

		if ( chunks.Num() >= chunks.Max() ) {
			Close();
			hfLib::Warning( "More than %d chunks in %s", chunks.Max(), filename );
			return false;
		}

		chunk_t * chunk = chunks.Alloc();
		chunk->id = chunkHeader.id;
		chunk->size = chunkHeader.size;
		chunk->offset = offset;
		offset += chunk->size;

		file->Seek( offset, FS_SEEK_SET );
	}

	return true;
}

/*
========================
hfWaveFile::SeekToChunk

Seeks to the specified chunk and returns the size of the chunk or 0 if the chunk wasn't found.
========================
*/
uint32 hfWaveFile::SeekToChunk( uint32 id ) {
	for ( int i = 0; i < chunks.Num(); i++ ) {
		if ( chunks[i].id == id ) {
			file->Seek( chunks[i].offset, FS_SEEK_SET );
			return chunks[i].size;
		}
	}
	return 0;
}

/*
========================
hfWaveFile::GetChunkOffset

Seeks to the specified chunk and returns the size of the chunk or 0 if the chunk wasn't found.
========================
*/
uint32 hfWaveFile::GetChunkOffset( uint32 id ) {
	for ( int i = 0; i < chunks.Num(); i++ ) {
		if ( chunks[i].id == id ) {
			return chunks[i].offset;
		}
	}
	return 0;
}

// Used in XMA2WAVEFORMAT for per-stream data
typedef struct XMA2STREAMFORMAT {
    byte Channels;			// Number of channels in the stream (1 or 2)
    byte RESERVED;			// Reserved for future use
    uint16 ChannelMask;		// Spatial positions of the channels in the stream
} XMA2STREAMFORMAT;

// Legacy XMA2 format structure (big-endian byte ordering)
typedef struct XMA2WAVEFORMAT {
    byte Version;			// XMA encoder version that generated the file.
							// Always 3 or higher for XMA2 files.
    byte NumStreams;		// Number of interleaved audio streams
    byte RESERVED;			// Reserved for future use
    byte LoopCount;			// Number of loop repetitions; 255 = infinite
    uint32 LoopBegin;		// Loop begin point, in samples
    uint32 LoopEnd;			// Loop end point, in samples
    uint32 SampleRate;		// The file's decoded sample rate
    uint32 EncodeOptions;		// Options for the XMA encoder/decoder
    uint32 PsuedoBytesPerSec;	// Used internally by the XMA encoder
    uint32 BlockSizeInBytes;	// Size in bytes of this file's XMA blocks (except
								// possibly the last one).  Always a multiple of
								// 2Kb, since XMA blocks are arrays of 2Kb packets.
    uint32 SamplesEncoded;		// Total number of PCM samples encoded in this file
    uint32 SamplesInSource;		// Actual number of PCM samples in the source
								// material used to generate this file
    uint32 BlockCount;			// Number of XMA blocks in this file (and hence
								// also the number of entries in its seek table)
} XMA2WAVEFORMAT;

/*
========================
hfWaveFile::ReadWaveFormat

Reads a wave format header, returns NULL if it found one and read it.
otherwise, returns a human-readable error message.
========================
*/
const char * hfWaveFile::ReadWaveFormat( waveFmt_t & format ) {
	memset( &format, 0, sizeof( format ) );

	uint32 formatSize = SeekToChunk( waveFmt_t::id );
	if ( formatSize == 0 ) {
		return "No format chunk";
	}
	if ( formatSize < sizeof( format.basic ) ) {
		return "Format chunk too small";
	}

	Read( &format.basic, sizeof( format.basic ) );

	hfSwapClass<waveFmt_t::basic_t> swap;
	swap.Little( format.basic.formatTag );
	swap.Little( format.basic.numChannels );
	swap.Little( format.basic.samplesPerSec );
	swap.Little( format.basic.avgBytesPerSec );
	swap.Little( format.basic.blockSize );
	swap.Little( format.basic.bitsPerSample );

	if ( format.basic.formatTag == FORMAT_PCM ) {
	} else if ( format.basic.formatTag == FORMAT_ADPCM ) {
		Read( &format.extraSize, sizeof( format.extraSize ) );
		hfSwap::Little( format.extraSize );
		if ( format.extraSize != sizeof( waveFmt_t::extra_t::adpcm_t ) ) {
			return "Incorrect number of coefficients in ADPCM file";
		}
		Read( &format.extra.adpcm, sizeof( format.extra.adpcm ) );
		hfSwapClass<waveFmt_t::extra_t::adpcm_t> swap;
		swap.Little( format.extra.adpcm.samplesPerBlock );
		swap.Little( format.extra.adpcm.numCoef );
		for ( int i = 0; i < format.extra.adpcm.numCoef; i++ ) {
			swap.Little( format.extra.adpcm.aCoef[ i ].coef1 );
			swap.Little( format.extra.adpcm.aCoef[ i ].coef2 );
		}
	} else if ( format.basic.formatTag == FORMAT_XMA2 ) {
		Read( &format.extraSize, sizeof( format.extraSize ) );
		hfSwap::Little( format.extraSize );
		if ( format.extraSize != sizeof( waveFmt_t::extra_t::xma2_t ) ) {
			return "Incorrect chunk size in XMA2 file";
		}
		Read( &format.extra.xma2, sizeof( format.extra.xma2 ) );
		hfSwapClass<waveFmt_t::extra_t::xma2_t> swap;
		swap.Little( format.extra.xma2.numStreams );
		swap.Little( format.extra.xma2.channelMask );
		swap.Little( format.extra.xma2.samplesEncoded );
		swap.Little( format.extra.xma2.bytesPerBlock );
		swap.Little( format.extra.xma2.playBegin );
		swap.Little( format.extra.xma2.playLength );
		swap.Little( format.extra.xma2.loopBegin );
		swap.Little( format.extra.xma2.loopLength );
		swap.Little( format.extra.xma2.loopCount );
		swap.Little( format.extra.xma2.encoderVersion );
		swap.Little( format.extra.xma2.blockCount );
	} else if ( format.basic.formatTag == FORMAT_EXTENSIBLE ) {
		Read( &format.extraSize, sizeof( format.extraSize ) );
		hfSwap::Little( format.extraSize );
		if ( format.extraSize != sizeof( waveFmt_t::extra_t::extensible_t ) ) {
			return "Incorrect chunk size in extensible wave file";
		}
		Read( &format.extra.extensible, sizeof( format.extra.extensible ) );
		hfSwapClass<waveFmt_t::extra_t::extensible_t> swap;
		swap.Little( format.extra.extensible.validBitsPerSample );
		swap.Little( format.extra.extensible.channelMask );
		swap.Little( format.extra.extensible.subFormat.data1 );
		swap.Little( format.extra.extensible.subFormat.data2 );
		swap.Little( format.extra.extensible.subFormat.data3 );
		swap.Little( format.extra.extensible.subFormat.data4 );
		swap.LittleArray( format.extra.extensible.subFormat.data5, 6 );
		waveFmt_t::extra_t::extensible_t::guid_t pcmGuid = {
			FORMAT_PCM,
			0x0000,
			0x0010,
			0x8000,
			{ 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 }
		};
		if ( memcmp( &pcmGuid, &format.extra.extensible.subFormat, sizeof( pcmGuid ) ) != 0 ) {
			return "Unsupported Extensible format";
		}
	} else {
		return "Unknown wave format tag";
	}

	return NULL;
}

/*
========================
hfWaveFile::ReadWaveFormatDirect

Reads a wave format header from a file ptr, 
========================
*/
bool hfWaveFile::ReadWaveFormatDirect( waveFmt_t & format, hfFile *file ) {
	
	file->Read( &format.basic, sizeof( format.basic ) );
	hfSwapClass<waveFmt_t::basic_t> swap;
	swap.Little( format.basic.formatTag );
	swap.Little( format.basic.numChannels );
	swap.Little( format.basic.samplesPerSec );
	swap.Little( format.basic.avgBytesPerSec );
	swap.Little( format.basic.blockSize );
	swap.Little( format.basic.bitsPerSample );

	if ( format.basic.formatTag == FORMAT_PCM ) {
	} else if ( format.basic.formatTag == FORMAT_ADPCM ) {
		file->Read( &format.extraSize, sizeof( format.extraSize ) );
		hfSwap::Little( format.extraSize );
		if ( format.extraSize != sizeof( waveFmt_t::extra_t::adpcm_t ) ) {
			return false;
		}
		file->Read( &format.extra.adpcm, sizeof( format.extra.adpcm ) );
		hfSwapClass<waveFmt_t::extra_t::adpcm_t> swap;
		swap.Little( format.extra.adpcm.samplesPerBlock );
		swap.Little( format.extra.adpcm.numCoef );
		for ( int i = 0; i < format.extra.adpcm.numCoef; i++ ) {
			swap.Little( format.extra.adpcm.aCoef[ i ].coef1 );
			swap.Little( format.extra.adpcm.aCoef[ i ].coef2 );
		}
	} else if ( format.basic.formatTag == FORMAT_XMA2 ) {
		file->Read( &format.extraSize, sizeof( format.extraSize ) );
		hfSwap::Little( format.extraSize );
		if ( format.extraSize != sizeof( waveFmt_t::extra_t::xma2_t ) ) {
			return false;
		}
		file->Read( &format.extra.xma2, sizeof( format.extra.xma2 ) );
		hfSwapClass<waveFmt_t::extra_t::xma2_t> swap;
		swap.Little( format.extra.xma2.numStreams );
		swap.Little( format.extra.xma2.channelMask );
		swap.Little( format.extra.xma2.samplesEncoded );
		swap.Little( format.extra.xma2.bytesPerBlock );
		swap.Little( format.extra.xma2.playBegin );
		swap.Little( format.extra.xma2.playLength );
		swap.Little( format.extra.xma2.loopBegin );
		swap.Little( format.extra.xma2.loopLength );
		swap.Little( format.extra.xma2.loopCount );
		swap.Little( format.extra.xma2.encoderVersion );
		swap.Little( format.extra.xma2.blockCount );
	} else if ( format.basic.formatTag == FORMAT_EXTENSIBLE ) {
		file->Read( &format.extraSize, sizeof( format.extraSize ) );
		hfSwap::Little( format.extraSize );
		if ( format.extraSize != sizeof( waveFmt_t::extra_t::extensible_t ) ) {
			return false;
		}
		file->Read( &format.extra.extensible, sizeof( format.extra.extensible ) );
		hfSwapClass<waveFmt_t::extra_t::extensible_t> swap;
		swap.Little( format.extra.extensible.validBitsPerSample );
		swap.Little( format.extra.extensible.channelMask );
		swap.Little( format.extra.extensible.subFormat.data1 );
		swap.Little( format.extra.extensible.subFormat.data2 );
		swap.Little( format.extra.extensible.subFormat.data3 );
		swap.Little( format.extra.extensible.subFormat.data4 );
		swap.LittleArray( format.extra.extensible.subFormat.data5, 6 );
		waveFmt_t::extra_t::extensible_t::guid_t pcmGuid = {
			FORMAT_PCM,
			0x0000,
			0x0010,
			0x8000,
			{ 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 }
		};
		if ( memcmp( &pcmGuid, &format.extra.extensible.subFormat, sizeof( pcmGuid ) ) != 0 ) {
			return false;
		}
	} else {
		return false;
	}

	return true;
}

/*
========================
hfWaveFile::WriteWaveFormatDirect

Writes a wave format header to a file ptr, 
========================
*/
bool hfWaveFile::WriteWaveFormatDirect( waveFmt_t & format, hfFile *file ) {
	//hfSwapClass<waveFmt_t::basic_t> swap;
	//swap.Little( format.basic.formatTag );
	//swap.Little( format.basic.numChannels );
	//swap.Little( format.basic.samplesPerSec );
	//swap.Little( format.basic.avgBytesPerSec );
	//swap.Little( format.basic.blockSize );
	//swap.Little( format.basic.bitsPerSample );
	file->Write( &format.basic, sizeof( format.basic ) );
	if ( format.basic.formatTag == FORMAT_PCM ) {
		//file->Write( &format.basic, sizeof( format.basic ) );	
	} else if ( format.basic.formatTag == FORMAT_ADPCM ) {
		//file->Write( &format.basic, sizeof( format.basic ) );
		file->Write( &format.extraSize, sizeof( format.extraSize ) );
		file->Write( &format.extra.adpcm, sizeof( format.extra.adpcm ) );
	} else if ( format.basic.formatTag == FORMAT_XMA2 ) {
		//file->Write( &format.basic, sizeof( format.basic ) );
		file->Write( &format.extraSize, sizeof( format.extraSize ) );
		file->Write( &format.extra.xma2, sizeof( format.extra.xma2 ) );
	} else if ( format.basic.formatTag == FORMAT_EXTENSIBLE ) {
		//file->Write( &format.basic, sizeof( format.basic ) );
		file->Write( &format.extraSize, sizeof( format.extraSize ) );
		file->Write( &format.extra.extensible, sizeof( format.extra.extensible ) );
	} else {
		return false;
	}
	return true;
}

/*
========================
hfWaveFile::WriteWaveFormatDirect

Writes a wave format header to a file ptr, 
========================
*/

bool hfWaveFile::WriteSampleDataDirect( hfList< sampleData_t > & sampleData, hfFile * file ) {
	static const uint32 sample = 'smpl';
	file->WriteBig( sample );
	uint32 samplerData = sampleData.Num() * 24;
	uint32 chunkSize = 36 + samplerData;
	uint32 zero = 0;
	uint32 numSamples = sampleData.Num();

	file->Write( &chunkSize, sizeof( uint32 ) );
	file->Write( &zero, sizeof( uint32 ) );
	file->Write( &zero, sizeof( uint32 ) );
	file->Write( &zero, sizeof( uint32 ) );
	file->Write( &zero, sizeof( uint32 ) );
	file->Write( &zero, sizeof( uint32 ) );
	file->Write( &zero, sizeof( uint32 ) );
	file->Write( &zero, sizeof( uint32 ) );
	file->Write( &numSamples, sizeof( uint32 ) );
	file->Write( &samplerData, sizeof( uint32 ) );

	for ( int i = 0; i < sampleData.Num(); ++i ) {
		file->Write( &zero, sizeof( uint32 ) );
		file->Write( &zero, sizeof( uint32 ) );
		file->Write( &sampleData[ i ].start, sizeof( uint32 ) );
		file->Write( &sampleData[ i ].end, sizeof( uint32 ) );
		file->Write( &zero, sizeof( uint32 ) );
		file->Write( &zero, sizeof( uint32 ) );
	}
	return true;
}

/*
========================
hfWaveFile::WriteWaveFormatDirect

Writes a data chunk to a file ptr
========================
*/

bool hfWaveFile::WriteDataDirect( char * _data, uint32 size, hfFile * file ) {
	static const uint32 data = 'data';
	file->WriteBig( data );
	file->Write( &size, sizeof( uint32 ) );
	file->WriteBigArray( _data, size );
	return true;
}

/*
========================
hfWaveFile::WriteWaveFormatDirect

Writes a wave header to a file ptr, 
========================
*/

bool hfWaveFile::WriteHeaderDirect( uint32 fileSize, hfFile * file ) {
	static const uint32 riff = 'RIFF';
	static const uint32 wave = 'WAVE';
	file->WriteBig( riff );
	file->WriteBig( fileSize );
	file->WriteBig( wave );
	return true;
}

/*
========================
hfWaveFile::ReadLoopPoint

Reads a loop point from a 'smpl' chunk in a wave file, returns 0 if none are found.
========================
*/
bool hfWaveFile::ReadLoopData( int & start, int & end ) {
	uint32 chunkSize = SeekToChunk( samplerChunk_t::id );
	if ( chunkSize < sizeof( samplerChunk_t ) ) {
		return false;
	}

	samplerChunk_t smpl;
	Read( &smpl, sizeof( smpl ) );
	hfSwap::Little( smpl.numSampleLoops );

	if ( smpl.numSampleLoops < 1 ) {
		return false; // this is possible returning false lets us know there are more then 1 sample look in the file and is not appropriate for traditional looping
	}

	sampleData_t smplData;
	Read( &smplData, sizeof( smplData ) );
	hfSwap::Little( smplData.start );
	hfSwap::Little( smplData.end );

	if ( smplData.type != 0 ) {
		hfLib::Warning( "Invalid loop type in %s", file->GetName() );
		return false;
	}

	start = smplData.start;
	end = smplData.end;
	return true;
}

/*
========================
hfWaveFile::Close

Closes the file and frees resources.
========================
*/
void hfWaveFile::Close() { 
	if ( file != NULL ) {
		delete file;
		file = NULL;
	}
	chunks.SetNum( 0 );
}

