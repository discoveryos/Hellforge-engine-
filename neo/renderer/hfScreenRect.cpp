#pragma hdrstop
#include "../hflib/hfprecompiled.h"

#include "hftr_local.h"

/*
==========================================================================================

hfScreenRect

==========================================================================================
*/

/*
======================
hfScreenRect::Clear
======================
*/
void hfScreenRect::Clear() {
	x1 = y1 = 32000;
	x2 = y2 = -32000;
	zmin = 0.0f;
	zmax = 1.0f;
}

/*
======================
hfScreenRect::AddPoint
======================
*/
void hfScreenRect::AddPoint( float x, float y ) {
	int	ix = hfMath::Ftoi( x );
	int iy = hfMath::Ftoi( y );

	if ( ix < x1 ) {
		x1 = ix;
	}
	if ( ix > x2 ) {
		x2 = ix;
	}
	if ( iy < y1 ) {
		y1 = iy;
	}
	if ( iy > y2 ) {
		y2 = iy;
	}
}

/*
======================
hfScreenRect::Expand
======================
*/
void hfScreenRect::Expand() {
	x1--;
	y1--;
	x2++;
	y2++;
}

/*
======================
hfScreenRect::Intersect
======================
*/
void hfScreenRect::Intersect( const hfScreenRect &rect ) {
	if ( rect.x1 > x1 ) {
		x1 = rect.x1;
	}
	if ( rect.x2 < x2 ) {
		x2 = rect.x2;
	}
	if ( rect.y1 > y1 ) {
		y1 = rect.y1;
	}
	if ( rect.y2 < y2 ) {
		y2 = rect.y2;
	}
}

/*
======================
hfScreenRect::Union
======================
*/
void hfScreenRect::Union( const hfScreenRect &rect ) {
	if ( rect.x1 < x1 ) {
		x1 = rect.x1;
	}
	if ( rect.x2 > x2 ) {
		x2 = rect.x2;
	}
	if ( rect.y1 < y1 ) {
		y1 = rect.y1;
	}
	if ( rect.y2 > y2 ) {
		y2 = rect.y2;
	}
}

/*
======================
hfScreenRect::Equals
======================
*/
bool hfScreenRect::Equals( const hfScreenRect &rect ) const {
	return ( x1 == rect.x1 && x2 == rect.x2 && y1 == rect.y1 && y2 == rect.y2 );
}

/*
======================
hfScreenRect::IsEmpty
======================
*/
bool hfScreenRect::IsEmpty() const {
	return ( x1 > x2 || y1 > y2 );
}

/*
======================
R_ShowColoredScreenRect
======================
*/
void R_ShowColoredScreenRect( const hfScreenRect &rect, int colorIndex ) {
	if ( !rect.IsEmpty() ) {
		static hfVec4 colors[] = { colorRed, colorGreen, colorBlue, colorYellow, colorMagenta, colorCyan, colorWhite, colorPurple };
		tr.viewDef->renderWorld->DebugScreenRect( colors[colorIndex & 7], rect, tr.viewDef );
	}
}

