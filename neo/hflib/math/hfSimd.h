#ifndef __MATH_SIMD_H__
#define __MATH_SIMD_H__

/*
===============================================================================

	Single Instruction Multiple Data (SIMD)

	For optimal use data should be aligned on a 16 byte boundary.
	All hfSIMDProcessor routines are thread safe.

===============================================================================
*/

class hfSIMD {
public:
	static void			Init();
	static void			InitProcessor( const char *module, bool forceGeneric );
	static void			Shutdown();
	static void			Test_f( const class hfCmdArgs &args );
};


/*
===============================================================================

	virtual base class for different SIMD processors

===============================================================================
*/

#if defined(_WIN32)
#define VPCALL __fastcall
#else
#define VPCALL
#endif

class hfVec2;
class hfVec3;
class hfVec4;
class hfVec5;
class hfVec6;
class hfVecX;
class hfMat2;
class hfMat3;
class hfMat4;
class hfMat5;
class hfMat6;
class hfMatX;
class hfPlane;
class hfDrawVert;
class hfJointQuat;
class hfJointMat;
struct dominantTri_t;

class hfSIMDProcessor {
public:
									hfSIMDProcessor() { cpuid = CPUID_NONE; }

	cpuid_t							cpuid;

	virtual const char * VPCALL		GetName() const = 0;

	virtual	void VPCALL MinMax( float &min,			float &max,				const float *src,		const int count ) = 0;
	virtual	void VPCALL MinMax( hfVec2 &min,		hfVec2 &max,			const hfVec2 *src,		const int count ) = 0;
	virtual	void VPCALL MinMax( hfVec3 &min,		hfVec3 &max,			const hfVec3 *src,		const int count ) = 0;
	virtual	void VPCALL MinMax( hfVec3 &min,		hfVec3 &max,			const hfDrawVert *src,	const int count ) = 0;
	virtual	void VPCALL MinMax( hfVec3 &min,		hfVec3 &max,			const hfDrawVert *src,	const triIndex_t *indexes,		const int count ) = 0;

	virtual void VPCALL Memcpy( void *dst,			const void *src,		const int count ) = 0;
	virtual void VPCALL Memset( void *dst,			const int val,			const int count ) = 0;

	// animation
	virtual void VPCALL BlendJoints( hfJointQuat *joints, const hfJointQuat *blendJoints, const float lerp, const int *index, const int numJoints ) = 0;
	virtual void VPCALL BlendJointsFast( hfJointQuat *joints, const hfJointQuat *blendJoints, const float lerp, const int *index, const int numJoints ) = 0;
	virtual void VPCALL ConvertJointQuatsToJointMats( hfJointMat *jointMats, const hfJointQuat *jointQuats, const int numJoints ) = 0;
	virtual void VPCALL ConvertJointMatsToJointQuats( hfJointQuat *jointQuats, const hfJointMat *jointMats, const int numJoints ) = 0;
	virtual void VPCALL TransformJoints( hfJointMat *jointMats, const int *parents, const int firstJoint, const int lastJoint ) = 0;
	virtual void VPCALL UntransformJoints( hfJointMat *jointMats, const int *parents, const int firstJoint, const int lastJoint ) = 0;
};

// pointer to SIMD processor
extern hfSIMDProcessor *SIMDProcessor;

#endif /* !__MATH_SIMD_H__ */

