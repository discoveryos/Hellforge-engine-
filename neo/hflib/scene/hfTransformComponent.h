#ifndef __HF_SCENE_TRANSFORM_COMPONENT_H__
#define __HF_SCENE_TRANSFORM_COMPONENT_H__

// Local transform (position + rotation + scale) plus the world matrix
// computed by the TransformSystem.  Mirrors the role of SynapseEngine's
// TransformComponent; the TRS math is Hellforge-native hfQuat/hfMat4.

namespace HF_Scene {

struct hfTransformComponent {
	hfVec3			position;
	hfQuat			rotation;
	hfVec3			scale;
	bool			dirty;
	hfMat4			worldMatrix;			// written by TransformSystem

	HF_INLINE hfTransformComponent() {
		position = hfVec3( 0.0f, 0.0f, 0.0f );
		rotation = hfQuat( 0.0f, 0.0f, 0.0f, 1.0f );
		scale = hfVec3( 1.0f, 1.0f, 1.0f );
		dirty = true;
		worldMatrix.Identity();
	}

	// Rebuild the local TRS matrix manually (used by the transform system;
	// fine to call directly when a component is edited outside a frame).
	HF_INLINE hfMat4 LocalMatrix() const {
		hfVec3 right = rotation * hfVec3( scale.x, 0.0f, 0.0f );
		hfVec3 up = rotation * hfVec3( 0.0f, scale.y, 0.0f );
		hfVec3 fwd = rotation * hfVec3( 0.0f, 0.0f, scale.z );
		return hfMat4(
			right.x, up.x, fwd.x, position.x,
			right.y, up.y, fwd.y, position.y,
			right.z, up.z, fwd.z, position.z,
			0.0f, 0.0f, 0.0f, 1.0f );
	}

	HF_INLINE void SetPosition( const hfVec3 &p ) { position = p; dirty = true; }
	HF_INLINE void SetRotation( const hfQuat &q ) { rotation = q; rotation.Normalize(); dirty = true; }
	HF_INLINE void SetScale( const hfVec3 &s ) { scale = s; dirty = true; }

	// Local->world direction: rotate+scale (no translation).
	HF_INLINE hfVec3 TransformDirection( const hfVec3 &local ) const {
		return rotation * ( hfVec3( local.x * scale.x, local.y * scale.y, local.z * scale.z ) );
	}

	// Local->world point.
	HF_INLINE hfVec3 TransformPoint( const hfVec3 &local ) const {
		return worldMatrix * local;
	}

	// Derived world axes (normalized, unscaled framing).
	HF_INLINE hfVec3 WorldRight() const { return rotation * hfVec3( 1.0f, 0.0f, 0.0f ); }
	HF_INLINE hfVec3 WorldUp() const { return rotation * hfVec3( 0.0f, 1.0f, 0.0f ); }
	HF_INLINE hfVec3 WorldForward() const { return rotation * hfVec3( 0.0f, 0.0f, 1.0f ); }
	HF_INLINE hfVec3 WorldPosition() const { return worldMatrix * hfVec3( 0.0f, 0.0f, 0.0f ); }
};

} // namespace HF_Scene

#endif