#pragma hdrstop
#include "../hfprecompiled.h"

hfComplex complex_origin( 0.0f, 0.0f );

/*
=============
hfComplex::ToString
=============
*/
const char *hfComplex::ToString( int precision ) const {
	return hfStr::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

