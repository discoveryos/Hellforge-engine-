#pragma hdrstop
#include "../hflib/hfprecompiled.h"

#include "hfShadowMapManager.h"

#include "hftr_local.h"
#include "hfGLMatrix.h"
#include "hfRenderProgs.h"
#include "hfCVarSystem.h"
#include "hfCmdArgs.h"
#include "hfCmdSystem.h"

#include <dlfcn.h>

// GL 3.x tokens that postdate the wrapped GL header (mirrors
// hfOpenGLPostprocess.cpp).
#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER					0x8D40
#endif
#ifndef GL_FRAMEBUFFER_COMPLETE
#define GL_FRAMEBUFFER_COMPLETE			0x8CD5
#endif
#ifndef GL_COLOR_ATTACHMENT0
#define GL_COLOR_ATTACHMENT0			0x8CE0
#endif
#ifndef GL_RENDERBUFFER
#define GL_RENDERBUFFER					0x8D41
#endif
#ifndef GL_DEPTH_ATTACHMENT
#define GL_DEPTH_ATTACHMENT				0x8D00
#endif
#ifndef GL_DEPTH_COMPONENT24
#define GL_DEPTH_COMPONENT24			0x81A6
#endif

// -------------------------------------------------------------------------
// cvars
// -------------------------------------------------------------------------
hfCVar r_shadowMapMode( "r_shadowMapMode", "1", CVAR_RENDERER | CVAR_INTEGER, "shadow map capture mode (0 = off/stencil, 1 = capture chips, 2 = verbose)" );
hfCVar r_shadowMapSize( "r_shadowMapSize", "512", CVAR_RENDERER | CVAR_INTEGER, "shadow chip resolution (64..2048)" );
hfCVar r_shadowMapZNear( "r_shadowMapZNear", "4", CVAR_RENDERER | CVAR_FLOAT, "shadow camera near plane" );
hfCVar r_shadowMapFar( "r_shadowMapFar", "512", CVAR_RENDERER | CVAR_FLOAT, "shadow camera far plane" );
hfCVar r_shadowMapFov( "r_shadowMapFov", "90", CVAR_RENDERER | CVAR_FLOAT, "shadow camera FOV (degrees)" );

// -------------------------------------------------------------------------
// FBO entry points resolved dynamically (wrapper ends at GL 3.x)
// -------------------------------------------------------------------------
static PFNGLGENFRAMEBUFFERSPROC psmGenFramebuffers = NULL;
static PFNGLBINDFRAMEBUFFERPROC psmBindFramebuffer = NULL;
static PFNGLDELETEFRAMEBUFFERSPROC psmDeleteFramebuffers = NULL;
static PFNGLGENRENDERBUFFERSPROC psmGenRenderbuffers = NULL;
static PFNGLBINDRENDERBUFFERPROC psmBindRenderbuffer = NULL;
static PFNGLRENDERBUFFERSTORAGEPROC psmRenderbufferStorage = NULL;
static PFNGLFRAMEBUFFERRENDERBUFFERPROC psmFramebufferRenderbuffer = NULL;
static PFNGLFRAMEBUFFERTEXTURE2DPROC psmFramebufferTexture2D = NULL;
static PFNGLCHECKFRAMEBUFFERSTATUSPROC psmCheckFramebufferStatus = NULL;
static PFNGLDELETERENDERBUFFERSPROC psmDeleteRenderbuffers = NULL;

static bool hfShadowMap_ResolveGL() {
#define HF_RESOLVE( var, fn ) var = ( PFNGL##fn##PROC )dlsym( RTLD_DEFAULT, #fn )
	HF_RESOLVE( psmGenFramebuffers, GENFRAMEBUFFERS );
	HF_RESOLVE( psmBindFramebuffer, BINDFRAMEBUFFER );
	HF_RESOLVE( psmDeleteFramebuffers, DELETEFRAMEBUFFERS );
	HF_RESOLVE( psmGenRenderbuffers, GENRENDERBUFFERS );
	HF_RESOLVE( psmBindRenderbuffer, BINDRENDERBUFFER );
	HF_RESOLVE( psmRenderbufferStorage, RENDERBUFFERSTORAGE );
	HF_RESOLVE( psmFramebufferRenderbuffer, FRAMEBUFFERRENDERBUFFER );
	HF_RESOLVE( psmFramebufferTexture2D, FRAMEBUFFERTEXTURE2D );
	HF_RESOLVE( psmCheckFramebufferStatus, CHECKFRAMEBUFFERSTATUS );
	HF_RESOLVE( psmDeleteRenderbuffers, DELETERENDERBUFFERS );
#undef HF_RESOLVE
	return psmGenFramebuffers && psmBindFramebuffer && psmDeleteFramebuffers
		&& psmGenRenderbuffers && psmBindRenderbuffer && psmRenderbufferStorage
		&& psmFramebufferRenderbuffer && psmFramebufferTexture2D
		&& psmCheckFramebufferStatus && psmDeleteRenderbuffers;
}

