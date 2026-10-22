#include "hfFpsOptimizer.h"
#include "hfOptimize.h"
#include "hfGLCaps.h"
#include "hfResolutionScale.h"
#include "hfOpenGLPostprocess.h"
#include "hfRenderProgs.h"
#include "hfUberLogic.h"
#include "hftr_local.h"

// forward decl — defined at the bottom
static void opt_printStats_f( const hfCmdArgs &args );

hfFpsOptimizer fpsOptimizer;

/*
--------------------
Cvars
--------------------
*/
hfCVar opt_enable(        "opt_enable",        "1", CVAR_RENDERER | CVAR_BOOL,  "Enable adaptive FPS optimizer (0 = off)" );
hfCVar opt_targetFps(     "opt_targetFps",     "0", CVAR_RENDERER | CVAR_INTEGER, "Target FPS (0 = use com_engineHz / 60)" );
hfCVar opt_aggression(    "opt_aggression",    "0.5", CVAR_RENDERER | CVAR_FLOAT, "Optimizer reaction strength (0 = gentle, 1 = aggressive)" );
hfCVar opt_stepUpFrames(  "opt_stepUpFrames",  "8", CVAR_RENDERER | CVAR_INTEGER, "Consecutive below-budget frames before stepping quality up" );
hfCVar opt_minQuality(    "opt_minQuality",    "0", CVAR_RENDERER | CVAR_INTEGER, "Lowest allowed quality level (0 = Ultra Performance)" );
hfCVar opt_maxQuality(    "opt_maxQuality",    "9", CVAR_RENDERER | CVAR_INTEGER, "Highest allowed quality level (9 = Nightmare)" );
hfCVar opt_show(          "opt_show",          "0", CVAR_RENDERER | CVAR_INTEGER, "Optimizer overlay (0 = off, 1 = per-frame print, 2 = stats line)" );
hfCVar opt_warmup(        "opt_warmup",        "1", CVAR_RENDERER | CVAR_BOOL,  "Pre-compile all uber shaders at init to eliminate hitches" );
hfCVar opt_applyPreset(   "opt_applyPreset",   "-1", CVAR_RENDERER | CVAR_INTEGER, "Apply a fixed quality level (-1 = use adaptive, 0..9 = locked)" );
// Read-only status cvar: updated every frame by the optimizer so the console
// overlay can draw the current quality level without a header dependency.
hfCVar opt_level(         "opt_level",         "4", CVAR_RENDERER | CVAR_INTEGER | CVAR_ROM, "Current optimizer quality level (read-only)" );

/*
 hfFpsOptimizer constructor
*/
hfFpsOptimizer::hfFpsOptimizer() {
	currentQualityLevel = 4; // "Balanced" as safe default
	minQualityLevel     = 0;
	maxQualityLevel     = HF_Opt::QUALITY_LEVELS - 1;
	lastGpuMs           = 0.0f;
	framesBelowBudget   = 0;
	lastAppliedLevel    = -1;
}

/*
 ApplyCapabilityDefaults

 Called once from R_InitOpenGL after hfGLCaps and renderProgManager are
 initialised. Picks the right starting quality level based on GPU caps.
*/
void hfFpsOptimizer::ApplyCapabilityDefaults() {
	bool compute = hfGLCaps::HasFlag( HF_GL_CAP_COMPUTE );
	bool ubo     = hfGLCaps::HasFlag( HF_GL_CAP_UBO );
	int glMajor  = hfGLCaps::GetGLMajorVersion();
	int glMinor  = hfGLCaps::GetGLMinorVersion();

	currentQualityLevel = HF_Opt::QualityLevelForCaps( glMajor, glMinor, compute, ubo );
	minQualityLevel = HF_Opt::ClampQuality( opt_minQuality.GetInteger(), 0, HF_Opt::QUALITY_LEVELS - 1 );
	maxQualityLevel = HF_Opt::ClampQuality( opt_maxQuality.GetInteger(), 0, HF_Opt::QUALITY_LEVELS - 1 );
	currentQualityLevel = HF_Opt::ClampQuality( currentQualityLevel, minQualityLevel, maxQualityLevel );

	ApplyQualityLevel( currentQualityLevel );

	hfLib::Printf( "hfFpsOptimizer: GL %d.%d caps -> quality level %d (%s)\n",
		glMajor, glMinor, currentQualityLevel, HF_Opt::QualityLabel( currentQualityLevel ) );
}

