#pragma once
/*
 hfFpsOptimizer — Adaptive FPS quality management for broad GPU support.

 Monitors GPU frame time and adjusts the quality ladder and dynamic resolution
 in real time so the engine sustains a user-specified FPS target on both old
 (GL 2.1) and new (GL 4.6) hardware. Quality changes take effect instantly
 by writing the associated renderer cvars through cvarSystem->Find().

 The optimizer feeds into the existing hfResolutionScale (rs_*) for the
 resolution axis, and directly writes the feature cvars (r_skip*,
 r_multiSamples, etc.) for the quality axis.

 Design rationale:
  - Pure logic in hfOptimize.h so both the renderer and the CPU-only test
    harness exercise the exact same code paths.
  - The optimizer is polled once per frame from the backend execution
    (RB_ExecuteBackEndCommands) right where GPU timing is already read back.
  - On headless / timing-unavailable hosts, GPU time reports 0 and the
    optimizer stays at its initial level — no overreaction.
*/
#include "../hflib/hfprecompiled.h"

class hfResolutionScale; // forward

class hfFpsOptimizer {
public:
	hfFpsOptimizer();

	// Called once after R_InitOpenGL to read GL caps and apply the initial
	// capability-based quality level.
	void ApplyCapabilityDefaults();

	// Called once during render initialization to compile all uber material
	// native shaders and the compute post shader upfront, avoiding per-frame
	 // hitches.
	void Warmup();

	// Called every frame from RB_ExecuteBackEndCommands with the latest GPU
	// timing (microseconds, 0 = unavailable). Drives quality stepping and
	// writes the appropriate cvars.
	void FrameSample( int gpuMicroSec );

	// Console / testUber-style one-line status.
	void GetStatusText( hfStr &s ) const;

	// Direct access to current state for tests and diagnostics
	int  GetCurrentLevel() const { return currentQualityLevel; }
	float GetLastGpuMs() const { return lastGpuMs; }

private:
	void ApplyQualityLevel( int level );
	void AdjustDynamicResolution( int gpuMicroSec );
	void PrintFrameStats() const;

	int   currentQualityLevel;
	int   minQualityLevel;
	int   maxQualityLevel;
	float lastGpuMs;
	int   framesBelowBudget;
	int   lastAppliedLevel;
};

extern hfFpsOptimizer fpsOptimizer;