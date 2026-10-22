#include "hfOpenGLPostprocess.h"

#include "hfGLCaps.h"

// HDR post-processing via compute shaders (idTech 6 style). The resolved
// rendering is captured into an RGBA32F texture, tone-mapped on the GPU with a
// compute pass, and the LDR image is then presented with the normal texture
// fullscreen quad. GL 4.3 entry points are resolved dynamically because the
// engine's GL wrapper ends at GL 3.x; when they are absent the renderer uses
// its legacy quad-based post process instead.
//
// idTech7-style extension: the scene depth buffer is blitted into the HDR
// frame buffer and a full-res SSAO pass (u_pass == 8) feeds screen-space
// ambient occlusion into the final tonemap, which also sources depth for a
// screen-space reflection ray march and a depth-of-field blur.

#if defined( _WIN32 ) || defined( _XBOXONE ) || defined( ID_CONSOLE )

// No dynamic loading on these platforms yet; compute post stays disabled.
#define HF_COMPUTE_POST_DISABLED 1

#else

#include <dlfcn.h>

#define HF_COMPUTE_POST_DISABLED 0

#endif

// GL 3.0 / 4.3 tokens that postdate the wrapped header.
#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER				0x8D40
#endif
#ifndef GL_FRAMEBUFFER_COMPLETE
#define GL_FRAMEBUFFER_COMPLETE		0x8CD5
#endif
#ifndef GL_COLOR_ATTACHMENT0
#define GL_COLOR_ATTACHMENT0		0x8CE0
#endif
#ifndef GL_RGBA32F
#define GL_RGBA32F					0x8814
#endif
#ifndef GL_RGBA8
#define GL_RGBA8					0x8058
#endif
#ifndef GL_COMPUTE_SHADER
#define GL_COMPUTE_SHADER			0x91B9
#endif
#ifndef GL_SHADER_IMAGE_ACCESS_BARRIER_BIT
#define GL_SHADER_IMAGE_ACCESS_BARRIER_BIT 0x00000020
#endif
#ifndef GL_TEXTURE_FETCH_BARRIER_BIT
#define GL_TEXTURE_FETCH_BARRIER_BIT 0x00000008
#endif
#ifndef GL_FRAMEBUFFER_BARRIER_BIT
#define GL_FRAMEBUFFER_BARRIER_BIT 0x00000400
#endif
#ifndef GL_READ_ONLY
#define GL_READ_ONLY				0x88B8
#endif
#ifndef GL_WRITE_ONLY
#define GL_WRITE_ONLY				0x88B9
#endif
#ifndef GL_DEPTH_BUFFER_BIT
#define GL_DEPTH_BUFFER_BIT			0x00000100
#endif
#ifndef GL_DEPTH_COMPONENT24
#define GL_DEPTH_COMPONENT24		0x81A6
#endif
#ifndef GL_DEPTH_ATTACHMENT
#define GL_DEPTH_ATTACHMENT			0x8D00
#endif
#ifndef GL_READ_FRAMEBUFFER
#define GL_READ_FRAMEBUFFER			0x8CA8
#endif
#ifndef GL_DRAW_FRAMEBUFFER
#define GL_DRAW_FRAMEBUFFER			0x8CA9
#endif
#ifndef GL_TEXTURE4
#define GL_TEXTURE4					0x84C4
#endif

typedef void ( APIENTRYP PFNGLGENFRAMEBUFFERSPROC )( GLsizei n, GLuint *framebuffers );
typedef void ( APIENTRYP PFNGLBINDFRAMEBUFFERPROC )( GLenum target, GLuint framebuffer );
typedef void ( APIENTRYP PFNGLDELETEFRAMEBUFFERSPROC )( GLsizei n, const GLuint *framebuffers );
typedef void ( APIENTRYP PFNGLFRAMEBUFFERTEXTURE2DPROC )( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level );
typedef GLenum ( APIENTRYP PFNGLCHECKFRAMEBUFFERSTATUSPROC )( GLenum target );
typedef void ( APIENTRYP PFNGLBINDIMAGETEXTUREPROC )( GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format );
typedef void ( APIENTRYP PFNGLDISPATCHCOMPUTEPROC )( GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z );
typedef void ( APIENTRYP PFNGLUNIFORM1FPROC )( GLint location, GLfloat v0 );
typedef void ( APIENTRYP PFNGLMEMORYBARRIERPROC )( GLbitfield barriers );
typedef void ( APIENTRYP PFNGLDELETETEXTURESPROC )( GLsizei n, const GLuint *textures );