/*
 Warmup

 Pre-compiles all native uber material shaders and the compute post shader so
 the first real frame doesn't stall on GL shader compilation. Called once
 during R_InitOpenGL.
*/
void hfFpsOptimizer::Warmup() {
	static bool s_registered = false;
	if ( !s_registered ) {
		cmdSystem->AddCommand( "opt_printStats", opt_printStats_f, CMD_FL_RENDERER,
			"prints the FPS optimizer status line" );
		s_registered = true;
	}

	if ( !opt_warmup.GetBool() ) {
		return;
	}

	// Uber material shaders — iterate every covered builtin and force both
	// vertex and fragment native compilation through the normal load path.
	const char * const * list = HF_Uber::CoveredList();
	int compiled = 0;
	for ( int i = 0; list[i] != NULL; i++ ) {
		if ( renderProgManager.FindVertexShader( list[i] ) >= 0 ) {
			compiled++;
		}
		if ( renderProgManager.FindFragmentShader( list[i] ) >= 0 ) {
			compiled++;
		}
	}
	hfLib::Printf( "hfFpsOptimizer: warmup compiled %d uber shaders\n", compiled );

	// Compute post shader (bloom + SSAO + SSR + DoF) — force the link now
	// instead of the first frame with an actual post-process pass.
	if ( hfComputePost_Available() ) {
		hfLib::Printf( "hfFpsOptimizer: warmup compute post program ready\n" );
	}
}

/*
 ApplyQualityLevel

 Writes the cvar values for the given quality level. Only writes cvars that
 actually changed to avoid needless texture/image reloads.
*/
void hfFpsOptimizer::ApplyQualityLevel( int level ) {
	level = HF_Opt::ClampQuality( level, minQualityLevel, maxQualityLevel );
	if ( level == lastAppliedLevel ) {
		return;
	}
	lastAppliedLevel = level;
	currentQualityLevel = level;

	const HF_Opt::QualityRow & row = HF_Opt::kQualityRows[level];

	for ( int i = 0; i < HF_Opt::FI_COUNT; i++ ) {
		hfCVar * cv = cvarSystem->Find( row.cvarNames[i] );
		if ( cv == NULL ) {
			continue;
		}
		const char * val = row.cvarValues[i];
		// r_lodBias is a float; everything else is integer
		if ( i == HF_Opt::FI_LODBIAS ) {
			cv->SetFloat( atof( val ) );
		} else {
			int iv = atoi( val );
			cv->SetInteger( iv );
		}
	}

	if ( opt_show.GetInteger() > 0 ) {
		hfLib::Printf( "hfFpsOptimizer: quality level -> %d (%s)\n", level, HF_Opt::QualityLabel( level ) );
	}

	// Keep the display cvar in sync so the console overlay reflects manual
	// changes (vid_restart, manual opt_applyPreset set, etc.)
	hfCVar * lvl = cvarSystem->Find( "opt_level" );
	if ( lvl ) { lvl->SetInteger( level ); }
}

/*
 AdjustDynamicResolution

 Feeds the existing hfResolutionScale with thresholds derived from the
 current FPS target and aggression setting. The resolution axis runs
 independently of the quality axis. rs_* cvars are found by name because only
 hfResolutionScale.cpp owns them.
*/
void hfFpsOptimizer::AdjustDynamicResolution( int gpuMicroSec ) {
	if ( gpuMicroSec <= 0 ) {
		return;
	}
	// Compute target frame time and write it into the resolution-scale cvars
	// so the existing rs_enable path adjusts resolution proportionally.
	float budgetMs = HF_Opt::TargetFrameMs( opt_targetFps.GetInteger() );
	float agg = opt_aggression.GetFloat();

	float dropMs  = budgetMs * HF_Opt::OverBudgetRatio( agg );
	float raiseMs = budgetMs * HF_Opt::HeadroomRatio( agg );

	hfCVar * rsDrop    = cvarSystem->Find( "rs_dropMilliseconds" );
	hfCVar * rsRaise   = cvarSystem->Find( "rs_raiseMilliseconds" );
	hfCVar * rsDropFr  = cvarSystem->Find( "rs_dropFraction" );
	hfCVar * rsRaiseFr = cvarSystem->Find( "rs_raiseFraction" );
	hfCVar * rsRaiseF  = cvarSystem->Find( "rs_raiseFrames" );
	hfCVar * rsEnable  = cvarSystem->Find( "rs_enable" );

	if ( rsDrop )    { rsDrop->SetFloat( dropMs ); }
	if ( rsRaise )   { rsRaise->SetFloat( raiseMs ); }
	if ( rsDropFr )  { rsDropFr->SetFloat( 0.08f + agg * 0.10f ); }
	if ( rsRaiseFr ) { rsRaiseFr->SetFloat( 0.03f + ( 1.0f - agg ) * 0.04f ); }
	if ( rsRaiseF )  { rsRaiseF->SetInteger( ( int ) ( 6 + ( 1.0f - agg ) * 10 ) ); }
	if ( rsEnable )  { rsEnable->SetInteger( 3 ); } // both axes dynamic
}

