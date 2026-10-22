#ifndef __GLMATRIX_H__
#define __GLMATRIX_H__

/*
==========================================================================================

This deals with column-major (OpenGL style) matrices where transforms are
applied with right-multiplication.

This is the old matrix code that should really to be replaced with hfRenderMatrix.

==========================================================================================
*/

void R_AxisToModelMatrix( const hfMat3 &axis, const hfVec3 &origin, float modelMatrix[16] );
void R_MatrixTranspose( const float in[16], float out[16] );
void R_MatrixMultiply( const float *a, const float *b, float *out );

void R_TransformModelToClip( const hfVec3 &src, const float *modelMatrix, const float *projectionMatrix, hfPlane &eye, hfPlane &dst );
void R_TransformClipToDevice( const hfPlane &clip, hfVec3 &ndc );
void R_GlobalToNormalizedDeviceCoordinates( const hfVec3 &global, hfVec3 &ndc );

// note that these assume a normalized matrix, and will not work with scaled axis
void R_GlobalPointToLocal( const float modelMatrix[16], const hfVec3 &in, hfVec3 &out );
void R_LocalPointToGlobal( const float modelMatrix[16], const hfVec3 &in, hfVec3 &out );

void R_GlobalVectorToLocal( const float modelMatrix[16], const hfVec3 &in, hfVec3 &out );
void R_LocalVectorToGlobal( const float modelMatrix[16], const hfVec3 &in, hfVec3 &out );

void R_GlobalPlaneToLocal( const float modelMatrix[16], const hfPlane &in, hfPlane &out );
void R_LocalPlaneToGlobal( const float modelMatrix[16], const hfPlane &in, hfPlane &out );

void R_SetupViewMatrix( viewDef_t *viewDef );
void R_SetupProjectionMatrix( viewDef_t *viewDef );

#endif /* !__GLMATRIX_H__ */