hfCVar r_computePostprocess( "r_computePostprocess", "1", CVAR_RENDERER | CVAR_INTEGER, "enable compute-shader HDR post-processing" );
hfCVar r_postProcessExposure( "r_postProcessExposure", "1.0", CVAR_RENDERER | CVAR_FLOAT, "exposure applied before the ACES tone map" );
hfCVar r_postProcessGamma( "r_postProcessGamma", "2.2", CVAR_RENDERER | CVAR_FLOAT, "gamma applied after the ACES tone map" );
hfCVar r_bloom( "r_bloom", "1", CVAR_RENDERER | CVAR_INTEGER | CVAR_ARCHIVE, "enable the compute bloom pass (bright extraction + separable gaussian blur)" );
hfCVar r_bloomBrightThreshold( "r_bloomBrightThreshold", "1.0", CVAR_RENDERER | CVAR_FLOAT, "luminance threshold for the bloom bright pass" );
hfCVar r_bloomIntensity( "r_bloomIntensity", "0.6", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE, "amount of bloom added back on top of the tone-mapped image" );
hfCVar r_computeDepthBlit( "r_computeDepthBlit", "1", CVAR_RENDERER | CVAR_INTEGER, "copy the scene depth buffer into the HDR post framebuffer for SSAO/SSR/DoF" );
hfCVar r_ssao( "r_ssao", "0", CVAR_RENDERER | CVAR_INTEGER | CVAR_ARCHIVE, "screen-space ambient occlusion in the compute post-process" );
hfCVar r_ssaoStrength( "r_ssaoStrength", "1.2", CVAR_RENDERER | CVAR_FLOAT, "SSAO intensity" );
hfCVar r_ssr( "r_ssr", "0", CVAR_RENDERER | CVAR_INTEGER | CVAR_ARCHIVE, "screen-space reflections in the compute post-process" );
hfCVar r_ssrScale( "r_ssrScale", "0.6", CVAR_RENDERER | CVAR_FLOAT, "SSR intensity" );
hfCVar r_ssrSteps( "r_ssrSteps", "16", CVAR_RENDERER | CVAR_INTEGER, "SSR ray-march steps" );
hfCVar r_depthOfField( "r_depthOfField", "0", CVAR_RENDERER | CVAR_INTEGER | CVAR_ARCHIVE, "depth-of-field blur in the compute post-process" );
hfCVar r_dofStrength( "r_dofStrength", "0.5", CVAR_RENDERER | CVAR_FLOAT, "DoF blur strength" );
hfCVar r_dofFocusDist( "r_dofFocusDist", "400.0", CVAR_RENDERER | CVAR_FLOAT, "DoF focus distance (view-space, negative-z toward scene)" );

// near Z clip plane distance, defined in hfRenderSystem_init.cpp
extern hfCVar r_znear;

static const char * s_computeSource = R"GLSL(
#version 430 core

layout( local_size_x = 8, local_size_y = 8, local_size_z = 1 ) in;

layout( rgba32f, binding = 0 ) uniform readonly		image2D	in_image;
layout( rgba8,   binding = 1 ) uniform writeonly	image2D	out_image;
layout( rgba16f, binding = 2 ) uniform				image2D bloomA;
layout( rgba16f, binding = 3 ) uniform				image2D bloomB;
layout( binding = 4 ) uniform sampler2D 				in_depth;	// scene depth (blitted into the HDR target)
layout( rgba8,   binding = 5 ) uniform writeonly	image2D 	out_ao;		// SSAO write target (u_pass == 8)
layout( rgba8,   binding = 5 ) uniform readonly		image2D 	aoSrc;		// SSAO read source (u_pass == 4)

uniform int u_pass;
uniform float u_exposure;
uniform float u_gamma;
uniform float u_bright;
uniform float u_glow;

// projection matrix as four rows: clip = u_projX*x + u_projY*y + u_projZ*z + u_projW.
// The engine's hfMatrix is row-major float[16] (element i*4+j = row i, col j), so
// u_projX.x == projection[0][0] (focal_x) and u_projY.y == projection[1][1] (focal_y).
uniform vec4 u_projX, u_projY, u_projZ, u_projW;
uniform vec4 u_zNearFar;			// (near, far) clip planes, positive
uniform float u_ssaoStrength;		// 0 -> AO pass writes 1.0 everywhere (passthrough)
uniform float u_dofStrength;
uniform float u_dofFocus;
uniform int   u_ssrSteps;
uniform float u_ssrScale;

// Narkowicz / ACES filmic tone mapping curve.
vec3 HF_ACESFilm( vec3 x ) {
	const float a = 2.51;
	const float b = 0.03;
	const float c = 2.43;
	const float d = 0.59;
	const float e = 0.14;
	return clamp( ( x * ( a * x + b ) ) / ( x * ( c * x + d ) + e ), 0.0, 1.0 );
}

// depth [0..1] -> positive view-space z (OpenGL convention, -z toward the scene).
// dlin = a*d + b  with  a = -2*f*n/(f-n), b = (f+n)/(f-n)  inverts to
// zview = n*f / ( f - d*(f-n) )   (background d==0 lands exactly on the near plane).
float HF_LinearizeDepth( const float d ) {
	return max( u_zNearFar.x * u_zNearFar.y / ( u_zNearFar.y - d * ( u_zNearFar.y - u_zNearFar.x ) ), 0.0 );
}

// Project a view-space position back to a y-down [0..1] screen UV. The clip
// rows already contain the engine's flipProjection sign, and we flip NDC Y so
// the y-down texture/screen space reconstructed by ndc = (p+0.5)/size*2-1 lines up.
vec2 HF_ProjectScreen( const vec3 vpos ) {
	vec4 clip = u_projX * vpos.x + u_projY * vpos.y + u_projZ * vpos.z + u_projW;	// clip = M * viewPos
	vec2 ndcC = clip.xy / clip.w;
	return ndcC * vec2( 0.5, -0.5 ) + vec2( 0.5, 0.5 );
}

