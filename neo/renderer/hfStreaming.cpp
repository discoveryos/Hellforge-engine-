#pragma hdrstop
#include "../hflib/hfprecompiled.h"

#include "hfStreaming.h"

#include "hfCVarSystem.h"
#include "hftr_local.h"

hfCVar r_streamTextures( "r_streamTextures", "0", CVAR_RENDERER | CVAR_BOOL, "engine image streaming toggle (planner active, mip hook pending)" );
hfCVar r_streamBudgetMB( "r_streamBudgetMB", "512", CVAR_RENDERER | CVAR_INTEGER, "texture resident budget in megabytes" );

hfStreamingManager streamingManager;

hfStreamingManager::hfStreamingManager() {
	m_reported = false;
	m_lastDegraded = 0;
}

bool hfStreamingManager::IsEnabled() const {
	return r_streamTextures.GetBool();
}

unsigned int hfStreamingManager::GetBudgetBytes() const {
	unsigned int mb = r_streamBudgetMB.GetInteger();
	if ( mb < 32 ) { mb = 32; }
	if ( mb > 8192 ) { mb = 8192; }
	return mb << 20;
}

void hfStreamingManager::Frame() {
	if ( !IsEnabled() ) { return; }
	if ( !m_reported ) {
		m_reported = true;
		hfLib::Printf( "hfStreaming: mip-budget planner active (%d MB budget). "
			"hfImage mip-drop hook not wired yet; texture residency unchanged.\n",
			r_streamBudgetMB.GetInteger() );
	}
}

const char * hfStreamingManager::GetStatus( hfStr &out ) const {
	out = va( "stream:%s budget:%dMB", r_streamTextures.GetBool() ? "on" : "off", r_streamBudgetMB.GetInteger() );
	return out.c_str();
}