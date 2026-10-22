#include "hfUberGLSL.h"
#include "hftr_local.h"
#include "hfMaterial.h"
#include "hfGLCaps.h"

// idTech 6 style material shader: two hand-written native GLSL program pairs
// (one regular, one GPU-skinned) replace the runtime Cg->GLSL conversion for
// the common diffuse/vertex-color/texgen/interaction material builtins. Program
// behavior is selected per-draw with a feature mask uniform (rpUser0) that
// mirrors the explicit program permutation selection performed by the material
// compiler in idTech 6.

hfCVar r_useNativeGLSL( "r_useNativeGLSL", "1", CVAR_RENDERER | CVAR_INTEGER, "use the native GLSL uber material shaders instead of the Cg->GLSL conversion" );

hfCVar r_usePBR( "r_usePBR", "1", CVAR_RENDERER | CVAR_INTEGER | CVAR_ARCHIVE, "physically-based (GGX) lighting in the native uber interaction shader" );
hfCVar r_pbrRoughness( "r_pbrRoughness", "0.55", CVAR_RENDERER | CVAR_FLOAT, "default PBR roughness" );
hfCVar r_pbrMetallic( "r_pbrMetallic", "0.0", CVAR_RENDERER | CVAR_FLOAT, "default PBR metallic" );
hfCVar r_pbrAO( "r_pbrAO", "1.0", CVAR_RENDERER | CVAR_FLOAT, "default PBR ambient occlusion" );
hfCVar r_pbrEmissiveScale( "r_pbrEmissiveScale", "1.0", CVAR_RENDERER | CVAR_FLOAT, "default PBR emissive scale" );
hfCVar r_pbrAmbientR( "r_pbrAmbientR", "0.55", CVAR_RENDERER | CVAR_FLOAT, "PBR ambient light color (R)" );
hfCVar r_pbrAmbientG( "r_pbrAmbientG", "0.55", CVAR_RENDERER | CVAR_FLOAT, "PBR ambient light color (G)" );
hfCVar r_pbrAmbientB( "r_pbrAmbientB", "0.6", CVAR_RENDERER | CVAR_FLOAT, "PBR ambient light color (B)" );
hfCVar r_pbrAmbientScale( "r_pbrAmbientScale", "1.0", CVAR_RENDERER | CVAR_FLOAT, "PBR ambient intensity" );

// Material builtins covered by the uber pair. Environment/bumpyEnvironment use
// cube-map texgen and keep using the legacy converter; the *_skinned variants
// use the GPU-skinning vertex program. Coverage and the default feature masks
// live in HF_Uber (hfUberLogic.cpp) so the CPU test harness shares them.

static bool IsSkinnedProgram( const char * name ) {
	if ( name == NULL || name[0] == '\0' ) {
		return false;
	}
	return hfStr::Icmp( name, "texture_color_skinned.vfp" ) == 0
			|| hfStr::Icmp( name, "interaction_skinned.vfp" ) == 0
			|| hfStr::Icmp( name, "interactionAmbient_skinned.vfp" ) == 0;
}

/*
================
HF_UberGLSL_TestUber_f

"testUber [programName]" - dumps the native-GLSL coverage and the resolved
feature mask for every uber-covered builtin program, or for a single named
program. Uses the same pure logic as the CPU test harness.
================
*/
static void HF_UberGLSL_TestUber_f( const hfCmdArgs & args ) {
	static const uint32 s_allBits[] = {
		HF_UBER_DIFFUSE, HF_UBER_BUMP, HF_UBER_SPECULAR, HF_UBER_VERTEXCOLOR,
		HF_UBER_ALPHATEST, HF_UBER_TEXGEN, HF_UBER_DIFFUSEMOD, HF_UBER_LIGHTED,
		HF_UBER_PBR
	};

	if ( args.Argc() > 1 ) {
		const char * name = args.Argv( 1 );
		if ( HF_Uber::IsCovered( name ) ) {
			uint32 f = HF_UberGLSL_DefaultFeaturesFor( name );
			hfLib::Printf( "testUber: %s covered, features 0x%04x:", name, ( unsigned ) f );
			for ( size_t i = 0; i < sizeof( s_allBits ) / sizeof( s_allBits[0] ); i++ ) {
				if ( HF_Uber::FeatureEnabled( f, s_allBits[i] ) ) {
					hfLib::Printf( " %s", HF_Uber::FeatureName( s_allBits[i] ) );
				}
			}
			hfLib::Printf( "\n" );
		} else {
			hfLib::Printf( "testUber: %s not covered (legacy converter)\n", name );
		}
		return;
	}

	hfLib::Printf( "testUber: uber native-GLSL program coverage\n" );
	const char * const * list = HF_Uber::CoveredList();
	for ( int i = 0; list[i] != NULL; i++ ) {
		uint32 f = HF_UberGLSL_DefaultFeaturesFor( list[i] );
		hfLib::Printf( "  %-34s 0x%04x%s\n", list[i], ( unsigned ) f,
			IsSkinnedProgram( list[i] ) ? " (skinned)" : "" );
	}
}