// Neighbour-tap normal: the two screen-space tangent vectors cross into a
// view-space normal. (P[0][0], P[1][1]) convert pixel-space depth deltas into
// view-space slopes; the flipped V axis is corrected by negating the Y tap.
vec3 HF_ViewNormal( const ivec2 p, const float zview ) {
	float dzx = HF_LinearizeDepth( texelFetch( in_depth, p + ivec2( 1, 0 ), 0 ).r );
	float dzy = HF_LinearizeDepth( texelFetch( in_depth, p + ivec2( 0, 1 ), 0 ).r );
	float dzmy = HF_LinearizeDepth( texelFetch( in_depth, p + ivec2( 0, -1 ), 0 ).r );
	float dzdX = dzx - zview;
	float dzdY = ( dzy + dzmy ) * 0.5 - zview;
	vec3 n = normalize( vec3( -dzdX * u_projX.x, -dzdY * u_projY.y, 1.0 ) );
	if ( abs( n.z ) < 0.4 ) {
		n = vec3( 0.0, 0.0, 1.0 );
	}
	return n;
}

// eight fixed, jittered hemisphere sample directions
const vec3 HBASE[8] = vec3[](
	vec3(  0.57735,  0.57735, 0.57735 ),
	vec3(  0.57735, -0.57735, 0.57735 ),
	vec3( -0.57735,  0.57735, 0.57735 ),
	vec3( -0.57735, -0.57735, 0.57735 ),
	vec3(  0.81650,  0.00000, 0.57735 ),
	vec3( -0.81650,  0.00000, 0.57735 ),
	vec3(  0.00000,  0.81650, 0.57735 ),
	vec3(  0.00000, -0.81650, 0.57735 )
);

float HF_Hash12( const vec2 p ) {
	vec3 p3 = fract( vec3( p.xyx ) * 0.1031 );
	p3 += dot( p3, p3.yzx + 33.33 );
	return fract( ( p3.x + p3.y ) * p3.z );
}

