#pragma hdrstop
#include "../hfprecompiled.h"
#include "hfSimd_Generic.h"

//===============================================================
//
//	Generic implementation of hfSIMDProcessor
//
//===============================================================

#define UNROLL1(Y) { int _IX; for (_IX=0;_IX<count;_IX++) {Y(_IX);} }
#define UNROLL2(Y) { int _IX, _NM = count&0xfffffffe; for (_IX=0;_IX<_NM;_IX+=2){Y(_IX+0);Y(_IX+1);} if (_IX < count) {Y(_IX);}}
#define UNROLL4(Y) { int _IX, _NM = count&0xfffffffc; for (_IX=0;_IX<_NM;_IX+=4){Y(_IX+0);Y(_IX+1);Y(_IX+2);Y(_IX+3);}for(;_IX<count;_IX++){Y(_IX);}}
#define UNROLL8(Y) { int _IX, _NM = count&0xfffffff8; for (_IX=0;_IX<_NM;_IX+=8){Y(_IX+0);Y(_IX+1);Y(_IX+2);Y(_IX+3);Y(_IX+4);Y(_IX+5);Y(_IX+6);Y(_IX+7);} _NM = count&0xfffffffe; for(;_IX<_NM;_IX+=2){Y(_IX); Y(_IX+1);} if (_IX < count) {Y(_IX);} }

#ifdef _DEBUG
#define NODEFAULT	default: assert( 0 )
#else
#define NODEFAULT	default: __assume( 0 )
#endif


/*
============
hfSIMD_Generic::GetName
============
*/
const char * hfSIMD_Generic::GetName() const {
	return "generic code";
}

/*
============
hfSIMD_Generic::MinMax
============
*/
void VPCALL hfSIMD_Generic::MinMax( float &min, float &max, const float *src, const int count ) {
	min = hfMath::INFINITY; max = -hfMath::INFINITY;
#define OPER(X) if ( src[(X)] < min ) {min = src[(X)];} if ( src[(X)] > max ) {max = src[(X)];}
	UNROLL1(OPER)
#undef OPER
}

/*
============
hfSIMD_Generic::MinMax
============
*/
void VPCALL hfSIMD_Generic::MinMax( hfVec2 &min, hfVec2 &max, const hfVec2 *src, const int count ) {
	min[0] = min[1] = hfMath::INFINITY; max[0] = max[1] = -hfMath::INFINITY;
#define OPER(X) const hfVec2 &v = src[(X)]; if ( v[0] < min[0] ) { min[0] = v[0]; } if ( v[0] > max[0] ) { max[0] = v[0]; } if ( v[1] < min[1] ) { min[1] = v[1]; } if ( v[1] > max[1] ) { max[1] = v[1]; }
	UNROLL1(OPER)
#undef OPER
}

/*
============
hfSIMD_Generic::MinMax
============
*/
void VPCALL hfSIMD_Generic::MinMax( hfVec3 &min, hfVec3 &max, const hfVec3 *src, const int count ) {
	min[0] = min[1] = min[2] = hfMath::INFINITY; max[0] = max[1] = max[2] = -hfMath::INFINITY;
#define OPER(X) const hfVec3 &v = src[(X)]; if ( v[0] < min[0] ) { min[0] = v[0]; } if ( v[0] > max[0] ) { max[0] = v[0]; } if ( v[1] < min[1] ) { min[1] = v[1]; } if ( v[1] > max[1] ) { max[1] = v[1]; } if ( v[2] < min[2] ) { min[2] = v[2]; } if ( v[2] > max[2] ) { max[2] = v[2]; }
	UNROLL1(OPER)
#undef OPER
}

/*
============
hfSIMD_Generic::MinMax
============
*/
void VPCALL hfSIMD_Generic::MinMax( hfVec3 &min, hfVec3 &max, const hfDrawVert *src, const int count ) {
	min[0] = min[1] = min[2] = hfMath::INFINITY; max[0] = max[1] = max[2] = -hfMath::INFINITY;
#define OPER(X) const hfVec3 &v = src[(X)].xyz; if ( v[0] < min[0] ) { min[0] = v[0]; } if ( v[0] > max[0] ) { max[0] = v[0]; } if ( v[1] < min[1] ) { min[1] = v[1]; } if ( v[1] > max[1] ) { max[1] = v[1]; } if ( v[2] < min[2] ) { min[2] = v[2]; } if ( v[2] > max[2] ) { max[2] = v[2]; }
	UNROLL1(OPER)
#undef OPER
}

