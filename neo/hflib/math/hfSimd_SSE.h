#ifndef __MATH_SIMD_SSE_H__
#define __MATH_SIMD_SSE_H__

/*
===============================================================================

	SSE implementation of hfSIMDProcessor

===============================================================================
*/

class hfSIMD_SSE : public hfSIMD_Generic {
public:
	virtual const char * VPCALL GetName() const;

	virtual void VPCALL BlendJoints( hfJointQuat *joints, const hfJointQuat *blendJoints, const float lerp, const int *index, const int numJoints );
	virtual void VPCALL BlendJointsFast( hfJointQuat *joints, const hfJointQuat *blendJoints, const float lerp, const int *index, const int numJoints );
	virtual void VPCALL ConvertJointQuatsToJointMats( hfJointMat *jointMats, const hfJointQuat *jointQuats, const int numJoints );
	virtual void VPCALL ConvertJointMatsToJointQuats( hfJointQuat *jointQuats, const hfJointMat *jointMats, const int numJoints );
	virtual void VPCALL TransformJoints( hfJointMat *jointMats, const int *parents, const int firstJoint, const int lastJoint );
	virtual void VPCALL UntransformJoints( hfJointMat *jointMats, const int *parents, const int firstJoint, const int lastJoint );
};

#endif /* !__MATH_SIMD_SSE_H__ */