void main() {
	ivec2 p = ivec2( gl_GlobalInvocationID.xy );

	if ( u_pass == 8 ) {
		// SSAO: read depth, write an RGBA8 occlusion mask (1.0 = fully lit).
		// The ubiquity-gate: u_ssaoStrength == 0 writes 1.0, so aoVal in the
		// final pass is a no-op even though the depth data may be stale.
		ivec2 hdrSize = imageSize( in_image );
		if ( p.x >= hdrSize.x || p.y >= hdrSize.y ) {
			imageStore( out_ao, p, vec4( 1.0 ) );
			return;
		}

		float zview = HF_LinearizeDepth( texelFetch( in_depth, p, 0 ).r );
		if ( zview <= 0.0 ) {
			imageStore( out_ao, p, vec4( 1.0 ) );
			return;
		}

		// screen NDC is y-down; the texture's flipped V is corrected here
		vec2 ndc = ( vec2( p ) + 0.5 ) / vec2( hdrSize ) * 2.0 - 1.0;
		vec3 viewPos = vec3( ndc.x * zview / u_projX.x, -ndc.y * zview / u_projY.y, zview );
		vec3 n = HF_ViewNormal( p, zview );

		// occlusion radius grows with distance to the surface
		float R = 1.2 + 8.0 / zview;
		float occ = 0.0;
		float j = HF_Hash12( vec2( p ) );
		for ( int i = 0; i < 8; i++ ) {
			vec3 dir = HBASE[ i ] * ( 1.0 + j * 0.7 );
			dir.z = abs( dir.z );						// hemisphere aimed at +z (toward the viewer)
			vec3 sampPos = viewPos + dir * R * ( 0.5 + 0.5 * n.z );
			vec2 fuv = clamp( HF_ProjectScreen( sampPos ), 0.0, 1.0 );
			ivec2 q = min( ivec2( fuv * vec2( hdrSize ) ), hdrSize - ivec2( 1 ) );
			float zOther = HF_LinearizeDepth( texelFetch( in_depth, q, 0 ).r );
			if ( zOther > 0.0 && zOther > sampPos.z + 0.05 ) {
				occ += 1.0;
			}
		}
		occ *= 0.125;
		imageStore( out_ao, p, vec4( 1.0 - occ * u_ssaoStrength, 0.0, 0.0, 1.0 ) );
		return;
	}

	if ( u_pass == 4 ) {
		// final pass: tone map + add the blurred bloom, then the gated
		// screen-space AO / DoF / SSR improvements
		ivec2 hdrSize = imageSize( in_image );
		if ( p.x >= hdrSize.x || p.y >= hdrSize.y ) {
			return;
		}
		vec4 color = imageLoad( in_image, p );
		vec3 mapped = HF_ACESFilm( color.rgb * u_exposure );
		ivec2 bd = imageSize( bloomA );
		vec3 glow = u_glow * imageLoad( bloomA, ivec2( min( bd.x - 1, p.x / 4 ), min( bd.y - 1, p.y / 4 ) ) ).rgb;
		mapped += glow;

		// SSAO term: always reads the mask; pass 8 is always dispatched with
		// strength 0 (writes 1.0) when r_ssao is off, so this stays exact.
		float aoVal = imageLoad( aoSrc, p ).r;
		mapped *= aoVal;

		float zview = HF_LinearizeDepth( texelFetch( in_depth, p, 0 ).r );
		vec3 nrm = HF_ViewNormal( p, zview );
		vec2 ndc = ( vec2( p ) + 0.5 ) / vec2( hdrSize ) * 2.0 - 1.0;
		vec3 viewPos = vec3( ndc.x * zview / u_projX.x, -ndc.y * zview / u_projY.y, zview );

		if ( zview > 0.0 ) {
			// DoF: 3-tap horizontal blur whose tap spacing grows with the
			// circle of confusion projected to pixels.
			if ( u_dofStrength > 0.0 && u_dofFocus > u_zNearFar.x ) {
				float cocE = abs( zview - u_dofFocus );
				float cocS = cocE * ( u_projX.x / zview );				// pixels, approx
				float th = clamp( u_projX.x * 0.0008, 0.4, 2.0 );
				if ( cocS > th ) {
					float off = clamp( cocS * 2.0, 1.5, 24.0 );
					ivec2 t0 = ivec2( clamp( float( p.x ) + off, 0.0, float( hdrSize.x - 1 ) ), p.y );
					ivec2 t1 = ivec2( clamp( float( p.x ) - off, 0.0, float( hdrSize.x - 1 ) ), p.y );
					vec3 blurred = ( mapped + imageLoad( in_image, t0 ).rgb + imageLoad( in_image, t1 ).rgb ) * 0.3333333;
					float dl = clamp( cocE * 0.05 * u_dofStrength, 0.0, 1.0 );
					mapped = mix( mapped, blurred, dl );
				}
			}

			// SSR: short view-space ray march along the reflected view ray,
			// sampling the HDR color at the first depth hit.
			if ( u_ssrSteps > 0 && u_ssrScale > 0.0 ) {
				vec3 V = normalize( viewPos );
				float NdotV = max( dot( nrm, V ), 0.0 );
				vec3 Rdir = normalize( 2.0 * NdotV * nrm - V );
				vec3 rayCol = vec3( 0.0 );
				if ( Rdir.z > 1e-5 ) {
					float zStep = max( ( zview - u_zNearFar.x ) / ( 8.0 + float( u_ssrSteps ) ), 0.25 );
					vec3 cur = viewPos;
					int  step = 0;
					for ( int i = 0; i < u_ssrSteps; i++ ) {
						step = i;
						cur += Rdir * zStep;
						vec2 fuv = clamp( HF_ProjectScreen( cur ), 0.0, 1.0 );
						ivec2 q = min( ivec2( fuv * vec2( hdrSize ) ), hdrSize - ivec2( 1 ) );
						float depthHit = HF_LinearizeDepth( texelFetch( in_depth, q, 0 ).r );
						if ( depthHit > 0.0 && depthHit < cur.z ) {
							// mask near-plane hits so behavior matches plain depth
							rayCol = ( length( cur - viewPos ) > 1.5 )
										? imageLoad( in_image, q ).rgb
										: mapped.rgb * 0.1;
							break;
						}
					}
					float fade = exp( -length( cur - viewPos ) * 0.02 );
					rayCol *= ( 1.0 - float( step ) / float( max( u_ssrSteps, 1 ) ) ) * fade * u_ssrScale;
					mapped += rayCol;
				}
			}
		}

		vec3 outColor = pow( clamp( mapped, 0.0, 1.0 ), vec3( 1.0 / max( u_gamma, 0.01 ) ) );
		imageStore( out_image, p, vec4( outColor, color.a ) );
		return;
	}

	if ( u_pass == 1 ) {
		// bright extraction + 4x4 downsample of the HDR source to quarter res
		ivec2 bd = imageSize( bloomA );
		if ( p.x >= bd.x || p.y >= bd.y ) {
			return;
		}
		ivec2 hdrSize = imageSize( in_image );
		ivec2 h0 = p * 4;
		vec4 sum = vec4( 0.0 );
		for ( int dy = 0; dy < 4; dy++ ) {
			int yy = clamp( h0.y + dy, 0, hdrSize.y - 1 );
			for ( int dx = 0; dx < 4; dx++ ) {
				int xx = clamp( h0.x + dx, 0, hdrSize.x - 1 );
				sum += imageLoad( in_image, ivec2( xx, yy ) );
			}
		}
		sum *= 0.0625;
		float luma = dot( sum.rgb, vec3( 0.2126, 0.7152, 0.0722 ) );
		float knee = clamp( ( luma - u_bright ) / max( u_bright * 0.5, 0.001 ), 0.0, 1.0 );
		imageStore( bloomA, p, vec4( sum.rgb * knee, 1.0 ) );
		return;
	}

	// separable gaussian blur, ping-ponging between bloomA and bloomB
	ivec2 bd = imageSize( bloomA );
	if ( p.x >= bd.x || p.y >= bd.y ) {
		return;
	}
	const vec4 weights = vec4( 0.1945946, 0.1216216, 0.054054, 0.016216 );
	if ( u_pass == 3 ) {
		// vertical pass: read bloomB, write back into bloomA
		vec4 col = imageLoad( bloomB, p ) * 0.227027;
		for ( int i = 1; i <= 4; i++ ) {
			ivec2 o1 = ivec2( p.x, clamp( p.y + i, 0, bd.y - 1 ) );
			ivec2 o2 = ivec2( p.x, clamp( p.y - i, 0, bd.y - 1 ) );
			col += weights[ i - 1 ] * ( imageLoad( bloomB, o1 ) + imageLoad( bloomB, o2 ) );
		}
		imageStore( bloomA, p, col );
	} else {
		// horizontal pass: read bloomA, write into bloomB
		vec4 col = imageLoad( bloomA, p ) * 0.227027;
		for ( int i = 1; i <= 4; i++ ) {
			ivec2 o1 = ivec2( clamp( p.x + i, 0, bd.x - 1 ), p.y );
			ivec2 o2 = ivec2( clamp( p.x - i, 0, bd.x - 1 ), p.y );
			col += weights[ i - 1 ] * ( imageLoad( bloomA, o1 ) + imageLoad( bloomA, o2 ) );
		}
		imageStore( bloomB, p, col );
	}
}
)GLSL";