bool HF_UberGLSL_Covered( const char * name ) {
	return HF_Uber::IsCovered( name );
}

uint32 HF_UberGLSL_DefaultFeaturesFor( const char * name ) {
	if ( name == NULL || name[0] == '\0' ) {
		return 0;
	}
	uint32 f = HF_Uber::DefaultFeaturesFor( name );
	if ( r_usePBR.GetBool() ) {
		f |= HF_UBER_PBR;
	}
	return f;
}

void HF_UberGLSL_SetFeatures( uint32 features ) {
	const float v[4] = {
		(float)( features & 0xFFFF ), 0.0f, 0.0f, 0.0f
	};
	renderProgManager.SetRenderParm( RENDERPARM_USER, v );

	if ( r_usePBR.GetBool() ) {
		float pbrParms[4] = { r_pbrRoughness.GetFloat(), r_pbrMetallic.GetFloat(), r_pbrAO.GetFloat(), r_pbrEmissiveScale.GetFloat() };
		renderProgManager.SetRenderParm( ( renderParm_t )( RENDERPARM_USER + 1 ), pbrParms );
		float dofParms[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		renderProgManager.SetRenderParm( ( renderParm_t )( RENDERPARM_USER + 2 ), dofParms );
		float ssrParms[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		renderProgManager.SetRenderParm( ( renderParm_t )( RENDERPARM_USER + 3 ), ssrParms );
		float ambParms[4] = { r_pbrAmbientR.GetFloat(), r_pbrAmbientG.GetFloat(), r_pbrAmbientB.GetFloat(), r_pbrAmbientScale.GetFloat() };
		renderProgManager.SetRenderParm( ( renderParm_t )( RENDERPARM_USER + 4 ), ambParms );
	}
}

/*
==============
HF_UberGLSL_VertexSource

Embedded regular vertex program. Named uniforms match the renderParm_t /
engine attribute names so the program works with the unmodified geometry
buffers and matrix setup.
==============
*/
static const char * s_uberVertexSource = R"GLSL(
#version 140

uniform vec4 rpMVPmatrixX, rpMVPmatrixY, rpMVPmatrixZ, rpMVPmatrixW;
uniform vec4 rpModelMatrixX, rpModelMatrixY, rpModelMatrixZ, rpModelMatrixW;
uniform vec4 rpModelViewMatrixX, rpModelViewMatrixY, rpModelViewMatrixZ, rpModelViewMatrixW;
uniform vec4 rpUser0;

in vec4 in_Position;
in vec2 in_TexCoord;
in vec4 in_Normal;
in vec4 in_Tangent;
in vec4 in_Color;

out vec2 vofi_TexCoord0;
out vec3 vofi_TexCoord1;
out vec3 vofi_TexCoord2;
out vec3 vofi_TexCoord3;
out vec4 vofi_Color;

const float F_TEXGEN = 32.0;
const float F_LIGHTED = 128.0;
const float F_PBR = 256.0;

bool HF_HasFeature( const float feature ) {
	return mod( floor( rpUser0.x / feature ), 2.0 ) > 0.5;
}

void main() {
	vec4 modelPos = rpModelMatrixX * in_Position.x
				  + rpModelMatrixY * in_Position.y
				  + rpModelMatrixZ * in_Position.z
				  + rpModelMatrixW;

	mat3 modelView3 = mat3( rpModelViewMatrixX.xyz,
							rpModelViewMatrixY.xyz,
							rpModelViewMatrixZ.xyz );

	vofi_TexCoord0 = in_TexCoord;
	vofi_TexCoord2 = normalize( modelView3 * normalize( in_Normal.xyz ) );
	vofi_TexCoord3 = normalize( modelView3 * normalize( in_Tangent.xyz ) );
	vofi_Color = in_Color;

	gl_Position = rpMVPmatrixX * modelPos.x
				+ rpMVPmatrixY * modelPos.y
				+ rpMVPmatrixZ * modelPos.z
				+ rpMVPmatrixW;

	if ( HF_HasFeature( F_TEXGEN ) ) {
		// screen-space texgen (TG_SCREEN / TG_SCREEN2 approximation)
		vofi_TexCoord1.xy = gl_Position.xy / gl_Position.w * 0.5 + 0.5;
		vofi_TexCoord1.z = 0.0;
	} else {
		vec4 viewPos = rpModelViewMatrixX * modelPos.x
					  + rpModelViewMatrixY * modelPos.y
					  + rpModelViewMatrixZ * modelPos.z
					  + rpModelViewMatrixW;
		if ( HF_HasFeature( F_LIGHTED ) && HF_HasFeature( F_PBR ) ) {
			vofi_TexCoord1 = viewPos.xyz;
		} else {
			vofi_TexCoord1 = modelPos.xyz;
		}
	}
}
)GLSL";

/*
==============
HF_UberGLSL_VertexSkinnedSource

Embedded GPU-skinned vertex program. The joint matrices live in the
matrices_ubo uniform block bound to binding point 0 (already wired up by
LoadGLSLProgram). hfJointMat is a 3x4 row-major matrix, so one joint occupies
three consecutive vec4 rows: row0 dot (p,1), row1 dot (p,1), row2 dot (p,1).
Joint indices arrive packed in in_Color (UNSIGNED_BYTE normalized) and the
per-joint weights in in_Color2 (UNSIGNED_BYTE normalized, 0..1).
==============
*/
static const char * s_uberVertexSkinnedSource = R"GLSL(
#version 140

uniform vec4 rpMVPmatrixX, rpMVPmatrixY, rpMVPmatrixZ, rpMVPmatrixW;
uniform vec4 rpModelMatrixX, rpModelMatrixY, rpModelMatrixZ, rpModelMatrixW;
uniform vec4 rpModelViewMatrixX, rpModelViewMatrixY, rpModelViewMatrixZ, rpModelViewMatrixW;
uniform vec4 rpUser0;

layout( std140 ) uniform matrices_ubo {
	vec4 uboJointRows[ 256 * 3 ];
};

in vec4 in_Position;
in vec2 in_TexCoord;
in vec4 in_Normal;
in vec4 in_Tangent;
in vec4 in_Color;
in vec4 in_Color2;

out vec2 vofi_TexCoord0;
out vec3 vofi_TexCoord1;
out vec3 vofi_TexCoord2;
out vec3 vofi_TexCoord3;
out vec4 vofi_Color;

const float F_TEXGEN = 32.0;
const float F_LIGHTED = 128.0;
const float F_PBR = 256.0;

const int HF_MAX_JOINTS = 256;

int HF_JointIndex( const float packed ) {
	return clamp( int( packed * 255.0 ), 0, HF_MAX_JOINTS - 1 );
}

vec3 HF_BoneTransform( const vec3 p, const float packedIndex ) {
	int j = HF_JointIndex( packedIndex );
	vec4 row0 = uboJointRows[ j * 3 + 0 ];
	vec4 row1 = uboJointRows[ j * 3 + 1 ];
	vec4 row2 = uboJointRows[ j * 3 + 2 ];
	return row0.xyz * p.x + row1.xyz * p.y + row2.xyz * p.z
			+ vec3( row0.w, row1.w, row2.w );
}

vec3 HF_BoneRotate( const vec3 n, const float packedIndex ) {
	int j = HF_JointIndex( packedIndex );
	vec4 row0 = uboJointRows[ j * 3 + 0 ];
	vec4 row1 = uboJointRows[ j * 3 + 1 ];
	vec4 row2 = uboJointRows[ j * 3 + 2 ];
	return row0.xyz * n.x + row1.xyz * n.y + row2.xyz * n.z;
}

bool HF_HasFeature( const float feature ) {
	return mod( floor( rpUser0.x / feature ), 2.0 ) > 0.5;
}

void main() {
	vec3 skinnedPos = vec3( 0.0 );
	vec3 skinnedNormal = vec3( 0.0 );
	vec3 skinnedTangent = vec3( 0.0 );

	skinnedPos     += in_Color2.x * HF_BoneTransform( in_Position.xyz, in_Color.x );
	skinnedNormal  += in_Color2.x * HF_BoneRotate( in_Normal.xyz, in_Color.x );
	skinnedTangent += in_Color2.x * HF_BoneRotate( in_Tangent.xyz, in_Color.x );
	skinnedPos     += in_Color2.y * HF_BoneTransform( in_Position.xyz, in_Color.y );
	skinnedNormal  += in_Color2.y * HF_BoneRotate( in_Normal.xyz, in_Color.y );
	skinnedTangent += in_Color2.y * HF_BoneRotate( in_Tangent.xyz, in_Color.y );
	skinnedPos     += in_Color2.z * HF_BoneTransform( in_Position.xyz, in_Color.z );
	skinnedNormal  += in_Color2.z * HF_BoneRotate( in_Normal.xyz, in_Color.z );
	skinnedTangent += in_Color2.z * HF_BoneRotate( in_Tangent.xyz, in_Color.z );
	skinnedPos     += in_Color2.w * HF_BoneTransform( in_Position.xyz, in_Color.w );
	skinnedNormal  += in_Color2.w * HF_BoneRotate( in_Normal.xyz, in_Color.w );
	skinnedTangent += in_Color2.w * HF_BoneRotate( in_Tangent.xyz, in_Color.w );

	vec4 modelPos = rpModelMatrixX * skinnedPos.x
				  + rpModelMatrixY * skinnedPos.y
				  + rpModelMatrixZ * skinnedPos.z
				  + rpModelMatrixW;

	mat3 modelView3 = mat3( rpModelViewMatrixX.xyz,
							rpModelViewMatrixY.xyz,
							rpModelViewMatrixZ.xyz );

	vofi_TexCoord0 = in_TexCoord;
	vofi_TexCoord2 = normalize( modelView3 * normalize( skinnedNormal ) );
	vofi_TexCoord3 = normalize( modelView3 * normalize( skinnedTangent ) );
	// the color attribute is packed with the joint indices: never let it leak
	// into the fragment program as a vertex color
	vofi_Color = vec4( 1.0 );

	gl_Position = rpMVPmatrixX * modelPos.x
				+ rpMVPmatrixY * modelPos.y
				+ rpMVPmatrixZ * modelPos.z
				+ rpMVPmatrixW;

	if ( HF_HasFeature( F_TEXGEN ) ) {
		vofi_TexCoord1.xy = gl_Position.xy / gl_Position.w * 0.5 + 0.5;
		vofi_TexCoord1.z = 0.0;
	} else {
		vec4 viewPos = rpModelViewMatrixX * modelPos.x
					  + rpModelViewMatrixY * modelPos.y
					  + rpModelViewMatrixZ * modelPos.z
					  + rpModelViewMatrixW;
		if ( HF_HasFeature( F_LIGHTED ) && HF_HasFeature( F_PBR ) ) {
			vofi_TexCoord1 = viewPos.xyz;
		} else {
			vofi_TexCoord1 = modelPos.xyz;
		}
	}
}
)GLSL";

