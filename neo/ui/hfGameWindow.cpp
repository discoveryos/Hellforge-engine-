#include "../hflib/hfprecompiled.h"
#pragma hdrstop

#include "hfDeviceContext.h"
#include "hfWindow.h"
#include "hfUserInterfaceLocal.h"
#include "hfGameWindow.h"

/*
================
hfGameWindowProxy::hfGameWindowProxy
================
*/
hfGameWindowProxy::hfGameWindowProxy( hfDeviceContext *d, hfUserInterfaceLocal *g ) : hfWindow( g ) { }

/*
================
hfGameWindowProxy::Draw
================
*/
void hfGameWindowProxy::Draw( int time, float x, float y ) {
	common->Printf( "TODO: hfGameWindowProxy::Draw\n" );
}


