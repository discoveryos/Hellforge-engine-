#pragma hdrstop
#include "../hflib/hfprecompiled.h"

#include "hftr_local.h"

#include "hfUberGLSL.h"



hfRenderProgManager renderProgManager;

/*
================================================================================================
hfRenderProgManager::hfRenderProgManager()
================================================================================================
*/
hfRenderProgManager::hfRenderProgManager() {
}

/*
================================================================================================
hfRenderProgManager::~hfRenderProgManager()
================================================================================================
*/
hfRenderProgManager::~hfRenderProgManager() {
}

/*
================================================================================================
R_ReloadShaders
================================================================================================
*/
static void R_ReloadShaders( const hfCmdArgs &args ) {	
	renderProgManager.KillAllShaders();
	renderProgManager.LoadAllShaders();
}

/*
================================================================================================
hfRenderProgManager::Init()
================================================================================================
*/
void hfRenderProgManager::Init() {
	common->Printf( "----- Initializing Render Shaders -----\n" );


	for ( int i = 0; i < MAX_BUILTINS; i++ ) {
		builtinShaders[i] = -1;
	}
	struct builtinShaders_t {
		int index;
		const char * name;
	} builtins[] = {
		{ BUILTIN_GUI, "gui.vfp" },
		{ BUILTIN_COLOR, "color.vfp" },
		{ BUILTIN_SIMPLESHADE, "simpleshade.vfp" },
		{ BUILTIN_TEXTURED, "texture.vfp" },
		{ BUILTIN_TEXTURE_VERTEXCOLOR, "texture_color.vfp" },
		{ BUILTIN_TEXTURE_VERTEXCOLOR_SKINNED, "texture_color_skinned.vfp" },
		{ BUILTIN_TEXTURE_TEXGEN_VERTEXCOLOR, "texture_color_texgen.vfp" },
		{ BUILTIN_INTERACTION, "interaction.vfp" },
		{ BUILTIN_INTERACTION_SKINNED, "interaction_skinned.vfp" },
		{ BUILTIN_INTERACTION_AMBIENT, "interactionAmbient.vfp" },
		{ BUILTIN_INTERACTION_AMBIENT_SKINNED, "interactionAmbient_skinned.vfp" },
		{ BUILTIN_ENVIRONMENT, "environment.vfp" },
		{ BUILTIN_ENVIRONMENT_SKINNED, "environment_skinned.vfp" },
		{ BUILTIN_BUMPY_ENVIRONMENT, "bumpyEnvironment.vfp" },
		{ BUILTIN_BUMPY_ENVIRONMENT_SKINNED, "bumpyEnvironment_skinned.vfp" },

		{ BUILTIN_DEPTH, "depth.vfp" },
		{ BUILTIN_DEPTH_SKINNED, "depth_skinned.vfp" },
		{ BUILTIN_SHADOW_DEBUG, "shadowDebug.vfp" },
		{ BUILTIN_SHADOW_DEBUG_SKINNED, "shadowDebug_skinned.vfp" },

		{ BUILTIN_BLENDLIGHT, "blendlight.vfp" },
		{ BUILTIN_FOG, "fog.vfp" },
		{ BUILTIN_FOG_SKINNED, "fog_skinned.vfp" },
		{ BUILTIN_SKYBOX, "skybox.vfp" },
		{ BUILTIN_WOBBLESKY, "wobblesky.vfp" },
		{ BUILTIN_POSTPROCESS, "postprocess.vfp" },
		{ BUILTIN_STEREO_DEGHOST, "stereoDeGhost.vfp" },
		{ BUILTIN_STEREO_WARP, "stereoWarp.vfp" },
		{ BUILTIN_ZCULL_RECONSTRUCT, "zcullReconstruct.vfp" },
		{ BUILTIN_BINK, "bink.vfp" },
		{ BUILTIN_BINK_GUI, "bink_gui.vfp" },
		{ BUILTIN_STEREO_INTERLACE, "stereoInterlace.vfp" },
		{ BUILTIN_MOTION_BLUR, "motionBlur.vfp" },
	};
	int numBuiltins = sizeof( builtins ) / sizeof( builtins[0] );
	vertexShaders.SetNum( numBuiltins );
	fragmentShaders.SetNum( numBuiltins );
	glslPrograms.SetNum( numBuiltins );

	for ( int i = 0; i < numBuiltins; i++ ) {
		vertexShaders[i].name = builtins[i].name;
		fragmentShaders[i].name = builtins[i].name;
		builtinShaders[builtins[i].index] = i;
		LoadVertexShader( i );
		LoadFragmentShader( i );
		LoadGLSLProgram( i, i, i );
	}

	// Special case handling for fastZ shaders
	builtinShaders[BUILTIN_SHADOW] = FindVertexShader( "shadow.vp" );
	builtinShaders[BUILTIN_SHADOW_SKINNED] = FindVertexShader( "shadow_skinned.vp" );

	FindGLSLProgram( "shadow.vp", builtinShaders[BUILTIN_SHADOW], -1 );
	FindGLSLProgram( "shadow_skinned.vp", builtinShaders[BUILTIN_SHADOW_SKINNED], -1 );

	glslUniforms.SetNum( RENDERPARM_USER + MAX_GLSL_USER_PARMS, vec4_zero );

	vertexShaders[builtinShaders[BUILTIN_TEXTURE_VERTEXCOLOR_SKINNED]].usesJoints = true;
	vertexShaders[builtinShaders[BUILTIN_INTERACTION_SKINNED]].usesJoints = true;
	vertexShaders[builtinShaders[BUILTIN_INTERACTION_AMBIENT_SKINNED]].usesJoints = true;
	vertexShaders[builtinShaders[BUILTIN_ENVIRONMENT_SKINNED]].usesJoints = true;
	vertexShaders[builtinShaders[BUILTIN_BUMPY_ENVIRONMENT_SKINNED]].usesJoints = true;
	vertexShaders[builtinShaders[BUILTIN_DEPTH_SKINNED]].usesJoints = true;
	vertexShaders[builtinShaders[BUILTIN_SHADOW_SKINNED]].usesJoints = true;
	vertexShaders[builtinShaders[BUILTIN_SHADOW_DEBUG_SKINNED]].usesJoints = true;
	vertexShaders[builtinShaders[BUILTIN_FOG_SKINNED]].usesJoints = true;

	cmdSystem->AddCommand( "reloadShaders", R_ReloadShaders, CMD_FL_RENDERER, "reloads shaders" );
}

