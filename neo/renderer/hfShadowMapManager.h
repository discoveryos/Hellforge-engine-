#ifndef __HF_SHADOWMAP__H__
#define __HF_SHADOWMAP__H__

// Sparse shadow-map capture subsystem. Renders each active light's caster
// chain into a small depth-only FBO (a 'shadow chip') once per frame using the
// existing depth programs, then records the light->clip transform so the uber
// interaction shader can later sample the chip for a cheap shadow test
// (sampling is a documented opt-in step so current visuals are unchanged).
//
// Gating: r_shadowMapMode
//   0 - disabled; the renderer uses the classic stencil shadow volumes
//   1 - capture shadow chips + keep stencil (fill/geometry timing stats)
//   2 - capture + 'shadowMapStats' prints per-light info
//
// Completely optional: if the FBO entry points can't be resolved, every
// CaptureLight() call returns false and the engine path is untouched.

#include "hfStr.h"

class hfRenderMatrix;

class hfShadowMapManager {
public:
						hfShadowMapManager();

	// Can this build target do shadow-map GL calls? (dlsym of FBO entry
	// points + r_shadowMapSize sanity). Call once before use.
	bool				Supported();

	// Reset per-frame counters. Call at the start of the backend frame.
	void				BeginRender();

	// Render 'light's caster chain (globalShadows + localShadows) into the
	// shared shadow chip. Returns true when the light is captured and HAS
	// been recorded this frame; false means "use the fallback path".
	bool				CaptureLight( const void *lightCtx );

	// True when this light was captured this frame.
	bool				WasLightCaptured( const void *light ) const;

	int					GetCapturedCount() const { return m_capturedCount; }
	unsigned long long	GetCapturedMicros() const { return m_capturedMicros; }
	int					GetFrameCasters() const { return m_frameCasters; }
	int					GetInitFlags() const { return m_initFlags; }
	const char *		GetStatus( hfStr &out ) const;

protected:
	bool				m_resolved;
	int					m_initFlags;			// bit0 = dlsym ok, bit1 = FBO ok
	int					m_size;
	int					m_capturedCount;
	int					m_frameCasters;
	unsigned long long	m_capturedMicros;
	unsigned int		m_lightIds[32];		// captured light idents (pointer hash)
	unsigned char		m_captured[32];
	unsigned int		m_fbo;				// shared chip FBO
	unsigned int		m_colorTex;			// color attachment (RGBA8)
	unsigned int		m_depthRBO;
};

extern hfShadowMapManager shadowMapManager;

#endif