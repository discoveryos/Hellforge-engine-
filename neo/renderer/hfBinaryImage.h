#ifndef __BINARYIMAGE_H__
#define __BINARYIMAGE_H__

#include "hfBinaryImageData.h"

/*
================================================
hfBinaryImage is used by the hfImage class for constructing mipmapped 
textures and for loading and saving generated files by hfImage.
Also used in a memory-mapped form for imageCPU for offline megatexture
generation.
================================================
*/
class hfBinaryImage {
public:
	hfBinaryImage( const char * name ) : imgName( name ) { }

	const char *		GetName() const { return imgName.c_str(); }
	void				SetName( const char *_name ) { imgName = _name; }

	void				Load2DFromMemory( int width, int height, const byte * pic_const, int numLevels, textureFormat_t & textureFormat, textureColor_t & colorFormat, bool gammaMips );
	void				LoadCubeFromMemory( int width, const byte * pics[6], int numLevels, textureFormat_t & textureFormat, bool gammaMips );

	HF_TIME_T			LoadFromGeneratedFile( HF_TIME_T sourceFileTime );
	HF_TIME_T			WriteGeneratedFile( HF_TIME_T sourceFileTime );

	const bimageFile_t &	GetFileHeader() { return fileData; }

	int					NumImages() { return images.Num(); }
	const bimageImage_t &	GetImageHeader( int i ) const { return images[i]; }
	const byte *			GetImageData( int i ) const { return images[i].data; }
	static void			GetGeneratedFileName( hfStr & gfn, const char *imageName );
private:
	hfStr				imgName;			// game path, including extension (except for cube maps), may be an image program
	bimageFile_t		fileData;

	class hfBinaryImageData : public bimageImage_t {
	public:
		byte * data;

		hfBinaryImageData() : data( NULL ) { }
		~hfBinaryImageData() { Free(); }
		hfBinaryImageData & operator=( hfBinaryImageData & other ) {
			if ( this == &other ) {
				return *this;
			}

			Alloc( other.dataSize );
			memcpy( data, other.data, other.dataSize );
			return *this;
		}
		void Free() {
			if ( data != NULL ) {
				Mem_Free( data );
				data = NULL;
				dataSize = 0;
			}
		}
		void Alloc( int size ) {
			Free();
			dataSize = size;
			data = (byte *)Mem_Alloc( size, TAG_CRAP );
		}
	};

	hfList< hfBinaryImageData, TAG_IDLIB_LIST_IMAGE > images;

private:
	void				MakeGeneratedFileName( hfStr & gfn );
	bool				LoadFromGeneratedFile( hfFile * f, HF_TIME_T sourceFileTime );
};

#endif // __BINARYIMAGE_H__