/*
================================================================================================
hfRenderProgManager::LoadAllShaders()
================================================================================================
*/
void hfRenderProgManager::LoadAllShaders() {
	for ( int i = 0; i < vertexShaders.Num(); i++ ) {
		LoadVertexShader( i );
	}
	for ( int i = 0; i < fragmentShaders.Num(); i++ ) {
		LoadFragmentShader( i );
	}

	for ( int i = 0; i < glslPrograms.Num(); ++i ) {
		LoadGLSLProgram( i, glslPrograms[i].vertexShaderIndex, glslPrograms[i].fragmentShaderIndex );
	}
}

/*
================================================================================================
hfRenderProgManager::KillAllShaders()
================================================================================================
*/
void hfRenderProgManager::KillAllShaders() {
	Unbind();
	for ( int i = 0; i < vertexShaders.Num(); i++ ) {
		if ( vertexShaders[i].progId != INVALID_PROGID ) {
			qglDeleteShader( vertexShaders[i].progId );
			vertexShaders[i].progId = INVALID_PROGID;
		}
	}
	for ( int i = 0; i < fragmentShaders.Num(); i++ ) {
		if ( fragmentShaders[i].progId != INVALID_PROGID ) {
			qglDeleteShader( fragmentShaders[i].progId );
			fragmentShaders[i].progId = INVALID_PROGID;
		}
	}
	for ( int i = 0; i < glslPrograms.Num(); ++i ) {
		if ( glslPrograms[i].progId != INVALID_PROGID ) {
			qglDeleteProgram( glslPrograms[i].progId );
			glslPrograms[i].progId = INVALID_PROGID;
		}
	}
}

/*
================================================================================================
hfRenderProgManager::Shutdown()
================================================================================================
*/
void hfRenderProgManager::Shutdown() {
	KillAllShaders();
}