/*
============
hfSIMD_Generic::MinMax
============
*/
void VPCALL hfSIMD_Generic::MinMax( hfVec3 &min, hfVec3 &max, const hfDrawVert *src, const triIndex_t *indexes, const int count ) {
	min[0] = min[1] = min[2] = hfMath::INFINITY; max[0] = max[1] = max[2] = -hfMath::INFINITY;
#define OPER(X) const hfVec3 &v = src[indexes[(X)]].xyz; if ( v[0] < min[0] ) { min[0] = v[0]; } if ( v[0] > max[0] ) { max[0] = v[0]; } if ( v[1] < min[1] ) { min[1] = v[1]; } if ( v[1] > max[1] ) { max[1] = v[1]; } if ( v[2] < min[2] ) { min[2] = v[2]; } if ( v[2] > max[2] ) { max[2] = v[2]; }
	UNROLL1(OPER)
#undef OPER
}

/*
================
hfSIMD_Generic::Memcpy
================
*/
void VPCALL hfSIMD_Generic::Memcpy( void *dst, const void *src, const int count ) {
	memcpy( dst, src, count );
}

/*
================
hfSIMD_Generic::Memset
================
*/
void VPCALL hfSIMD_Generic::Memset( void *dst, const int val, const int count ) {
	memset( dst, val, count );
}

/*
============
hfSIMD_Generic::BlendJoints
============
*/
void VPCALL hfSIMD_Generic::BlendJoints( hfJointQuat *joints, const hfJointQuat *blendJoints, const float lerp, const int *index, const int numJoints ) {
	for ( int i = 0; i < numJoints; i++ ) {
		int j = index[i];
		joints[j].q.Slerp( joints[j].q, blendJoints[j].q, lerp );
		joints[j].t.Lerp( joints[j].t, blendJoints[j].t, lerp );
		joints[j].w = 0.0f;
	}
}

/*
============
hfSIMD_Generic::BlendJointsFast
============
*/
void VPCALL hfSIMD_Generic::BlendJointsFast( hfJointQuat *joints, const hfJointQuat *blendJoints, const float lerp, const int *index, const int numJoints ) {
	for ( int i = 0; i < numJoints; i++ ) {
		int j = index[i];
		joints[j].q.Lerp( joints[j].q, blendJoints[j].q, lerp );
		joints[j].t.Lerp( joints[j].t, blendJoints[j].t, lerp );
		joints[j].w = 0.0f;
	}
}

/*
============
hfSIMD_Generic::ConvertJointQuatsToJointMats
============
*/
void VPCALL hfSIMD_Generic::ConvertJointQuatsToJointMats( hfJointMat *jointMats, const hfJointQuat *jointQuats, const int numJoints ) {
	for ( int i = 0; i < numJoints; i++ ) {
		jointMats[i].SetRotation( jointQuats[i].q.ToMat3() );
		jointMats[i].SetTranslation( jointQuats[i].t );
	}
}

/*
============
hfSIMD_Generic::ConvertJointMatsToJointQuats
============
*/
void VPCALL hfSIMD_Generic::ConvertJointMatsToJointQuats( hfJointQuat *jointQuats, const hfJointMat *jointMats, const int numJoints ) {
	for ( int i = 0; i < numJoints; i++ ) {
		jointQuats[i] = jointMats[i].ToJointQuat();
	}
}

/*
============
hfSIMD_Generic::TransformJoints
============
*/
void VPCALL hfSIMD_Generic::TransformJoints( hfJointMat *jointMats, const int *parents, const int firstJoint, const int lastJoint ) {
	for ( int i = firstJoint; i <= lastJoint; i++ ) {
		assert( parents[i] < i );
		jointMats[i] *= jointMats[parents[i]];
	}
}

/*
============
hfSIMD_Generic::UntransformJoints
============
*/
void VPCALL hfSIMD_Generic::UntransformJoints( hfJointMat *jointMats, const int *parents, const int firstJoint, const int lastJoint ) {
	for ( int i = lastJoint; i >= firstJoint; i-- ) {
		assert( parents[i] < i );
		jointMats[i] /= jointMats[parents[i]];
	}
}

