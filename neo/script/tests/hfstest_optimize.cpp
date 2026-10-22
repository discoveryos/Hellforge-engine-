/*
===========================================================================

CPU-only unit tests for the pure HF_Opt FPS optimizer logic.

Covers:
  - Target frame time computation
  - Quality-level clamping
  - Over-budget / headroom ratios
  - Step direction state machine
  - GL-capability -> quality-level mapping
  - Quality label table integrity

===========================================================================
*/
#include "../../hflib/hfprecompiled.h"
#include "../../renderer/hfOptimize.h"

static int g_optFailures = 0;

#define OPT_CHECK( cond )														\
	do {																			\
		if ( !( cond ) ) {															\
			printf( "  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond );				\
			g_optFailures++;														\
		} else {																	\
			printf( "  ok   %s\n", #cond );											\
		}																			\
	} while ( 0 )

/*
================
Test_TargetFrameMs
================
*/
static void Test_TargetFrameMs() {
	OPT_CHECK( HF_Opt::TargetFrameMs( 60 ) > 16.6f && HF_Opt::TargetFrameMs( 60 ) < 16.7f );
	OPT_CHECK( HF_Opt::TargetFrameMs( 30 ) > 33.3f && HF_Opt::TargetFrameMs( 30 ) < 33.4f );
	OPT_CHECK( HF_Opt::TargetFrameMs( 120 ) > 8.3f && HF_Opt::TargetFrameMs( 120 ) < 8.4f );
	OPT_CHECK( HF_Opt::TargetFrameMs( 144 ) > 6.9f && HF_Opt::TargetFrameMs( 144 ) < 7.0f );
	OPT_CHECK( HF_Opt::TargetFrameMs( 0 ) == 16.6667f );
	OPT_CHECK( HF_Opt::TargetFrameMs( -1 ) == 16.6667f );
}

/*
================
Test_ClampQuality
================
*/
static void Test_ClampQuality() {
	OPT_CHECK( HF_Opt::ClampQuality( 5, 0, 9 ) == 5 );
	OPT_CHECK( HF_Opt::ClampQuality( -1, 0, 9 ) == 0 );
	OPT_CHECK( HF_Opt::ClampQuality( 12, 0, 9 ) == 9 );
	OPT_CHECK( HF_Opt::ClampQuality( 0, 3, 8 ) == 3 );
	OPT_CHECK( HF_Opt::ClampQuality( 8, 3, 8 ) == 8 );
	OPT_CHECK( HF_Opt::ClampQuality( 0, 0, 0 ) == 0 );
}

/*
================
Test_BudgetRatios
================
*/
static void Test_BudgetRatios() {
	// OverBudgetRatio: 1.0 at aggression 0, 1.25 at aggression 1
	float over0 = HF_Opt::OverBudgetRatio( 0.0f );
	float over1 = HF_Opt::OverBudgetRatio( 1.0f );
	OPT_CHECK( over0 > 0.99f && over0 < 1.01f );
	OPT_CHECK( over1 > 1.24f && over1 < 1.26f );

	// HeadroomRatio: 1.0 at aggression 0, 0.7 at aggression 1
	float head0 = HF_Opt::HeadroomRatio( 0.0f );
	float head1 = HF_Opt::HeadroomRatio( 1.0f );
	OPT_CHECK( head0 > 0.99f && head0 < 1.01f );
	OPT_CHECK( head1 > 0.69f && head1 < 0.71f );
}

/*
================
Test_StepDirection
================
*/
static void Test_StepDirection() {
	const float budget = 16.6667f;
	const float agg = 0.5f;

	// Way over budget — should step down
	OPT_CHECK( HF_Opt::StepDirection( 25.0f, budget, agg, 0, 8 ) < 0 );

	// Slightly over budget but still below overBudgetRatio — hold
	OPT_CHECK( HF_Opt::StepDirection( 17.0f, budget, agg, 0, 8 ) == 0 );

	// Way under budget, not enough frames — hold (waiting for stepUpFrames)
	OPT_CHECK( HF_Opt::StepDirection( 5.0f, budget, agg, 2, 8 ) == 0 );

	// Way under budget, enough frames — step up
	OPT_CHECK( HF_Opt::StepDirection( 5.0f, budget, agg, 8, 8 ) > 0 );

	// Right at budget boundary — should hold (not significantly over/under)
	OPT_CHECK( HF_Opt::StepDirection( 16.0f, budget, agg, 0, 8 ) == 0 );

	// Aggression 0: only the bare minimum headroom is needed
	OPT_CHECK( HF_Opt::StepDirection( 16.0f, budget, 0.0f, 8, 8 ) > 0 );

	// Aggression 1: very strict headroom — 16ms is *at* budget, not under
	OPT_CHECK( HF_Opt::StepDirection( 16.0f, budget, 1.0f, 8, 8 ) == 0 );
}

/*
================
Test_QualityLevelForCaps
================
*/
static void Test_QualityLevelForCaps() {
	// GL 4.6 with compute and UBO -> Ultra
	OPT_CHECK( HF_Opt::QualityLevelForCaps( 4, 6, true, true ) >= 7 );

	// GL 4.0 no compute (e.g. some older NVIDIA)
	OPT_CHECK( HF_Opt::QualityLevelForCaps( 4, 0, false, true ) >= 5
			   && HF_Opt::QualityLevelForCaps( 4, 0, false, true ) <= 6 );

	// GL 3.3 with UBO but no compute
	OPT_CHECK( HF_Opt::QualityLevelForCaps( 3, 3, false, true ) >= 4
			   && HF_Opt::QualityLevelForCaps( 3, 3, false, true ) <= 5 );

	// GL 3.0, no UBO
	OPT_CHECK( HF_Opt::QualityLevelForCaps( 3, 0, false, false ) <= 3 );

	// GL 2.1 — always falls to Performance
	OPT_CHECK( HF_Opt::QualityLevelForCaps( 2, 1, false, false ) <= 2 );
}

/*
================
Test_QualityTableIntegrity
================
*/
static void Test_QualityTableIntegrity() {
	// Every row has a non-NULL label and FI_COUNT entries
	for ( int i = 0; i < HF_Opt::QUALITY_LEVELS; i++ ) {
		const HF_Opt::QualityRow & row = HF_Opt::kQualityRows[i];
		OPT_CHECK( row.label != NULL );
		OPT_CHECK( row.label[0] != '\0' );
		for ( int f = 0; f < HF_Opt::FI_COUNT; f++ ) {
			OPT_CHECK( row.cvarNames[f] != NULL );
			OPT_CHECK( row.cvarValues[f] != NULL );
		}
	}

	// QualityLabel returns non-NULL for all valid levels
	for ( int i = 0; i < HF_Opt::QUALITY_LEVELS; i++ ) {
		OPT_CHECK( HF_Opt::QualityLabel( i ) != NULL );
		OPT_CHECK( HF_Opt::QualityLabel( i )[0] != '\0' );
	}
	OPT_CHECK( hfStr::Icmp( HF_Opt::QualityLabel( 0 ), "Ultra Performance" ) == 0 );
	OPT_CHECK( hfStr::Icmp( HF_Opt::QualityLabel( 9 ), "Nightmare" ) == 0 );
	OPT_CHECK( hfStr::Icmp( HF_Opt::QualityLabel( -1 ), "???" ) == 0 );
	OPT_CHECK( hfStr::Icmp( HF_Opt::QualityLabel( 10 ), "???" ) == 0 );

	// r_skipShadows exists in every row (column FI_SHADOWS) and is 0 or 1
	for ( int i = 0; i < HF_Opt::QUALITY_LEVELS; i++ ) {
		OPT_CHECK( hfStr::Icmp( HF_Opt::kQualityRows[i].cvarNames[HF_Opt::FI_SHADOWS], "r_skipShadows" ) == 0 );
		const char * val = HF_Opt::kQualityRows[i].cvarValues[HF_Opt::FI_SHADOWS];
		OPT_CHECK( hfStr::Icmp( val, "0" ) == 0 || hfStr::Icmp( val, "1" ) == 0 );
	}

	// r_maxAnisotropicFiltering is monotonically non-decreasing through the
	// ladder (higher quality -> >= aniso)
	int prevAniso = -1;
	for ( int i = 0; i < HF_Opt::QUALITY_LEVELS; i++ ) {
		int aniso = atoi( HF_Opt::kQualityRows[i].cvarValues[HF_Opt::FI_ANISO] );
		OPT_CHECK( aniso >= prevAniso );
		prevAniso = aniso;
	}
}

/*
================
Test_QualitySnapshot
================
*/
static void Test_QualitySnapshot() {
	char buf[256];
	HF_Opt::QualitySnapshot( buf, sizeof( buf ), 5, 12.3f, 16.67f, 0.82f );
	OPT_CHECK( hfStr::FindText( buf, "LVL 5/" ) != -1 );
	OPT_CHECK( hfStr::FindText( buf, "GPU 12.3ms" ) != -1 );
	OPT_CHECK( hfStr::FindText( buf, "RES 82%" ) != -1 );
}

/*
================
HfTest_OptimizeMain

Run all optimizer-logic unit tests; return the failure count (0 = pass).
================
*/
int HfTest_OptimizeMain() {
	printf( "--- optimizer logic (hfOptimize) ---\n" );

	Test_TargetFrameMs();
	Test_ClampQuality();
	Test_BudgetRatios();
	Test_StepDirection();
	Test_QualityLevelForCaps();
	Test_QualityTableIntegrity();
	Test_QualitySnapshot();

	printf( "%s\n", g_optFailures == 0 ? "  optimizer logic: all ok" : "  optimizer logic: FAILURES PRESENT" );
	return g_optFailures;
}