/*
================================================================================================
hfRenderProgManager::FindVertexShader
================================================================================================
*/
int hfRenderProgManager::FindVertexShader( const char * name ) {
	for ( int i = 0; i < vertexShaders.Num(); i++ ) {
		if ( vertexShaders[i].name.Icmp( name ) == 0 ) {
			LoadVertexShader( i );
			return i;
		}
	}
	vertexShader_t shader;
	shader.name = name;
	int index = vertexShaders.Append( shader );
	LoadVertexShader( index );
	currentVertexShader = index;

	// FIXME: we should really scan the program source code for using rpEnableSkinning but at this
	// point we directly load a binary and the program source code is not available on the consoles
	if (	hfStr::Icmp( name, "heatHaze.vfp" ) == 0 ||
			hfStr::Icmp( name, "heatHazeWithMask.vfp" ) == 0 ||
			hfStr::Icmp( name, "heatHazeWithMaskAndVertex.vfp" ) == 0 ) {
		vertexShaders[index].usesJoints = true;
		vertexShaders[index].optionalSkinning = true;
	}

	return index;
}

/*
================================================================================================
hfRenderProgManager::FindFragmentShader
================================================================================================
*/
int hfRenderProgManager::FindFragmentShader( const char * name ) {
	for ( int i = 0; i < fragmentShaders.Num(); i++ ) {
		if ( fragmentShaders[i].name.Icmp( name ) == 0 ) {
			LoadFragmentShader( i );
			return i;
		}
	}
	fragmentShader_t shader;
	shader.name = name;
	int index = fragmentShaders.Append( shader );
	LoadFragmentShader( index );
	currentFragmentShader = index;
	return index;
}




/*
================================================================================================
hfRenderProgManager::LoadVertexShader
================================================================================================
*/
void hfRenderProgManager::LoadVertexShader( int index ) {
	if ( vertexShaders[index].progId != INVALID_PROGID ) {
		return; // Already loaded
	}

	// idTech 6 style: prefer the hand-written native GLSL uber material shader
	// over the runtime Cg->GLSL conversion for the common material builtins.
	if ( r_useNativeGLSL.GetBool() ) {
		GLuint native = HF_UberGLSL_CompileShader( GL_VERTEX_SHADER, vertexShaders[index].name, vertexShaders[index].uniforms );
		if ( native != HF_UBERGLSL_INVALID_SHADER ) {
			vertexShaders[index].progId = native;
			vertexShaders[index].nativeGLSL = true;
			return;
		}
		common->Printf( "HF_UberGLSL: falling back to Cg->GLSL conversion for %s (vertex)\n", vertexShaders[index].name.c_str() );
	}

	vertexShaders[index].progId = ( GLuint ) LoadGLSLShader( GL_VERTEX_SHADER, vertexShaders[index].name, vertexShaders[index].uniforms );
}

/*
================================================================================================
hfRenderProgManager::LoadFragmentShader
================================================================================================
*/
void hfRenderProgManager::LoadFragmentShader( int index ) {
	if ( fragmentShaders[index].progId != INVALID_PROGID ) {
		return; // Already loaded
	}

	// idTech 6 style: prefer the hand-written native GLSL uber material shader.
	if ( r_useNativeGLSL.GetBool() ) {
		GLuint native = HF_UberGLSL_CompileShader( GL_FRAGMENT_SHADER, fragmentShaders[index].name, fragmentShaders[index].uniforms );
		if ( native != HF_UBERGLSL_INVALID_SHADER ) {
			fragmentShaders[index].progId = native;
			fragmentShaders[index].nativeGLSL = true;
			return;
		}
		common->Printf( "HF_UberGLSL: falling back to Cg->GLSL conversion for %s (fragment)\n", fragmentShaders[index].name.c_str() );
	}

	fragmentShaders[index].progId = ( GLuint ) LoadGLSLShader( GL_FRAGMENT_SHADER, fragmentShaders[index].name, fragmentShaders[index].uniforms );
}

