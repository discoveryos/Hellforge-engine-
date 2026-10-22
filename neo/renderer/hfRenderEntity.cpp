#pragma hdrstop
#include "../hflib/hfprecompiled.h"

#include "hftr_local.h"

hfRenderEntityLocal::hfRenderEntityLocal() {
	memset( &parms, 0, sizeof( parms ) );
	memset( modelMatrix, 0, sizeof( modelMatrix ) );

	world					= NULL;
	index					= 0;
	lastModifiedFrameNum	= 0;
	archived				= false;
	dynamicModel			= NULL;
	dynamicModelFrameCount	= 0;
	cachedDynamicModel		= NULL;
	localReferenceBounds	= bounds_zero;
	globalReferenceBounds	= bounds_zero;
	viewCount				= 0;
	viewEntity				= NULL;
	decals					= NULL;
	overlays				= NULL;
	entityRefs				= NULL;
	firstInteraction		= NULL;
	lastInteraction			= NULL;
	needsPortalSky			= false;
}

void hfRenderEntityLocal::FreeRenderEntity() {
}

void hfRenderEntityLocal::UpdateRenderEntity( const renderEntity_t *re, bool forceUpdate ) {
}

void hfRenderEntityLocal::GetRenderEntity( renderEntity_t *re ) {
}

void hfRenderEntityLocal::ForceUpdate() {
}

int hfRenderEntityLocal::GetIndex() {
	return index;
}

void hfRenderEntityLocal::ProjectOverlay( const hfPlane localTextureAxis[2], const hfMaterial *material ) {
}
void hfRenderEntityLocal::RemoveDecals() {
}

//======================================================================

hfRenderLightLocal::hfRenderLightLocal() {
	memset( &parms, 0, sizeof( parms ) );
	memset( lightProject, 0, sizeof( lightProject ) );

	lightHasMoved			= false;
	world					= NULL;
	index					= 0;
	areaNum					= 0;
	lastModifiedFrameNum	= 0;
	archived				= false;
	lightShader				= NULL;
	falloffImage			= NULL;
	globalLightOrigin		= vec3_zero;
	viewCount				= 0;
	viewLight				= NULL;
	references				= NULL;
	foggedPortals			= NULL;
	firstInteraction		= NULL;
	lastInteraction			= NULL;

	baseLightProject.Zero();
	inverseBaseLightProject.Zero();
}

void hfRenderLightLocal::FreeRenderLight() {
}
void hfRenderLightLocal::UpdateRenderLight( const renderLight_t *re, bool forceUpdate ) {
}
void hfRenderLightLocal::GetRenderLight( renderLight_t *re ) {
}
void hfRenderLightLocal::ForceUpdate() {
}
int hfRenderLightLocal::GetIndex() {
	return index;
}