/*
 FrameSample

 Called once per frame from the backend with the latest GPU timing.
*/
void hfFpsOptimizer::FrameSample( int gpuMicroSec ) {
	if ( !opt_enable.GetBool() ) {
		return;
	}

	lastGpuMs = gpuMicroSec * 0.001f;
	float budgetMs = HF_Opt::TargetFrameMs( opt_targetFps.GetInteger() );
	float agg = HF_Opt::ClampQuality( ( int ) ( opt_aggression.GetFloat() * 10.0f ), 0, 10 ) / 10.0f;

	// Fixed-preset override: when opt_applyPreset >= 0, lock to that level
	// and skip adaptive logic.
	int fixedPreset = opt_applyPreset.GetInteger();
	if ( fixedPreset >= 0 ) {
		int locked = HF_Opt::ClampQuality( fixedPreset, minQualityLevel, maxQualityLevel );
		if ( locked != currentQualityLevel ) {
			ApplyQualityLevel( locked );
		}
		AdjustDynamicResolution( gpuMicroSec );
		return;
	}

	// No valid timing — keep the current quality, don't thrash.
	if ( gpuMicroSec <= 0 ) {
		return;
	}

	// Determine step direction
	int dir = HF_Opt::StepDirection( lastGpuMs, budgetMs, agg,
		framesBelowBudget, opt_stepUpFrames.GetInteger() );

	if ( dir < 0 ) {
		// Over budget — drop quality immediately.
		// Also drop framesBelowBudget so we don't immediately re-raise.
		framesBelowBudget = 0;
		int newLevel = HF_Opt::ClampQuality( currentQualityLevel - 1, minQualityLevel, maxQualityLevel );
		if ( newLevel != currentQualityLevel ) {
			// Reset resolution scale on quality drop to give it a fresh
			// chance to re-adapt at the new quality cost.
			resolutionScale.ResetToFullResolution();
			ApplyQualityLevel( newLevel );
		}
	} else if ( dir > 0 ) {
		// Under budget — step up
		framesBelowBudget = 0;
		int newLevel = HF_Opt::ClampQuality( currentQualityLevel + 1, minQualityLevel, maxQualityLevel );
		if ( newLevel != currentQualityLevel ) {
			resolutionScale.ResetToFullResolution();
			ApplyQualityLevel( newLevel );
		}
	} else {
		framesBelowBudget++;
	}

	AdjustDynamicResolution( gpuMicroSec );

	// Update the read-only display cvar so the console overlay can show the
	// current quality level without pulling in the optimizer header.
	hfCVar * lvl = cvarSystem->Find( "opt_level" );
	if ( lvl ) { lvl->SetInteger( currentQualityLevel ); }

	// Optimizer stats line (when opt_show == 2, printed once per frame)
	if ( opt_show.GetInteger() >= 2 ) {
		PrintFrameStats();
	}
}

/*
 PrintFrameStats
*/
void hfFpsOptimizer::PrintFrameStats() const {
	float x, y;
	resolutionScale.GetCurrentResolutionScale( x, y );
	float resScale = ( x + y ) * 0.5f;
	char buf[256];
	HF_Opt::QualitySnapshot( buf, sizeof( buf ), currentQualityLevel, lastGpuMs,
		HF_Opt::TargetFrameMs( opt_targetFps.GetInteger() ), resScale );
	hfLib::Printf( "%s\n", buf );
}

/*
 GetStatusText
*/
void hfFpsOptimizer::GetStatusText( hfStr &s ) const {
	float x, y;
	resolutionScale.GetCurrentResolutionScale( x, y );
	float resScale = ( x + y ) * 0.5f;
	s = va( "OPT LVL %d/%d (%s)  GPU %4.1fms  RES %d%%  target %dfps",
		currentQualityLevel, HF_Opt::QUALITY_LEVELS - 1,
		HF_Opt::QualityLabel( currentQualityLevel ),
		lastGpuMs,
		( int ) ( resScale * 100.0f + 0.5f ),
		opt_targetFps.GetInteger() > 0 ? opt_targetFps.GetInteger() : 60 );
}

/*
 opt_printStats_f — console command registered by Warmup().
*/
static void opt_printStats_f( const hfCmdArgs &args ) {
	hfStr s;
	fpsOptimizer.GetStatusText( s );
	hfLib::Printf( "%s\n", s.c_str() );
}