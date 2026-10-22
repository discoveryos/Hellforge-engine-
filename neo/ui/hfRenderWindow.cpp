#pragma hdrstop
#include "../hflib/hfprecompiled.h"

#include "hfDeviceContext.h"
#include "hfWindow.h"
#include "hfUserInterfaceLocal.h"
#include "hfRenderWindow.h"

// NO LONGER SUPPORTED!
//
// D3 could render a 3D model in a subrect of a full screen
// GUI for the main menus, but we have cut that ability so
// we don't need to deal with offset viewports on all platforms.

hfRenderWindow::hfRenderWindow(hfUserInterfaceLocal *g) : hfWindow(g) {
	gui = g;
	CommonInit();
}

hfRenderWindow::~hfRenderWindow() {
	renderSystem->FreeRenderWorld( world ); 
}

void hfRenderWindow::CommonInit() {
	world = renderSystem->AllocRenderWorld();
	needsRender = true;
	lightOrigin = hfVec4(-128.0f, 0.0f, 0.0f, 1.0f);
	lightColor = hfVec4(1.0f, 1.0f, 1.0f, 1.0f);
	modelOrigin.Zero();
	viewOffset = hfVec4(-128.0f, 0.0f, 0.0f, 1.0f);
	modelAnim = NULL;
	animLength = 0;
	animEndTime = -1;
	modelDef = -1;
	updateAnimation = true;
}


void hfRenderWindow::BuildAnimation(int time) {
	
	if (!updateAnimation) {
		return;
	}

	if (animName.Length() && animClass.Length()) {
		worldEntity.numJoints = worldEntity.hModel->NumJoints();
		worldEntity.joints = ( hfJointMat * )Mem_Alloc16( SIMD_ROUND_JOINTS( worldEntity.numJoints ) * sizeof( *worldEntity.joints ), TAG_JOINTMAT );
		updateAnimation = false;
	}

}

void hfRenderWindow::PreRender() {
	if (needsRender) {
		world->InitFromMap( NULL );
		memset( &rLight, 0, sizeof( rLight ) );
		memset( &worldEntity, 0, sizeof( worldEntity ) );
		rLight.lightRadius = lightOrigin.ToVec3();
		rLight.origin = lightOrigin.ToVec3();
		rLight.shaderParms[SHADERPARM_RED] = lightColor.x();
		rLight.shaderParms[SHADERPARM_GREEN] = lightColor.y();
		rLight.shaderParms[SHADERPARM_BLUE] = lightColor.z();
		lightDef = world->AddLightDef( &rLight );
		if ( !modelName[0] ) {
			common->Warning( "Window '%s' in gui '%s': no model set", GetName(), GetGui()->GetSourceFile() );
		}
		if ( worldEntity.hModel ) {
			hfVec3 v = modelRotate.ToVec3();
			worldEntity.axis = v.ToMat3();
			worldEntity.shaderParms[0] = 1;
			worldEntity.shaderParms[1] = 1;
			worldEntity.shaderParms[2] = 1;
			worldEntity.shaderParms[3] = 1;
			modelDef = world->AddEntityDef( &worldEntity );
		}
		needsRender = false;
	}
}

void hfRenderWindow::Render( int time ) {
	rLight.origin = lightOrigin.ToVec3();
	rLight.shaderParms[SHADERPARM_RED] = lightColor.x();
	rLight.shaderParms[SHADERPARM_GREEN] = lightColor.y();
	rLight.shaderParms[SHADERPARM_BLUE] = lightColor.z();
	world->UpdateLightDef(lightDef, &rLight);
	if ( worldEntity.hModel ) {
		if (updateAnimation) {
			BuildAnimation(time);
		}
		worldEntity.axis = hfAngles(modelRotate.x(), modelRotate.y(), modelRotate.z()).ToMat3();
		world->UpdateEntityDef(modelDef, &worldEntity);
	}
}




void hfRenderWindow::Draw(int time, float x, float y) {
	PreRender();
	Render(time);

	memset( &refdef, 0, sizeof( refdef ) );
	refdef.vieworg = viewOffset.ToVec3();;
	//refdef.vieworg.Set(-128, 0, 0);

	refdef.viewaxis.Identity();
	refdef.shaderParms[0] = 1;
	refdef.shaderParms[1] = 1;
	refdef.shaderParms[2] = 1;
	refdef.shaderParms[3] = 1;

	refdef.fov_x = 90;
	refdef.fov_y = 2 * atan((float)drawRect.h / drawRect.w) * hfMath::M_RAD2DEG;

	refdef.time[0] = time;
	refdef.time[1] = time;
	world->RenderScene(&refdef);
}

void hfRenderWindow::PostParse() {
	hfWindow::PostParse();
}

// 
//  
hfWinVar *hfRenderWindow::GetWinVarByName(const char *_name, bool fixup, drawWin_t** owner ) {
// 
	if (hfStr::Icmp(_name, "model") == 0) {
		return &modelName;
	}
	if (hfStr::Icmp(_name, "anim") == 0) {
		return &animName;
	}
	if (hfStr::Icmp(_name, "lightOrigin") == 0) {
		return &lightOrigin;
	}
	if (hfStr::Icmp(_name, "lightColor") == 0) {
		return &lightColor;
	}
	if (hfStr::Icmp(_name, "modelOrigin") == 0) {
		return &modelOrigin;
	}
	if (hfStr::Icmp(_name, "modelRotate") == 0) {
		return &modelRotate;
	}
	if (hfStr::Icmp(_name, "viewOffset") == 0) {
		return &viewOffset;
	}
	if (hfStr::Icmp(_name, "needsRender") == 0) {
		return &needsRender;
	}

// 
//  
	return hfWindow::GetWinVarByName(_name, fixup, owner);
// 
}

bool hfRenderWindow::ParseInternalVar(const char *_name, hfTokenParser *src) {
	if (hfStr::Icmp(_name, "animClass") == 0) {
		ParseString(src, animClass);
		return true;
	}
	return hfWindow::ParseInternalVar(_name, src);
}

