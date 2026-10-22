#ifndef __HF_SCENE_CAMERA_COMPONENT_H__
#define __HF_SCENE_CAMERA_COMPONENT_H__

// Camera parameters + the view/projection matrices the CameraSystem
// produces each frame from the owning entity's world transform.

namespace HF_Scene {

struct hfCameraComponent {
	float			fovY;			// vertical field of view, degrees
	float			aspect;			// width / height
	float			nearZ;
	float			farZ;
	hfVec4			clearColor;
	hfMat4			viewMatrix;			// written by CameraSystem
	hfMat4			projectionMatrix;	// written by CameraSystem

	HF_INLINE hfCameraComponent() {
		fovY = 75.0f;
		aspect = 16.0f / 9.0f;
		nearZ = 0.1f;
		farZ = 1000.0f;
		clearColor = hfVec4( 0.05f, 0.05f, 0.08f, 1.0f );
		viewMatrix.Identity();
		projectionMatrix.Identity();
	}

	// Row-major OpenGL-style perspective (ndc depth in [-1,1], camera
	// looking down its -Z axis — consistent with the light-cluster maths).
	HF_INLINE void ComputeProjection() {
		float f = 1.0f / hfMath::Tan( fovY * 0.5f * hfMath::PI / 180.0f );
		float n = nearZ, fz = farZ;
		float m22 = ( fz + n ) / ( n - fz );
		float m23 = ( 2.0f * fz * n ) / ( n - fz );
		projectionMatrix = hfMat4(
			f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, f, 0.0f, 0.0f,
			0.0f, 0.0f, m22, m23,
			0.0f, 0.0f, -1.0f, 0.0f );
	}

	// World position of this camera (from the parent transform, if any).
	HF_INLINE hfVec3 GetPosition( const hfMat4 &camWorld ) const {
		return camWorld * hfVec3( 0.0f, 0.0f, 0.0f );
	}

	// Forward view direction (camera looks down its -Z).
	HF_INLINE hfVec3 GetForward( const hfMat4 &camWorld ) const {
		hfVec3 fwd = camWorld * hfVec3( 0.0f, 0.0f, -1.0f ) - ( camWorld * hfVec3( 0.0f, 0.0f, 0.0f ) );
		fwd.Normalize();
		return fwd;
	}
};

} // namespace HF_Scene

#endif