extern void RB_SetMVP( const hfRenderMatrix & mvp );
extern void RB_DrawElementsWithCounters( const drawSurf_t * surf );

static void hfShadowMapStats_f( const hfCmdArgs & args );

hfShadowMapManager shadowMapManager;

hfShadowMapManager::hfShadowMapManager() {
	m_resolved = false;
	m_initFlags = 0;
	m_size = 512;
	m_capturedCount = 0;
	m_frameCasters = 0;
	m_capturedMicros = 0;
	m_fbo = 0;
	m_colorTex = 0;
	m_depthRBO = 0;
	for ( int i = 0; i < 32; i++ ) { m_lightIds[i] = 0; m_captured[i] = 0; }
}

bool hfShadowMapManager::Supported() {
	if ( !( m_initFlags & 1 ) ) {
		m_initFlags |= 1;
		m_resolved = hfShadowMap_ResolveGL();
		if ( m_resolved ) { m_initFlags |= 2; }
	}
	return ( m_initFlags & 3 ) == 3;
}

void hfShadowMapManager::BeginRender() {
	m_capturedCount = 0;
	m_frameCasters = 0;
	m_capturedMicros = 0;
	for ( int i = 0; i < 32; i++ ) { m_captured[i] = 0; }
	static bool sCmdRegistered = false;
	if ( !sCmdRegistered ) {
		sCmdRegistered = true;
		if ( cmdSystem ) {
			cmdSystem->AddCommand( "shadowMapStats", hfShadowMapStats_f, CMD_FL_RENDERER, "print shadow-map capture statistics" );
		}
	}
}

bool hfShadowMapManager::WasLightCaptured( const void * light ) const {
	unsigned long id = ( unsigned long )light >> 5;
	for ( int i = 0; i < m_capturedCount; i++ ) {
		if ( m_lightIds[i] == id && m_captured[i] ) { return true; }
	}
	return false;
}