/*
==============
HF_UberGLSL_FragmentSource

Embedded native fragment program (shared by the regular and skinned vertex
programs). Feature flags live in rpUser0.x and are tested with integer
division so all combinations are handled by the single program. Interactions
select the F_DIFFUSEMOD / F_LIGHTED path; plain ambient stages modulate
unlit through rpColor exactly like the legacy material programs.
==============
*/
static const char * s_uberFragmentSource = R"GLSL(
#version 140

uniform vec4 rpColor;
uniform vec4 rpDiffuseModifier;
uniform vec4 rpSpecularModifier;
uniform vec4 rpOverbright;
uniform vec4 rpVertexColorModulate;
uniform vec4 rpVertexColorAdd;
uniform vec4 rpUser0;
uniform vec4 rpUser1;
uniform vec4 rpUser2;
uniform vec4 rpUser3;
uniform vec4 rpUser4;
uniform vec4 rpAlphaTest;
uniform vec4 rpLocalLightOrigin;
uniform vec4 rpLightFalloffS;

uniform sampler2D samp0;
uniform sampler2D samp1;
uniform sampler2D samp2;

in vec2 vofi_TexCoord0;
in vec3 vofi_TexCoord1;
in vec3 vofi_TexCoord2;
in vec3 vofi_TexCoord3;
in vec4 vofi_Color;

