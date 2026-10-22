#pragma once
/*
 hfOptimize — Pure, header-only logic for the adaptive FPS optimizer.

 Shares code between the live renderer (hfFpsOptimizer.cpp) and the CPU-only
 test harness (script/tests/hfstest_optimize.cpp).

 The quality ladder maps a numeric "quality level" 0..QUALITY_LEVELS-1 to a
 set of named cvar values. Level 0 is minimum quality (max performance);
 the highest level is maximum visual fidelity. The adaptive controller
 steps up/down through these levels to keep GPU time within a target budget.
*/

#include "../hflib/hfprecompiled.h"

namespace HF_Opt {

static const int QUALITY_LEVELS = 10;

// Feature indices into a quality row
enum FeatureIndex {
	FI_MSAA = 0,
	FI_SHADOWS,
	FI_SPECULAR,
	FI_BUMP,
	FI_SRGB,
	FI_BLOOM,
	FI_SSAO,
	FI_SSR,
	FI_DOF,
	FI_ANISO,
	FI_TRILINEAR,
	FI_LODBIAS,
	FI_COUNT
};

// One quality level — every field is a cvar value. Strings are numeric
// (0/1/2/4/8 etc.) so they can be pushed directly via SetInteger /
// SetFloat / SetValue on the target cvar.
struct QualityRow {
	const char * cvarNames[FI_COUNT];
	const char * cvarValues[FI_COUNT];
	const char * label; // human-readable name (e.g. "Performance")
};

// The ladder. Ordered 0 = lowest quality / highest performance through
// QUALITY_LEVELS-1 = max quality / lowest performance.
static const QualityRow kQualityRows[QUALITY_LEVELS] = {
	// cvar:  msaa  shadows specular bump  srgb bloom ssao ssr dof aniso trilinear lodBias  label
	{ { "r_multiSamples","r_skipShadows","r_skipSpecular","r_skipBump","r_useSRGB","r_bloom","r_ssao","r_ssr","r_depthOfField","r_maxAnisotropicFiltering","r_useTrilinearFiltering","r_lodBias" },
	  { "0",             "1",            "1",             "1",         "1",         "0",      "0",     "0",   "0",               "0",                      "0",                    "0.75"    },
	  "Ultra Performance" },
	{ { "r_multiSamples","r_skipShadows","r_skipSpecular","r_skipBump","r_useSRGB","r_bloom","r_ssao","r_ssr","r_depthOfField","r_maxAnisotropicFiltering","r_useTrilinearFiltering","r_lodBias" },
	  { "0",             "1",            "1",             "1",         "1",         "0",      "0",     "0",   "0",               "1",                      "0",                    "0.75"    },
	  "Performance" },
	{ { "r_multiSamples","r_skipShadows","r_skipSpecular","r_skipBump","r_useSRGB","r_bloom","r_ssao","r_ssr","r_depthOfField","r_maxAnisotropicFiltering","r_useTrilinearFiltering","r_lodBias" },
	  { "0",             "1",            "0",             "1",         "1",         "0",      "0",     "0",   "0",               "1",                      "0",                    "0.75"    },
	  "Balanced Low" },
	{ { "r_multiSamples","r_skipShadows","r_skipSpecular","r_skipBump","r_useSRGB","r_bloom","r_ssao","r_ssr","r_depthOfField","r_maxAnisotropicFiltering","r_useTrilinearFiltering","r_lodBias" },
	  { "0",             "0",            "0",             "1",         "1",         "0",      "0",     "0",   "0",               "2",                      "1",                    "0.5"     },
	  "Balanced" },
	{ { "r_multiSamples","r_skipShadows","r_skipSpecular","r_skipBump","r_useSRGB","r_bloom","r_ssao","r_ssr","r_depthOfField","r_maxAnisotropicFiltering","r_useTrilinearFiltering","r_lodBias" },
	  { "0",             "0",            "0",             "0",         "1",         "0",      "0",     "0",   "0",               "4",                      "1",                    "0.5"     },
	  "Quality" },
	{ { "r_multiSamples","r_skipShadows","r_skipSpecular","r_skipBump","r_useSRGB","r_bloom","r_ssao","r_ssr","r_depthOfField","r_maxAnisotropicFiltering","r_useTrilinearFiltering","r_lodBias" },
	  { "2",             "0",            "0",             "0",         "2",         "1",      "0",     "0",   "0",               "4",                      "1",                    "0.5"     },
	  "High" },
	{ { "r_multiSamples","r_skipShadows","r_skipSpecular","r_skipBump","r_useSRGB","r_bloom","r_ssao","r_ssr","r_depthOfField","r_maxAnisotropicFiltering","r_useTrilinearFiltering","r_lodBias" },
	  { "2",             "0",            "0",             "0",         "2",         "1",      "1",     "0",   "0",               "8",                      "1",                    "0.5"     },
	  "High+" },
	{ { "r_multiSamples","r_skipShadows","r_skipSpecular","r_skipBump","r_useSRGB","r_bloom","r_ssao","r_ssr","r_depthOfField","r_maxAnisotropicFiltering","r_useTrilinearFiltering","r_lodBias" },
	  { "4",             "0",            "0",             "0",         "2",         "1",      "1",     "1",   "0",               "8",                      "1",                    "0.5"     },
	  "Ultra" },
	{ { "r_multiSamples","r_skipShadows","r_skipSpecular","r_skipBump","r_useSRGB","r_bloom","r_ssao","r_ssr","r_depthOfField","r_maxAnisotropicFiltering","r_useTrilinearFiltering","r_lodBias" },
	  { "4",             "0",            "0",             "0",         "2",         "1",      "1",     "1",   "1",               "16",                     "1",                    "0"       },
	  "Ultra+" },
	{ { "r_multiSamples","r_skipShadows","r_skipSpecular","r_skipBump","r_useSRGB","r_bloom","r_ssao","r_ssr","r_depthOfField","r_maxAnisotropicFiltering","r_useTrilinearFiltering","r_lodBias" },
	  { "8",             "0",            "0",             "0",         "2",         "1",      "1",     "1",   "1",               "16",                     "1",                    "0"       },
	  "Nightmare" },
};

// Human-readable label for a quality level
inline const char * QualityLabel( int level ) {
	if ( level < 0 || level >= QUALITY_LEVELS ) { return "???"; }
	return kQualityRows[level].label;
}

// Target frame time in milliseconds given an FPS target
inline float TargetFrameMs( int targetFps ) {
	if ( targetFps <= 0 ) { return 16.6667f; }
	return 1000.0f / ( float ) targetFps;
}

// Clamp a quality level
inline int ClampQuality( int level, int minLevel, int maxLevel ) {
	if ( level < minLevel ) { return minLevel; }
	if ( level > maxLevel ) { return maxLevel; }
	return level;
}

// Over-budget ratio: when gpuMs exceeds budget * overBudget, step down.
// aggression ranges 0..1 (default 0.5) and widens the reaction window.
inline float OverBudgetRatio( float aggression ) {
	return 1.0f + aggression * 0.25f; // 1.0 .. 1.25
}

// Headroom ratio: when gpuMs is below budget * headroom, consider stepping up.
inline float HeadroomRatio( float aggression ) {
	return 1.0f - aggression * 0.3f; // 1.0 .. 0.7
}

// Return -1 (step down), 0 (hold), or +1 (step up) for a single GPU time
// sample relative to the quality budget. 'sinceStepUp' counts frames since
// the last upward step; 'stepUpFrames' is the minimum wait before stepping
// up is allowed.
inline int StepDirection( float gpuMs, float budgetMs, float aggression,
						 int sinceStepUp, int stepUpFrames ) {
	if ( gpuMs > budgetMs * OverBudgetRatio( aggression ) ) {
		return -1;
	}
	if ( sinceStepUp >= stepUpFrames && gpuMs < budgetMs * HeadroomRatio( aggression ) ) {
		return +1;
	}
	return 0;
}

// Recommended default quality level given GL capabilities (from hfGLCaps).
// When compute or UBO are missing the engine already falls back, but starting
// at a lower level on older drivers avoids initial hitches as the adaptive
// controller warms up.
inline int QualityLevelForCaps( int glMajor, int glMinor, bool compute, bool ubo ) {
	if ( glMajor >= 4 && glMinor >= 3 && compute ) {
		return 7; // Ultra  — full idTech7 feature set
	}
	if ( glMajor >= 4 ) {
		return 6; // High+
	}
	if ( glMajor >= 3 && glMinor >= 3 ) {
		return 5; // High
	}
	if ( glMajor >= 3 && ubo ) {
		return 4; // Quality
	}
	if ( glMajor >= 3 ) {
		return 3; // Balanced
	}
	return 1; // Performance fallback for GL 2.x
}

// Print a compact one-line quality snapshot into a provided buffer.
inline void QualitySnapshot( char * buf, int bufsize, int level, float gpuMs,
							 float targetMs, float resScale ) {
	hfStr::snPrintf( buf, bufsize, "FPS-Opt LVL %d/%d  GPU %4.1fms budget %4.1fms  RES %d%%",
		level, QUALITY_LEVELS - 1, gpuMs, targetMs, ( int ) ( resScale * 100.0f + 0.5f ) );
}

} // namespace HF_Opt