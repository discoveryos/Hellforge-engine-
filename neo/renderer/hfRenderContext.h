#ifndef __RENDERCONTEXT_H__
#define __RENDERCONTEXT_H__

// This is for "official" HDMI 3D support with with the left eye above the right and a guard band in the middle
// Some displays which don't support this can still do stereo 3D by packing 2 eyes into a single (mono-sized) buffer
enum hdmi3DState_t {
	HDMI3D_NOT_SUPPORTED,	// The TV doesn't support it
	HDMI3D_NOT_ENABLED,		// The TV supports it, but the user disabled it
	HDMI3D_NOT_ACTIVE,		// The TV supports it, and the user enabled it, but it's not active
	HDMI3D_ACTIVE
};


//================================================================================================
// class hfRenderContext 
//================================================================================================
class hfRenderContext {
public:
					hfRenderContext() : depthHackValue( 0.0f ), weaponDepthHackValue( 1.0f ) {}
	virtual			~hfRenderContext() {};

	void			InitContext();
	void			Shutdown();
	void			PrepareSwap();
	void			SwapBuffers( bool forceVsync = false );
	void			SetGamma( unsigned short red[256], unsigned short green[256], unsigned short blue[256] );

	hdmi3DState_t	GetHDMI3DState();
	void			EnableHDMI3D( const bool enable );

	// Back End Rendering
	void			ExecuteBackendCommands( const emptyCommand_t *cmds );
	void			Clear( float r, float g, float b, float a );
	void			InitGraphicsAPIWrapper();

	// Debug Tools
	void			RenderDebugTools( drawSurf_t **drawSurfs, int numDrawSurfs );

	void			SetWrapperContext( const wrapperContext_t & context );
	void			SetWrapperConfig( const wrapperConfig_t & config );

	// Texture Resolves
	void			ResolveTargetColor( hfImage* image );
	void			ResolveTargetDepth( hfImage* image );
	void			ResolveTargetColor( hfImage* image, int srcMinX, int srcMinY, int srcMaxX, int srcMaxY, int dstX, int dstY );
	void			ResolveTargetDepth( hfImage* image, int srcMinX, int srcMinY, int srcMaxX, int srcMaxY, int dstX, int dstY );

	void			SetDepthHackValue( float depth ) { depthHackValue = depth; }
	float			GetDepthHackValue() const { return depthHackValue; }
	void			SetWeaponDepthHackValue( float depth ) { weaponDepthHackValue = depth; }
	float			GetWeaponDepthHackValue() const { return weaponDepthHackValue; }

	uint64			GetGPUFrameMicroSec() const { return GPUFrameMicroSec; }
	
private:
	float			depthHackValue;
	float			weaponDepthHackValue;
	uint64			GPUFrameMicroSec;
};

extern hfRenderContext rRenderContext;

#endif	// !__RENDERCONTEXT_H__