out vec4 fragColor;

const float F_DIFFUSE			= 1.0;
const float F_BUMP				= 2.0;
const float F_SPECULAR			= 4.0;
const float F_VERTEXCOLOR		= 8.0;
const float F_ALPHATEST			= 16.0;
const float F_TEXGEN			= 32.0;
const float F_DIFFUSEMOD		= 64.0;
const float F_LIGHTED			= 128.0;
const float F_PBR				= 256.0;

bool HF_HasFeature( const float feature ) {
	return mod( floor( rpUser0.x / feature ), 2.0 ) > 0.5;
}

void main() {
	vec2 uv = vofi_TexCoord0;
	if ( HF_HasFeature( F_TEXGEN ) ) {
		uv = vofi_TexCoord1.xy;
	}

	vec4 base;
	if ( HF_HasFeature( F_DIFFUSEMOD ) ) {
		base = rpDiffuseModifier;
	} else {
		base = rpColor;
	}

	if ( HF_HasFeature( F_DIFFUSE ) ) {
		base *= texture( samp0, uv );
	}

	if ( HF_HasFeature( F_VERTEXCOLOR ) ) {
		// rb_SetVertexColorParms semantics: MODULATE -> 1/0, INVERSE -> -1/1,
		// IGNORE -> 0/1, so the mask alone keeps SVC_IGNORE a no-op
		base *= vofi_Color * rpVertexColorModulate + rpVertexColorAdd;
	}

	if ( HF_HasFeature( F_ALPHATEST ) ) {
		if ( base.a < rpAlphaTest.x ) {
			discard;
		}
	}

	vec3 lit = base.rgb;
	if ( HF_HasFeature( F_LIGHTED ) ) {
		vec3 N = normalize( vofi_TexCoord2 );
		if ( HF_HasFeature( F_PBR ) ) {
			if ( HF_HasFeature( F_BUMP ) ) {
				vec4 bump = texture( samp1, uv );
				vec3 T = normalize( vofi_TexCoord3 );
				vec3 B = normalize( cross( N, T ) );
				N = normalize( T * ( bump.y * 2.0 - 1.0 ) + B * ( bump.x * 2.0 - 1.0 ) + N * bump.z );
			}
			vec3 viewPos = vofi_TexCoord1;
			vec3 V = normalize( -viewPos );
			vec3 L = normalize( rpLocalLightOrigin.xyz - viewPos );
			vec3 H = normalize( V + L );
			float NdotV = max( dot( N, V ), 0.0 );
			float NdotL = max( dot( N, L ), 0.0 );
			float NdotH = max( dot( N, H ), 0.0 );
			float VdotH = max( dot( V, H ), 0.0 );

			float roughness = clamp( rpUser1.x, 0.04, 1.0 );
			float metallic  = clamp( rpUser1.y, 0.0,  1.0 );
			float ao        = clamp( rpUser1.z, 0.0,  1.0 );

			vec3 albedo = base.rgb;
			vec3 F0 = mix( vec3( 0.04 ), albedo, metallic );

			float a2 = roughness * roughness;
			a2 = a2 * a2;
			float D = a2 / ( 3.14159265 * ( NdotH * NdotH * ( a2 - 1.0 ) + 1.0 ) * ( NdotH * NdotH * ( a2 - 1.0 ) + 1.0 ) );

			float k = ( roughness + 1.0 ); k = k * k / 8.0;
			float Gv = NdotV / ( NdotV * ( 1.0 - k ) + k );
			float Gl = NdotL / ( NdotL * ( 1.0 - k ) + k );
			float G = Gv * Gl;

			vec3 F = F0 + ( 1.0 - F0 ) * pow( 1.0 - VdotH, 5.0 );

			vec3 kD = ( 1.0 - F ) * ( 1.0 - metallic );
			vec3 diffuseContrib = kD * albedo / 3.14159265;

			float specIntensity = 1.0;
			if ( HF_HasFeature( F_SPECULAR ) ) {
				specIntensity = texture( samp2, uv ).x;
			}
			vec3 specContrib = D * G * F * specIntensity / max( 4.0 * NdotV * NdotL, 0.001 );

			vec3 direct = ( diffuseContrib + specContrib ) * NdotL * rpLightFalloffS.w;
			vec3 ambient = rpUser4.rgb * rpUser4.w * ao * albedo;
			vec3 emissive = albedo * rpUser1.w;

			lit = direct + ambient + emissive;
			lit *= rpOverbright.x;
		} else {
			if ( HF_HasFeature( F_BUMP ) ) {
				vec4 bump = texture( samp1, uv );
				vec3 T = normalize( vofi_TexCoord3 );
				vec3 B = normalize( cross( N, T ) );
				N = normalize( T * ( bump.y * 2.0 - 1.0 ) + B * ( bump.x * 2.0 - 1.0 ) + N * bump.z );
			}
			lit *= 0.5 + 0.5 * N.z;
			if ( HF_HasFeature( F_SPECULAR ) ) {
				float specular = texture( samp2, uv ).x * rpSpecularModifier.x;
				lit += specular * N.z;
			}
			lit *= rpOverbright.x;
		}
	}

	fragColor = vec4( lit, base.a );
}
)GLSL";