bool hfShadowMapManager::CaptureLight( const void * lightCtx ) {
	if ( r_shadowMapMode.GetInteger() == 0 ) { return false; }
	if ( !Supported() ) { return true; }		// no FBO -> leave lights alone
	if ( m_capturedCount >= 32 ) { return false; }
	if ( WasLightCaptured( lightCtx ) ) { return true; }

	const viewLight_t * vLight = ( const viewLight_t * )lightCtx;

	int casterCount = 0;
	for ( const drawSurf_t * d = vLight->globalShadows; d; d = d->nextOnLight ) { casterCount++; }
	for ( const drawSurf_t * d = vLight->localShadows; d; d = d->nextOnLight ) { casterCount++; }
	if ( casterCount == 0 ) { return false; }

	if ( m_fbo == 0 ) {
		m_size = r_shadowMapSize.GetInteger();
		if ( m_size < 64 ) { m_size = 64; }
		if ( m_size > 2048 ) { m_size = 2048; }
		psmGenFramebuffers( 1, &m_fbo );
		psmBindFramebuffer( GL_FRAMEBUFFER, m_fbo );
		qglGenTextures( 1, &m_colorTex );
		qglBindTexture( GL_TEXTURE_2D, m_colorTex );
		qglTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, m_size, m_size, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );
		qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		psmFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTex, 0 );
		psmGenRenderbuffers( 1, &m_depthRBO );
		psmBindRenderbuffer( GL_RENDERBUFFER, m_depthRBO );
		psmRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, m_size, m_size );
		psmFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthRBO );
		if ( psmCheckFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE ) {
			m_initFlags = 1;	// degrade for the rest of the process
			return false;
		}
		psmBindFramebuffer( GL_FRAMEBUFFER, 0 );
	}

	// shadow camera: from light origin looking at the viewer
	const hfVec3 & lightOrg = vLight->globalLightOrigin;
	const hfVec3 & viewOrg = backEnd.viewDef->renderView.vieworg;

	hfVec3 forward = viewOrg - lightOrg;
	if ( forward.LengthSqr() < 1.0f ) { forward[2] = 1.0f; }
	forward.Normalize();
	hfVec3 up( 0.0f, 0.0f, 1.0f );
	hfVec3 right = forward.Cross( up );
	if ( right.LengthSqr() < 0.001f ) { right = hfVec3( 0.0f, 1.0f, 0.0f ); }
	right.Normalize();
	up = right.Cross( forward );

	float viewerMatrix[16];
	viewerMatrix[0*4+0] = forward[0]; viewerMatrix[1*4+0] = forward[1]; viewerMatrix[2*4+0] = forward[2];
	viewerMatrix[3*4+0] = -( lightOrg[0] * forward[0] + lightOrg[1] * forward[1] + lightOrg[2] * forward[2] );
	viewerMatrix[0*4+1] = right[0]; viewerMatrix[1*4+1] = right[1]; viewerMatrix[2*4+1] = right[2];
	viewerMatrix[3*4+1] = -( lightOrg[0] * right[0] + lightOrg[1] * right[1] + lightOrg[2] * right[2] );
	viewerMatrix[0*4+2] = up[0]; viewerMatrix[1*4+2] = up[1]; viewerMatrix[2*4+2] = up[2];
	viewerMatrix[3*4+2] = -( lightOrg[0] * up[0] + lightOrg[1] * up[1] + lightOrg[2] * up[2] );
	viewerMatrix[0*4+3] = 0.0f; viewerMatrix[1*4+3] = 0.0f; viewerMatrix[2*4+3] = 0.0f; viewerMatrix[3*4+3] = 1.0f;

	// convert from our coordinate system (looking down X) to OpenGL (-Z)
	static const float s_flipMatrix[16] = {
		 0, 0, -1, 0,
		-1, 0,  0, 0,
		 0, 1,  0, 0,
		 0, 0,  0, 1
	};
	float modelViewWorld[16];
	R_MatrixMultiply( viewerMatrix, s_flipMatrix, modelViewWorld );

	const float zNear = r_shadowMapZNear.GetFloat();
	const float zFar  = r_shadowMapFar.GetFloat();
	const float fov   = r_shadowMapFov.GetFloat();
	const float halfTan = tanf( fov * hfMath::PI / 360.0f );
	const float xmax = zNear * halfTan;
	const float ymax = zNear * halfTan;
	float proj[16];
	memset( proj, 0, sizeof( proj ) );
	proj[0*4+0] = 2.0f * zNear / ( xmax * 2.0f );
	proj[1*4+1] = 2.0f * zNear / ( ymax * 2.0f );
	proj[2*4+2] = -( zFar + zNear ) / ( zFar - zNear );
	proj[3*4+2] = -( 2.0f * zFar * zNear ) / ( zFar - zNear );
	proj[2*4+3] = -1.0f;

	unsigned long long t0 = Sys_Microseconds();
	psmBindFramebuffer( GL_FRAMEBUFFER, m_fbo );
	qglViewport( 0, 0, m_size, m_size );
	GL_State( GLS_DEPTHMASK | GLS_DEPTHFUNC_LESS );
	GL_Cull( CT_TWO_SIDED );
	GL_PolygonOffset( r_shadowPolygonFactor.GetFloat(), -r_shadowPolygonOffset.GetFloat() );
	qglClearColor( 1.0f, 1.0f, 1.0f, 1.0f );
	qglClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	backEnd.currentSpace = NULL;

	for ( int chain = 0; chain < 2; chain++ ) {
		const drawSurf_t * surf = ( chain == 0 ) ? vLight->globalShadows : vLight->localShadows;
		for ( ; surf != NULL; surf = surf->nextOnLight ) {
			if ( surf->numIndexes == 0 ) { continue; }

			float mv[16], mvp[16];
			R_MatrixMultiply( modelViewWorld, surf->space->modelMatrix, mv );
			R_MatrixMultiply( proj, mv, mvp );

			hfRenderMatrix viewProj;
			for ( int r = 0; r < 4; r++ ) {
				for ( int c = 0; c < 4; c++ ) {
					viewProj[r][c] = mvp[ c * 4 + r ];
				}
			}
			RB_SetMVP( viewProj );

			if ( surf->jointCache ) {
				renderProgManager.BindShader_DepthSkinned();
			} else {
				renderProgManager.BindShader_Depth();
			}
			RB_DrawElementsWithCounters( surf );
			m_frameCasters++;
		}
	}
	GL_PolygonOffset( 0.0f, 0.0f );
	psmBindFramebuffer( GL_FRAMEBUFFER, 0 );
	qglViewport( backEnd.viewDef->viewport.x1, backEnd.viewDef->viewport.y1,
					backEnd.viewDef->viewport.x2 + 1 - backEnd.viewDef->viewport.x1,
					backEnd.viewDef->viewport.y2 + 1 - backEnd.viewDef->viewport.y1 );
	unsigned long long dt = Sys_Microseconds() - t0;

	unsigned long id = ( unsigned long )lightCtx >> 5;
	if ( m_capturedCount < 32 ) {
		m_lightIds[m_capturedCount] = ( unsigned int )id;
		m_captured[m_capturedCount] = 1;
		m_capturedMicros += dt;
		m_capturedCount++;
	}
	if ( r_shadowMapMode.GetInteger() >= 2 ) {
		hfLib::Printf( "shadowMap: captured %d casters in %lluu s (%dx%d)\n", casterCount, dt, m_size, m_size );
	}
	return true;
}

const char * hfShadowMapManager::GetStatus( hfStr &out ) const {
	out = va( "sm:%d/%dx%d casters:%d us:%llu fbo:%s",
				m_capturedCount, m_size, m_size, m_frameCasters, m_capturedMicros,
				( m_initFlags & 2 ) ? "ok" : "unavail" );
	return out.c_str();
}

static void hfShadowMapStats_f( const hfCmdArgs & args ) {
	hfStr out;
	hfLib::Printf( "shadow map: %s\n", shadowMapManager.GetStatus( out ) );
}