static bool s_initialized = false;
static bool s_available = false;
static int s_width = 0;
static int s_height = 0;

static GLuint s_hdrTex = 0;
static GLuint s_resultTex = 0;
static GLuint s_bloomA = 0;
static GLuint s_bloomB = 0;
static GLuint s_depthTex = 0;
static GLuint s_aoTex = 0;
static GLuint s_fbo = 0;
static GLuint s_program = 0;

// Cached view/projection matrix, row-major float[16] (element i*4+j = row i, col j).
// clip = M * vec4(viewPos,1), so row i = s_proj[i*4 .. i*4+3]. Uploaded to
// u_projX..u_projW by hfComputePost_Dispatch before the compute passes.
static float s_proj[16] = { 0 };

// Resolved GL 4.3 entry points (NULL when the driver does not expose them).
#if !HF_COMPUTE_POST_DISABLED
static PFNGLGENFRAMEBUFFERSPROC pglGenFramebuffers = NULL;
static PFNGLBINDFRAMEBUFFERPROC pglBindFramebuffer = NULL;
static PFNGLDELETEFRAMEBUFFERSPROC pglDeleteFramebuffers = NULL;
static PFNGLFRAMEBUFFERTEXTURE2DPROC pglFramebufferTexture2D = NULL;
static PFNGLCHECKFRAMEBUFFERSTATUSPROC pglCheckFramebufferStatus = NULL;
static PFNGLBINDIMAGETEXTUREPROC pglBindImageTexture = NULL;
static PFNGLDISPATCHCOMPUTEPROC pglDispatchCompute = NULL;
static PFNGLMEMORYBARRIERPROC pglMemoryBarrier = NULL;
static PFNGLUNIFORM1FPROC pglUniform1f = NULL;
static PFNGLUNIFORM4FPROC pglUniform4f = NULL;
static PFNGLBLITFRAMEBUFFERPROC pglBlitFramebuffer = NULL;
static PFNGLACTIVETEXTUREPROC pglActiveTexture = NULL;
#endif

GLuint hfComputePost_HDRTexture() {
	return s_hdrTex;
}

GLuint hfComputePost_ResultTexture() {
	return s_resultTex;
}

GLuint hfComputePost_DepthTexture() {
	return s_depthTex;
}

GLuint hfComputePost_AOTexture() {
	return s_aoTex;
}

void hfComputePost_SetProjectionMatrix( const float m[16] ) {
	memcpy( s_proj, m, sizeof( s_proj ) );
}

static void HfComputePost_ReportDisable() {
	hfLib::Printf( "HF_OpenGLPostprocess: compute post-processing unavailable, using legacy quad path\n" );
}

