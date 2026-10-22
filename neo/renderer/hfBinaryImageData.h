#ifndef __BINARYIMAGEDATA_H__
#define __BINARYIMAGEDATA_H__

/*
================================================================================================

This is where the Binary image headers go that are also included by external tools such as the cloud.

================================================================================================
*/

// These structures are used for memory mapping bimage files, but
// not for the normal loading, so be careful making changes.
// Values are big endien to reduce effort on consoles.
#define BIMAGE_VERSION 10
#define BIMAGE_MAGIC (unsigned int)( ('B'<<0)|('I'<<8)|('M'<<16)|(BIMAGE_VERSION<<24) )

struct bimageImage_t {
	int		level;
	int		destZ;
	int		width;
	int		height;
	int		dataSize;
	// dataSize bytes follow
};

#pragma pack( push, 1 )
struct bimageFile_t {
	HF_TIME_T	sourceFileTime;
	int		headerMagic;
	int		textureType;
	int		format;
	int		colorFormat;
	int		width;
	int		height;
	int		numLevels;
	// one or more bimageImage_t structures follow
};
#pragma pack( pop )

#endif // __BINARYIMAGEDATA_H__

