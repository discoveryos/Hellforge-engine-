#pragma hdrstop
#include "../hflib/hfprecompiled.h"

// Material / shader editing CLI (no GUI; no display needed). A console loop
// for inspecting materials, decoding the native-GLSL feature mask each stage
// maps to, and force-compiling uber programs so shader hits happen in one
// cache-warm batch (or at an explicit point during editing).

#include "hfShaderTool.h"

#include "hftr_local.h"
#include "hfDeclManager.h"
#include "hfMaterial.h"
#include "hfRenderProgs.h"
#include "hfUberGLSL.h"
#include "hfOpenGLPostprocess.h"
#include "hfCVarSystem.h"
#include "hfCmdSystem.h"

static hfCVar r_showFeatBits( "r_showFeatBits", "0", CVAR_RENDERER | CVAR_BOOL, "toggle full feature bit dump in matCheck" );

static void hfMatList_f( const hfCmdArgs & args ) {
	if ( !declManager ) { return; }
	int num = declManager->GetNumDecls( DECL_MATERIAL );
	hfLib::Printf( "--- %d materials ---\n", num );
	for ( int i = 0; i < num; i++ ) {
		const hfDecl *decl = declManager->DeclByIndex( DECL_MATERIAL, i, false );
		if ( decl ) {
			hfLib::Printf( "%4d  %s\n", i, decl->GetName() );
		}
	}
}

static void hfMatCheck_f( const hfCmdArgs & args ) {
	if ( !declManager ) { return; }
	if ( args.Argc() < 2 ) {
		hfLib::Printf( "usage: matCheck <materialName>\n" );
		return;
	}
	const hfMaterial * mat = declManager->FindMaterial( args.Argv( 1 ), false );
	if ( !mat ) {
		hfLib::Printf( "matCheck: material '%s' not found\n", args.Argv( 1 ) );
		return;
	}
	hfLib::Printf( "material: %s (%d stages)\n", mat->GetName(), mat->GetNumStages() );
	for ( int s = 0; s < mat->GetNumStages(); s++ ) {
		uint32 f = HF_UberGLSL_StageFeatures( mat->GetStage( s ) );
		if ( r_usePBR.GetBool() ) { f |= HF_UBER_PBR; }
		hfLib::Printf( "  stage %d: featureMask=%08x", s, f );
		if ( r_showFeatBits.GetBool() ) {
			hfStr bits;
			for ( int b = 0; b < 32; b++ ) {
				if ( f & ( 1u << b ) ) { bits += va( " %d", b ); }
			}
			hfLib::Printf( " bits[%s]", bits.c_str() );
		}
		hfLib::Printf( "\n" );
	}
}

static void hfShaderCompile_f( const hfCmdArgs & args ) {
	// Warm the native uber pipeline: vertex + fragment for every covered
	// builtin (or a single named one), plus the compute post program.
	const char * want = ( args.Argc() >= 2 ) ? args.Argv( 1 ) : NULL;
	const char * const * list = HF_Uber::CoveredList();
	int compiled = 0;
	for ( int i = 0; list[i] != NULL; i++ ) {
		if ( want && hfStr::Icmp( want, "all" ) != 0 && hfStr::Icmp( want, list[i] ) != 0 ) {
			continue;
		}
		if ( renderProgManager.FindVertexShader( list[i] ) >= 0 ) { compiled++; }
		if ( renderProgManager.FindFragmentShader( list[i] ) >= 0 ) { compiled++; }
	}
	if ( !want || hfStr::Icmp( want, "all" ) == 0 ) {
		if ( hfComputePost_Available() ) { compiled += 2; }
	}
	hfLib::Printf( "shaderCompile: %d native programs compiled (%s)\n",
		compiled, want ? want : "all" );
}

void HF_ShaderTool_RegisterCommands() {
	static bool sDone = false;
	if ( sDone ) { return; }
	sDone = true;
	if ( cmdSystem ) {
		cmdSystem->AddCommand( "matList", hfMatList_f, CMD_FL_RENDERER, "list all materials" );
		cmdSystem->AddCommand( "matCheck", hfMatCheck_f, CMD_FL_RENDERER, "decode a material's native-GLSL stage features" );
		cmdSystem->AddCommand( "shaderCompile", hfShaderCompile_f, CMD_FL_RENDERER, "force-compile a material's uber programs" );
	}
}