/*
================================================================================================
hfRenderProgManager::LoadShader
================================================================================================
*/
GLuint hfRenderProgManager::LoadShader( GLenum target, const char * name, const char * startToken ) {

	hfStr fullPath = "renderprogs\\gl\\";
	fullPath += name;

	common->Printf( "%s", fullPath.c_str() );

	char * fileBuffer = NULL;
	fileSystem->ReadFile( fullPath.c_str(), (void **)&fileBuffer, NULL );
	if ( fileBuffer == NULL ) {
		common->Printf( ": File not found\n" );
		return INVALID_PROGID;
	}
	if ( !R_IsInitialized() ) {
		common->Printf( ": Renderer not initialized\n" );
		fileSystem->FreeFile( fileBuffer );
		return INVALID_PROGID;
	}

	// vertex and fragment shaders are both be present in a single file, so
	// scan for the proper header to be the start point, and stamp a 0 in after the end
	char * start = strstr( (char *)fileBuffer, startToken );
	if ( start == NULL ) {
		common->Printf( ": %s not found\n", startToken );
		fileSystem->FreeFile( fileBuffer );
		return INVALID_PROGID;
	}
	char * end = strstr( start, "END" );
	if ( end == NULL ) {
		common->Printf( ": END not found for %s\n", startToken );
		fileSystem->FreeFile( fileBuffer );
		return INVALID_PROGID;
	}
	end[3] = 0;

	hfStr program = start;
	program.Replace( "vertex.normal", "vertex.attrib[11]" );
	program.Replace( "vertex.texcoord[0]", "vertex.attrib[8]" );
	program.Replace( "vertex.texcoord", "vertex.attrib[8]" );

	GLuint progId;
	qglGenProgramsARB( 1, &progId );

	qglBindProgramARB( target, progId );
	qglGetError();

	qglProgramStringARB( target, GL_PROGRAM_FORMAT_ASCII_ARB, program.Length(), program.c_str() );
	GLenum err = qglGetError();

	GLint ofs = -1;
	qglGetIntegerv( GL_PROGRAM_ERROR_POSITION_ARB, &ofs );
	if ( ( err == GL_INVALID_OPERATION ) || ( ofs != -1 ) ) {
		if ( err == GL_INVALID_OPERATION ) {
			const GLubyte * str = qglGetString( GL_PROGRAM_ERROR_STRING_ARB );
			common->Printf( "\nGL_PROGRAM_ERROR_STRING_ARB: %s\n", str );
		} else {
			common->Printf( "\nUNKNOWN ERROR\n" );
		}
		if ( ofs < 0 ) {
			common->Printf( "GL_PROGRAM_ERROR_POSITION_ARB < 0\n" );
		} else if ( ofs >= program.Length() ) {
			common->Printf( "error at end of shader\n" );
		} else {
			common->Printf( "error at %i:\n%s", ofs, program.c_str() + ofs );
		}
		qglDeleteProgramsARB( 1, &progId );
		fileSystem->FreeFile( fileBuffer );
		return INVALID_PROGID;
	}
	common->Printf( "\n" );
	fileSystem->FreeFile( fileBuffer );
	return progId;
}

/*
================================================================================================
hfRenderProgManager::BindShader
================================================================================================
*/
void hfRenderProgManager::BindShader( int vIndex, int fIndex ) {
	// Native uber material programs carry their feature mask in rpUser0, which
	// is zeroed at the start of every frame. Reset it on every bind, even when
	// the same pair is already active, so a draw that did not restate its
	// features still gets the default for the bound program.
	if ( vIndex >= 0 && vIndex < vertexShaders.Num() && currentVertexShader != vIndex ) {
		if ( vertexShaders[vIndex].nativeGLSL ) {
			HF_UberGLSL_SetFeatures( HF_UberGLSL_DefaultFeaturesFor( vertexShaders[vIndex].name ) );
		}
	}
	if ( currentVertexShader == vIndex && currentFragmentShader == fIndex ) {
		return;
	}
	currentVertexShader = vIndex;
	currentFragmentShader = fIndex;
	// vIndex denotes the GLSL program
	if ( vIndex >= 0 && vIndex < glslPrograms.Num() ) {
		currentRenderProgram = vIndex;
		RENDERLOG_PRINTF( "Binding GLSL Program %s\n", glslPrograms[vIndex].name.c_str() );
		qglUseProgram( glslPrograms[vIndex].progId );
	}
}

/*
================================================================================================
hfRenderProgManager::Unbind
================================================================================================
*/
void hfRenderProgManager::Unbind() {
	currentVertexShader = -1;
	currentFragmentShader = -1;

	qglUseProgram( 0 );
}

/*
================================================================================================
hfRenderProgManager::SetRenderParms
================================================================================================
*/
void hfRenderProgManager::SetRenderParms( renderParm_t rp, const float * value, int num ) {
	for ( int i = 0; i < num; i++ ) {
		SetRenderParm( (renderParm_t)(rp + i), value + ( i * 4 ) );
	}
}

/*
================================================================================================
hfRenderProgManager::SetRenderParm
================================================================================================
*/
void hfRenderProgManager::SetRenderParm( renderParm_t rp, const float * value ) {
	SetUniformValue( rp, value );
}


