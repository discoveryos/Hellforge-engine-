#ifndef __RENDERTEXTURE_H__
#define __RENDERTEXTURE_H__

/*
================================================================================================

	Render Texture

================================================================================================
*/

#define CELL_GCM_INVALID_PITCH		64

/*
================================================
hfRenderTexture holds both the color and depth images that are made
resident on the video hardware.
================================================
*/
class hfRenderTexture {
public:
							hfRenderTexture();
							~hfRenderTexture();

	HF_INLINE int			GetWidth() const { return ( colorImage != NULL ) ? colorImage->GetUploadWidth() : depthImage->GetUploadWidth(); }
	HF_INLINE int			GetHeight() const { return ( colorImage != NULL ) ? colorImage->GetUploadHeight() : depthImage->GetUploadHeight(); }

	HF_INLINE hfImage *		GetColorImage() const { return colorImage; }
	HF_INLINE hfImage *		GetDepthImage() const { return depthImage; }


	void					Resize( int width, int height );

	void					MakeCurrent( int level = 0, int side = 0 );

private:
	hfImage *			colorImage;
	hfImage *			depthImage;
	int					targetWidth;
	int					targetHeight;

};

#endif //!__RENDERTEXTURE_H__