/*
==============
HF_UberGLSL_CompileShader
==============
*/
GLuint HF_UberGLSL_CompileShader( GLenum target, const char * name, hfList<int> & uniforms ) {
	static bool s_commandRegistered = false;
	if ( !s_commandRegistered ) {
		cmdSystem->AddCommand( "testUber", HF_UberGLSL_TestUber_f, CMD_FL_RENDERER,
			"testUber [programName] - dumps uber native-GLSL coverage and feature masks" );
		s_commandRegistered = true;
	}

	if ( !HF_UberGLSL_Covered( name ) ) {
		return HF_UBERGLSL_INVALID_SHADER;
	}

	// Broad GPU support: the native vertex sources need GLSL 1.40 (GL 3.1),
	// and the skinned variant additionally needs uniform buffer objects. On
	// older drivers the engine falls back to the converted programs.
	if ( !hfGLCaps::HasFlag( HF_GL_CAP_GLSL_140 ) ) {
		return HF_UBERGLSL_INVALID_SHADER;
	}
	if ( IsSkinnedProgram( name ) && !hfGLCaps::HasFlag( HF_GL_CAP_UBO ) ) {
		return HF_UBERGLSL_INVALID_SHADER;
	}

	// Native programs carry their uniforms by name; the array-based uniform
	// mechanism (r_useUniformArrays) is only used by the converted programs.
	uniforms.Clear();

	const char * src;
	if ( target == GL_FRAGMENT_SHADER ) {
		src = s_uberFragmentSource;
	} else if ( IsSkinnedProgram( name ) ) {
		src = s_uberVertexSkinnedSource;
	} else {
		src = s_uberVertexSource;
	}

	GLuint shader = qglCreateShader( target );
	if ( shader == 0 ) {
		hfLib::Printf( "HF_UberGLSL: qglCreateShader failed for %s\n", name );
		return HF_UBERGLSL_INVALID_SHADER;
	}

	GLint length = (GLint)strlen( src );
	const char * sourcePtr = src;
	qglShaderSource( shader, 1, &sourcePtr, &length );
	qglCompileShader( shader );

	GLint status = GL_FALSE;
	qglGetShaderiv( shader, GL_COMPILE_STATUS, &status );
	if ( status == GL_FALSE ) {
		char infoLog[2048] = {};
		GLint infoLen = 0;
		qglGetShaderInfoLog( shader, sizeof( infoLog ) - 1, &infoLen, infoLog );
		hfLib::Printf( "HF_UberGLSL: failed to compile %s program %s:\n%s\n", ( target == GL_FRAGMENT_SHADER ) ? "fragment" : "vertex", name, infoLog );
		qglDeleteShader( shader );
		return HF_UBERGLSL_INVALID_SHADER;
	}

	hfLib::Printf( "HF_UberGLSL: compiled native %s program %s\n", ( target == GL_FRAGMENT_SHADER ) ? "fragment" : "vertex", name );
	return shader;
}

/*
==============
HF_UberGLSL_StageFeatures
==============
*/
uint32 HF_UberGLSL_StageFeatures( const void * rawStage ) {
	const shaderStage_t * stage = static_cast< const shaderStage_t * >( rawStage );
	if ( stage == NULL || stage->newStage != NULL ) {
		return 0;
	}
	return HF_Uber::FeaturesFromStage(
		stage->texture.image != NULL && stage->texture.cinematic == NULL,
		stage->vertexColor == SVC_MODULATE || stage->vertexColor == SVC_INVERSE_MODULATE,
		stage->hasAlphaTest,
		stage->texture.texgen == TG_SCREEN || stage->texture.texgen == TG_SCREEN2 );
}

/*
==============
HF_UberGLSL_SetStageFeatures
==============
*/
void HF_UberGLSL_SetStageFeatures( const void * rawStage ) {
	const shaderStage_t * stage = static_cast< const shaderStage_t * >( rawStage );
	if ( stage != NULL && renderProgManager.UberNativeUsed() ) {
		uint32 f = HF_UberGLSL_StageFeatures( stage );
		if ( r_usePBR.GetBool() ) {
			f |= HF_UBER_PBR;
		}
		HF_UberGLSL_SetFeatures( f );
	}
}