/*
=================
ResolveGL43EntryPoints

Looks up the post-GL-3.x functions in the process. The engine is linked against
the system OpenGL library, so RTLD_DEFAULT finds them when the driver supports
the newer context. glBlitFramebuffer is deliberately *not* required for
success: when it is missing the depth-copy simply does not run (the feature
cvars all default to 0).
=================
*/
static bool ResolveGL43EntryPoints() {
#if HF_COMPUTE_POST_DISABLED
	return false;
#else
	pglGenFramebuffers = ( PFNGLGENFRAMEBUFFERSPROC ) dlsym( RTLD_DEFAULT, "glGenFramebuffers" );
	pglBindFramebuffer = ( PFNGLBINDFRAMEBUFFERPROC ) dlsym( RTLD_DEFAULT, "glBindFramebuffer" );
	pglDeleteFramebuffers = ( PFNGLDELETEFRAMEBUFFERSPROC ) dlsym( RTLD_DEFAULT, "glDeleteFramebuffers" );
	pglFramebufferTexture2D = ( PFNGLFRAMEBUFFERTEXTURE2DPROC ) dlsym( RTLD_DEFAULT, "glFramebufferTexture2D" );
	pglCheckFramebufferStatus = ( PFNGLCHECKFRAMEBUFFERSTATUSPROC ) dlsym( RTLD_DEFAULT, "glCheckFramebufferStatus" );
	pglBindImageTexture = ( PFNGLBINDIMAGETEXTUREPROC ) dlsym( RTLD_DEFAULT, "glBindImageTexture" );
	pglDispatchCompute = ( PFNGLDISPATCHCOMPUTEPROC ) dlsym( RTLD_DEFAULT, "glDispatchCompute" );
	pglMemoryBarrier = ( PFNGLMEMORYBARRIERPROC ) dlsym( RTLD_DEFAULT, "glMemoryBarrier" );
	pglUniform1f = ( PFNGLUNIFORM1FPROC ) dlsym( RTLD_DEFAULT, "glUniform1f" );
	pglUniform4f = ( PFNGLUNIFORM4FPROC ) dlsym( RTLD_DEFAULT, "glUniform4f" );
	pglBlitFramebuffer = ( PFNGLBLITFRAMEBUFFERPROC ) dlsym( RTLD_DEFAULT, "glBlitFramebuffer" );
	pglActiveTexture = ( PFNGLACTIVETEXTUREPROC ) dlsym( RTLD_DEFAULT, "glActiveTexture" );

	return pglGenFramebuffers && pglBindFramebuffer && pglDeleteFramebuffers
			&& pglFramebufferTexture2D && pglCheckFramebufferStatus
			&& pglBindImageTexture && pglDispatchCompute && pglMemoryBarrier
			&& pglUniform1f && pglUniform4f && pglActiveTexture;
#endif
}

/*
=================
CompileComputeProgram
=================
*/
static GLuint CompileComputeProgram() {
	const char * src = s_computeSource;

	GLuint shader = qglCreateShader( GL_COMPUTE_SHADER );
	if ( shader == 0 ) {
		return 0;
	}
	GLint length = (GLint)strlen( src );
	const char * sourcePtr = src;
	qglShaderSource( shader, 1, &sourcePtr, &length );
	qglCompileShader( shader );

	GLint status = GL_FALSE;
	qglGetShaderiv( shader, GL_COMPILE_STATUS, &status );
	if ( status == GL_FALSE ) {
		char infoLog[1024] = {};
		qglGetShaderInfoLog( shader, sizeof( infoLog ) - 1, NULL, infoLog );
		hfLib::Printf( "HF_OpenGLPostprocess: compute shader compile failed:\n%s\n", infoLog );
		qglDeleteShader( shader );
		return 0;
	}

	GLuint program = qglCreateProgram();
	qglAttachShader( program, shader );
	qglLinkProgram( program );
	qglDeleteShader( shader );

	qglGetProgramiv( program, GL_LINK_STATUS, &status );
	if ( status == GL_FALSE ) {
		char infoLog[1024] = {};
		qglGetProgramInfoLog( program, sizeof( infoLog ) - 1, NULL, infoLog );
		hfLib::Printf( "HF_OpenGLPostprocess: compute program link failed:\n%s\n", infoLog );
		qglDeleteProgram( program );
		return 0;
	}

	return program;
}

/*
=================
hfComputePost_Available
=================
*/
bool hfComputePost_Available() {
	if ( !s_initialized ) {
		s_initialized = true;

		if ( !r_computePostprocess.GetBool() ) {
			hfLib::Printf( "HF_OpenGLPostprocess: r_computePostprocess disabled\n" );
			s_available = false;
		} else if ( !ResolveGL43EntryPoints() || !hfGLCaps::HasFlag( HF_GL_CAP_COMPUTE ) ) {
			HfComputePost_ReportDisable();
			s_available = false;
		} else {
			s_program = CompileComputeProgram();
			s_available = ( s_program != 0 );
			if ( !s_available ) {
				HfComputePost_ReportDisable();
			} else {
				hfLib::Printf( "HF_OpenGLPostprocess: compute HDR post-processing enabled\n" );
			}
		}
	}
	return s_available;
}

