#ifndef __GRAPHICSAPIWRAPPER_H__
#define __GRAPHICSAPIWRAPPER_H__

/*
================================================================================================

	Graphics API wrapper/helper functions

	This wraps platform specific graphics API functionality that is used at run-time. This
	functionality is wrapped to avoid excessive conditional compilation and/or code duplication
	throughout the run-time rendering code that is shared on all platforms.

	Most other graphics API functions are called for initialization purposes and are called
	directly from platform specific code implemented in files in the platform specific folders:

	renderer/OpenGL/
	renderer/DirectX/
	renderer/GCM/

================================================================================================
*/

class hfImage;
//class hfTriangles;
class hfRenderModelSurface;
class hfDeclRenderProg;
class hfRenderTexture;

static const int MAX_OCCLUSION_QUERIES = 4096;
// returned by GL_GetDeferredQueryResult() when the query is from too long ago and the result is no longer available
static const int OCCLUSION_QUERY_TOO_OLD				= -1;

/*
================================================================================================

	Platform Specific Context

================================================================================================
*/




#define USE_CORE_PROFILE

struct wrapperContext_t {
};


/*
================================================
wrapperConfig_t
================================================
*/
struct wrapperConfig_t {
	// rendering options and settings
	bool			disableStateCaching;
	bool			lazyBindPrograms;
	bool			lazyBindParms;
	bool			lazyBindTextures;
	bool			stripFragmentBranches;
	bool			skipDetailTris;
	bool			singleTriangle;
	// values for polygon offset
	float			polyOfsFactor;
	float			polyOfsUnits;
	// global texture filter settings
	int				textureMinFilter;
	int				textureMaxFilter;
	int				textureMipFilter;
	float			textureAnisotropy;
	float			textureLODBias;
};

/*
================================================
wrapperStats_t
================================================
*/
struct wrapperStats_t {
	int				c_queriesIssued;
	int				c_queriesPassed;
	int				c_queriesWaitTime;
	int				c_queriesTooOld;
	int				c_programsBound;
	int				c_drawElements;
	int				c_drawIndices;
	int				c_drawVertices;
};

/*
================================================================================================

	API

================================================================================================
*/

void			GL_SetWrapperContext( const wrapperContext_t & context );
void			GL_SetWrapperConfig( const wrapperConfig_t & config );

void			GL_SetTimeDelta( uint64 delta );	// delta from GPU to CPU microseconds
void			GL_StartFrame( int frame );			// inserts a timing mark for the start of the GPU frame
void			GL_EndFrame();						// inserts a timing mark for the end of the GPU frame
void			GL_WaitForEndFrame();				// wait for the GPU to reach the last end frame marker
void			GL_GetLastFrameTime( uint64 & startGPUTimeMicroSec, uint64 & endGPUTimeMicroSec );	// GPU time between GL_StartFrame() and GL_EndFrame()
void			GL_StartDepthPass( const hfScreenRect & rect );
void			GL_FinishDepthPass();
void			GL_GetDepthPassRect( hfScreenRect & rect );

void			GL_SetDefaultState();
void			GL_State( uint64 stateVector, bool forceGlState = false );
uint64			GL_GetCurrentState();
uint64			GL_GetCurrentStateMinusStencil();
void			GL_Cull( int cullType );
void			GL_Scissor( int x /* left*/, int y /* bottom */, int w, int h );
void			GL_Viewport( int x /* left */, int y /* bottom */, int w, int h );
HF_INLINE void	GL_Scissor( const hfScreenRect & rect ) { GL_Scissor( rect.x1, rect.y1, rect.x2 - rect.x1 + 1, rect.y2 - rect.y1 + 1 ); }
HF_INLINE void	GL_Viewport( const hfScreenRect & rect ) { GL_Viewport( rect.x1, rect.y1, rect.x2 - rect.x1 + 1, rect.y2 - rect.y1 + 1 ); }
HF_INLINE void	GL_ViewportAndScissor( int x, int y, int w, int h ) { GL_Viewport( x, y, w, h ); GL_Scissor( x, y, w, h ); }
HF_INLINE void	GL_ViewportAndScissor( const hfScreenRect& rect ) { GL_Viewport( rect ); GL_Scissor( rect ); }
void			GL_Clear( bool color, bool depth, bool stencil, byte stencilValue, float r, float g, float b, float a );
void			GL_PolygonOffset( float scale, float bias );
void			GL_DepthBoundsTest( const float zmin, const float zmax );
void			GL_Color( float * color );
void			GL_Color( float r, float g, float b );
void			GL_Color( float r, float g, float b, float a );
void			GL_SelectTexture( int unit );

void			GL_Flush();		// flush the GPU command buffer
void			GL_Finish();	// wait for the GPU to have executed all commands
void			GL_CheckErrors();

wrapperStats_t	GL_GetCurrentStats();
void			GL_ClearStats();


#endif // !__GRAPHICSAPIWRAPPER_H__

