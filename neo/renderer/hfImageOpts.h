#ifndef __IMAGEOPTS_H__
#define __IMAGEOPTS_H__

enum textureType_t {
	TT_DISABLED,
	TT_2D,
	TT_CUBIC
};

/*
================================================
The internal *Texture Format Types*, ::textureFormat_t, are:
================================================
*/
enum textureFormat_t {
	FMT_NONE,

	//------------------------
	// Standard color image formats
	//------------------------

	FMT_RGBA8,			// 32 bpp
	FMT_XRGB8,			// 32 bpp

	//------------------------
	// Alpha channel only
	//------------------------

	// Alpha ends up being the same as L8A8 in our current implementation, because straight 
	// alpha gives 0 for color, but we want 1.
	FMT_ALPHA,		

	//------------------------
	// Luminance replicates the value across RGB with a constant A of 255
	// Intensity replicates the value across RGBA
	//------------------------

	FMT_L8A8,			// 16 bpp
	FMT_LUM8,			//  8 bpp
	FMT_INT8,			//  8 bpp

	//------------------------
	// Compressed texture formats
	//------------------------

	FMT_DXT1,			// 4 bpp
	FMT_DXT5,			// 8 bpp

	//------------------------
	// Depth buffer formats
	//------------------------

	FMT_DEPTH,			// 24 bpp

	//------------------------
	//
	//------------------------

	FMT_X16,			// 16 bpp
	FMT_Y16_X16,		// 32 bpp
	FMT_RGB565,			// 16 bpp
};

int BitsForFormat( textureFormat_t format );

/*
================================================
DXT5 color formats
================================================
*/
enum textureColor_t {
	CFM_DEFAULT,			// RGBA
	CFM_NORMAL_DXT5,		// XY format and use the fast DXT5 compressor
	CFM_YCOCG_DXT5,			// convert RGBA to CoCg_Y format
	CFM_GREEN_ALPHA			// Copy the alpha channel to green
};

/*
================================================
hfImageOpts hold parameters for texture operations.
================================================
*/
class hfImageOpts {
public:
	hfImageOpts();

	bool	operator==( const hfImageOpts & opts );

	//---------------------------------------------------
	// these determine the physical memory size and layout
	//---------------------------------------------------

	textureType_t		textureType;
	textureFormat_t		format;
	textureColor_t		colorFormat;
	int					width;
	int					height;			// not needed for cube maps
	int					numLevels;		// if 0, will be 1 for NEAREST / LINEAR filters, otherwise based on size
	bool				gammaMips;		// if true, mips will be generated with gamma correction
	bool				readback;		// 360 specific - cpu reads back from this texture, so allocate with cached memory
};

/*
========================
hfImageOpts::hfImageOpts
========================
*/
HF_INLINE hfImageOpts::hfImageOpts() {
	format			= FMT_NONE;
	colorFormat		= CFM_DEFAULT;
	width			= 0;
	height			= 0;
	numLevels		= 0;
	textureType		= TT_2D;
	gammaMips		= false;
	readback		= false;

};

/*
========================
hfImageOpts::operator==
========================
*/
HF_INLINE bool hfImageOpts::operator==( const hfImageOpts & opts ) {
	return ( memcmp( this, &opts, sizeof( *this ) ) == 0 );
}

#endif