/*
=================
CreateScratchTextures
=================
*/
static bool CreateScratchTextures( int width, int height ) {
	if ( width < 1 || height < 1 ) {
		return false;
	}

	qglGenTextures( 1, &s_hdrTex );
	qglBindTexture( GL_TEXTURE_2D, s_hdrTex );
	qglTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	qglGenTextures( 1, &s_resultTex );
	qglBindTexture( GL_TEXTURE_2D, s_resultTex );
	qglTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	// quarter-resolution half-float bloom ping-pong pair
	const int bloomW = ( width + 3 ) / 4;
	const int bloomH = ( height + 3 ) / 4;
	qglGenTextures( 1, &s_bloomA );
	qglBindTexture( GL_TEXTURE_2D, s_bloomA );
	qglTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA16F, bloomW, bloomH, 0, GL_RGBA, GL_HALF_FLOAT, NULL );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	qglGenTextures( 1, &s_bloomB );
	qglBindTexture( GL_TEXTURE_2D, s_bloomB );
	qglTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA16F, bloomW, bloomH, 0, GL_RGBA, GL_HALF_FLOAT, NULL );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	// full-res DEPTH_COMPONENT24 target receiving the default framebuffer's
	// depth via glBlitFramebuffer (consumed by SSAO / SSR / DoF) and the
	// full-res RGBA8 SSAO mask produced by u_pass == 8.
	qglGenTextures( 1, &s_depthTex );
	qglBindTexture( GL_TEXTURE_2D, s_depthTex );
	qglTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	qglGenTextures( 1, &s_aoTex );
	qglBindTexture( GL_TEXTURE_2D, s_aoTex );
	qglTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	pglGenFramebuffers( 1, &s_fbo );
	pglBindFramebuffer( GL_FRAMEBUFFER, s_fbo );
	pglFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_hdrTex, 0 );
	pglFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, s_depthTex, 0 );
	GLenum fboStatus = pglCheckFramebufferStatus( GL_FRAMEBUFFER );
	pglBindFramebuffer( GL_FRAMEBUFFER, 0 );

	if ( fboStatus != GL_FRAMEBUFFER_COMPLETE ) {
		hfLib::Printf( "HF_OpenGLPostprocess: HDR framebuffer incomplete (0x%04x)\n", (unsigned)fboStatus );
		hfComputePost_Shutdown();
		return false;
	}

	s_width = width;
	s_height = height;
	return true;
}

/*
=================
hfComputePost_BeginHDRCapture
=================
*/
bool hfComputePost_BeginHDRCapture( int width, int height ) {
	if ( !hfComputePost_Available() ) {
		return false;
	}
	if ( s_hdrTex == 0 || width != s_width || height != s_height ) {
		hfComputePost_Shutdown();
		if ( !CreateScratchTextures( width, height ) ) {
			return false;
		}
	}
	pglBindFramebuffer( GL_FRAMEBUFFER, s_fbo );

	// copy backbuffer scene depth into the HDR target's depth attachment; the
	// read source is the default (window) framebuffer. Failure cannot break the
	// render: if the blit did not run, the buffer simply holds stale/undefined
	// depth, and SSAO / SSR / DoF all default to disabled.
	if ( r_computeDepthBlit.GetBool() && pglBlitFramebuffer != NULL ) {
		pglBindFramebuffer( GL_READ_FRAMEBUFFER, 0 );				// default framebuffer as READ source
		pglBlitFramebuffer( 0, 0, width, height, 0, 0, width, height, GL_DEPTH_BUFFER_BIT, GL_NEAREST );
	}
	// ensure the HDR target stays bound as the DRAW framebuffer regardless
	pglBindFramebuffer( GL_FRAMEBUFFER, s_fbo );

	return true;
}

/*
=================
hfComputePost_EndHDRCapture
=================
*/
void hfComputePost_EndHDRCapture() {
	if ( s_available ) {
		pglBindFramebuffer( GL_FRAMEBUFFER, 0 );
	}
}

