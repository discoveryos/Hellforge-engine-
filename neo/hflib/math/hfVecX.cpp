#pragma hdrstop
#include "../hfprecompiled.h"

//===============================================================
//
//	hfVecX
//
//===============================================================

float	hfVecX::temp[VECX_MAX_TEMP+4];
float *	hfVecX::tempPtr = (float *) ( ( (uintptr_t) hfVecX::temp + 15 ) & ~15 );
int		hfVecX::tempIndex = 0;

/*
=============
hfVecX::ToString
=============
*/
const char *hfVecX::ToString( int precision ) const {
	return hfStr::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