/*
=================
hfComputePost_Dispatch
=================
*/
void hfComputePost_Dispatch( int width, int height, float exposure ) {
	if ( !s_available || s_program == 0 ) {
		return;
	}

	qglUseProgram( s_program );
	pglUniform1f( qglGetUniformLocation( s_program, "u_exposure" ), exposure );
	pglUniform1f( qglGetUniformLocation( s_program, "u_gamma" ), r_postProcessGamma.GetFloat() );
	pglUniform1f( qglGetUniformLocation( s_program, "u_bright" ), r_bloomBrightThreshold.GetFloat() );
	const bool bloomOn = r_bloom.GetInteger() > 0;
	pglUniform1f( qglGetUniformLocation( s_program, "u_glow" ), bloomOn ? r_bloomIntensity.GetFloat() : 0.0f );

	// bind the scene-depth sampler once (unit 4); stays valid for every pass
	if ( s_depthTex != 0 && pglActiveTexture != NULL ) {
		pglActiveTexture( GL_TEXTURE4 );
		qglBindTexture( GL_TEXTURE_2D, s_depthTex );
		pglActiveTexture( GL_TEXTURE0 );
	}

	// view/projection rows for view-space reconstruction in SSAO / SSR / DoF
	pglUniform4f( qglGetUniformLocation( s_program, "u_projX" ), s_proj[0], s_proj[1], s_proj[2], s_proj[3] );
	pglUniform4f( qglGetUniformLocation( s_program, "u_projY" ), s_proj[4], s_proj[5], s_proj[6], s_proj[7] );
	pglUniform4f( qglGetUniformLocation( s_program, "u_projZ" ), s_proj[8], s_proj[9], s_proj[10], s_proj[11] );
	pglUniform4f( qglGetUniformLocation( s_program, "u_projW" ), s_proj[12], s_proj[13], s_proj[14], s_proj[15] );

	// near plane from r_znear; the far plane has no cvar in this engine. The
	// projection is the far-plane-at-infinity formulation (m[14] == -2*near),
	// so no explicit far is recoverable — approximate with a generous constant;
	// depth reconstruction only needs it to differ from near.
	const float zNear = r_znear.GetFloat();
	const float zFar = 40000.0f;	// provisional far clip plane (see note above)
	pglUniform4f( qglGetUniformLocation( s_program, "u_zNearFar" ), zNear, zFar, 0.0f, 0.0f );

	pglUniform1f( qglGetUniformLocation( s_program, "u_ssaoStrength" ), r_ssao.GetInteger() ? r_ssaoStrength.GetFloat() : 0.0f );
	pglUniform1f( qglGetUniformLocation( s_program, "u_dofStrength" ), r_depthOfField.GetInteger() ? r_dofStrength.GetFloat() : 0.0f );
	pglUniform1f( qglGetUniformLocation( s_program, "u_dofFocus" ), r_dofFocusDist.GetFloat() );
	qglUniform1i( qglGetUniformLocation( s_program, "u_ssrSteps" ), r_ssrSteps.GetInteger() );
	pglUniform1f( qglGetUniformLocation( s_program, "u_ssrScale" ), r_ssr.GetInteger() ? r_ssrScale.GetFloat() : 0.0f );

	pglBindImageTexture( 0, s_hdrTex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F );
	pglBindImageTexture( 1, s_resultTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8 );

	const int bloomW = ( width + 3 ) / 4;
	const int bloomH = ( height + 3 ) / 4;

	if ( bloomOn && s_bloomA != 0 && s_bloomB != 0 ) {
		// bright extraction + downsample: hdr -> bloomA
		pglBindImageTexture( 2, s_bloomA, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F );
		qglUniform1i( qglGetUniformLocation( s_program, "u_pass" ), 1 );
		pglDispatchCompute( ( bloomW + 7 ) / 8, ( bloomH + 7 ) / 8, 1 );
		pglMemoryBarrier( GL_SHADER_IMAGE_ACCESS_BARRIER_BIT );

		// horizontal blur: bloomA -> bloomB
		pglBindImageTexture( 3, s_bloomB, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F );
		qglUniform1i( qglGetUniformLocation( s_program, "u_pass" ), 2 );
		pglDispatchCompute( ( bloomW + 7 ) / 8, ( bloomH + 7 ) / 8, 1 );
		pglMemoryBarrier( GL_SHADER_IMAGE_ACCESS_BARRIER_BIT );

		// vertical blur: bloomB -> bloomA
		qglUniform1i( qglGetUniformLocation( s_program, "u_pass" ), 3 );
		pglDispatchCompute( ( bloomW + 7 ) / 8, ( bloomH + 7 ) / 8, 1 );
		pglMemoryBarrier( GL_SHADER_IMAGE_ACCESS_BARRIER_BIT );
	}

	// SSAO into the AO texture. Always runs: when r_ssao == 0 the strength is 0
	// so the mask is written as 1.0 everywhere (exact passthrough for the AO
	// multiply in the final pass), keeping the AO buffer valid every frame.
	if ( s_aoTex != 0 ) {
		pglBindImageTexture( 5, s_aoTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8 );
		qglUniform1i( qglGetUniformLocation( s_program, "u_pass" ), 8 );
		pglDispatchCompute( ( width + 7 ) / 8, ( height + 7 ) / 8, 1 );
		pglMemoryBarrier( GL_SHADER_IMAGE_ACCESS_BARRIER_BIT );
	}

	// final pass: tone map + add bloom, then AO / DoF / SSR, gamma
	pglBindImageTexture( 2, s_bloomA, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F );
	pglBindImageTexture( 5, s_aoTex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8 );
	qglUniform1i( qglGetUniformLocation( s_program, "u_pass" ), 4 );
	pglDispatchCompute( ( width + 7 ) / 8, ( height + 7 ) / 8, 1 );

	pglMemoryBarrier( GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_FRAMEBUFFER_BARRIER_BIT );

	qglUseProgram( 0 );
}

/*
=================
hfComputePost_Shutdown
=================
*/
void hfComputePost_Shutdown() {
#if HF_COMPUTE_POST_DISABLED
	s_program = 0;
	s_fbo = 0;
	s_hdrTex = 0;
	s_resultTex = 0;
	s_bloomA = 0;
	s_bloomB = 0;
	s_depthTex = 0;
	s_aoTex = 0;
#else
	if ( s_program != 0 ) {
		qglDeleteProgram( s_program );
		s_program = 0;
	}
	if ( s_fbo != 0 ) {
		pglDeleteFramebuffers( 1, &s_fbo );
		s_fbo = 0;
	}
	if ( s_hdrTex != 0 ) {
		qglDeleteTextures( 1, &s_hdrTex );
		s_hdrTex = 0;
	}
	if ( s_resultTex != 0 ) {
		qglDeleteTextures( 1, &s_resultTex );
		s_resultTex = 0;
	}
	if ( s_bloomA != 0 ) {
		qglDeleteTextures( 1, &s_bloomA );
		s_bloomA = 0;
	}
	if ( s_bloomB != 0 ) {
		qglDeleteTextures( 1, &s_bloomB );
		s_bloomB = 0;
	}
	if ( s_depthTex != 0 ) {
		qglDeleteTextures( 1, &s_depthTex );
		s_depthTex = 0;
	}
	if ( s_aoTex != 0 ) {
		qglDeleteTextures( 1, &s_aoTex );
		s_aoTex = 0;
	}
#endif
	s_available = false;
	s_initialized = false;
	s_width = 0;
	s_height = 0;
}