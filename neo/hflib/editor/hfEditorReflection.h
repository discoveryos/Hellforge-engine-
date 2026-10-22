#ifndef __HF_EDITOR_REFLECTION_H__
#define __HF_EDITOR_REFLECTION_H__

/*
===============================================================================

	Phase 11 — property reflection.

	The inspector has to answer two questions about the selected entity:
	which components does it have, and what editable fields does each one
	offer. C++ hands us neither, so the editor carries its own small class
	table:

	  hfEditorProperty       one field: name, type, byte offset inside the
	                         component, and (for enums) the name table.

	  hfEditorComponentClass one component type: its display name, its
	                         HF_Ecs type key, three typed thunks (get / add /
	                         has) so the editor never needs the template, and
	                         its property table.

	  hfEditorReflection     the registry, plus ReadValue / WriteValue that
	                         turn (component, property, value) into a load or
	                         a store through the offset.

The thunks are what keep this header-only and free of RTTI: `get` returns a
`void *` obtained from `Registry::GetComponent< T >`, and every property
access is then `base + offset`. The type key is
`HF_Ecs::ComponentStore< T >::TypeKeyAddr()`, the same address the registry
itself uses, so lookup never depends on a string.

Nothing here is generated and nothing is automatic: adding a component to the
editor means adding one entry to `hfEditorRegisterBuiltins()`. That is a
feature — the inspector only ever offers fields the rest of the engine
actually owns.

===============================================================================
*/

#include "../hfprecompiled.h"
#include "../hfEcs.h"
#include "../scene/hfWorld.h"
#include "../scene/hfTransformComponent.h"
#include "../scene/hfTagComponent.h"
#include "../scene/hfCameraComponent.h"
#include "../scene/hfLightComponent.h"
#include "../scene/hfMeshRendererComponent.h"

#ifndef offsetof
#include <stddef.h>
#endif

namespace HF_Editor {

enum hfEditorValueType {
	HF_EDIT_FLOAT = 0,
	HF_EDIT_INT,
	HF_EDIT_BOOL,
	HF_EDIT_STRING,		// hfStr living at the offset
	HF_EDIT_VEC3,		// three contiguous floats
	HF_EDIT_QUAT,		// four contiguous floats
	HF_EDIT_ENUM		// int at the offset, named by enumNames[ value ]
};

static const int HF_EDITOR_MAX_SHEET = 64;

struct hfEditorProperty {
	const char *			name;
	hfEditorValueType		type;
	int						offset;
	int						count;			// 1 scalar, 3 (vec3), 4 (quat)
	const char *			group;
	const char *const *		enumNames;
	int						enumCount;
};

// one field's value, variant-shaped so the caller can hold any property
struct hfEditorValue {
	hfEditorValueType		type;
	float					f[ 4 ];
	int						i;
	bool					b;
	hfStr					s;

	HF_INLINE hfEditorValue() : type( HF_EDIT_FLOAT ), i( 0 ), b( false ) {
		f[ 0 ] = f[ 1 ] = f[ 2 ] = f[ 3 ] = 0.0f;
	}
	HF_INLINE void SetFloat( float v ) { type = HF_EDIT_FLOAT; f[0] = v; f[1] = f[2] = f[3] = 0.0f; }
	HF_INLINE void SetInt( int v ) { type = HF_EDIT_INT; i = v; }
	HF_INLINE void SetBool( bool v ) { type = HF_EDIT_BOOL; b = v; }
	HF_INLINE void SetString( const char *v ) { type = HF_EDIT_STRING; s = v; }
	HF_INLINE void SetVec3( float x, float y, float z ) {
		type = HF_EDIT_VEC3; f[0] = x; f[1] = y; f[2] = z; f[3] = 0.0f;
	}
	HF_INLINE void SetQuat( float x, float y, float z, float w ) {
		type = HF_EDIT_QUAT; f[0] = x; f[1] = y; f[2] = z; f[3] = w;
	}
};

typedef void *	( *hfEditorGetFn )( HF_Ecs::Registry &, HF_Ecs::Entity );
typedef void	( *hfEditorAddFn )( HF_Ecs::Registry &, HF_Ecs::Entity );
typedef bool	( *hfEditorHasFn )( HF_Ecs::Registry &, HF_Ecs::Entity );

struct hfEditorComponentClass {
	const char *			name;
	const void *			typeKey;
	hfEditorGetFn			get;
	hfEditorAddFn			add;
	hfEditorHasFn			has;
	const hfEditorProperty *props;
	int						propCount;
	int						size;			// sizeof( component ), for bounds checks
};

struct hfEditorReflectionState {
	hfEditorComponentClass	classes[ 32 ];
	int						count;
};

// one function-local static: zero-initialised once, never re-zeroed
HF_INLINE hfEditorReflectionState &hfEditor_ReflectionState() {
	static hfEditorReflectionState state;
	return state;
}

class hfEditorReflection {
public:
	static bool	Register( const hfEditorComponentClass &cls ) {
		hfEditorReflectionState &st = hfEditor_ReflectionState();
		if ( !cls.name || !cls.props || cls.propCount <= 0 ) { return false; }
		for ( int i = 0; i < st.count; i++ ) {
			if ( !strcmp( st.classes[i].name, cls.name ) ) {
				st.classes[i] = cls;				// re-register = replace
				return true;
			}
		}
		if ( st.count >= 32 ) { return false; }
		st.classes[ st.count++ ] = cls;
		return true;
	}

	static int		GetClassCount() { return hfEditor_ReflectionState().count; }

	static const hfEditorComponentClass *GetClass( int i ) {
		hfEditorReflectionState &st = hfEditor_ReflectionState();
		return ( i >= 0 && i < st.count ) ? &st.classes[i] : NULL;
	}

	static const hfEditorComponentClass *FindByName( const char *name ) {
		if ( !name ) { return NULL; }
		hfEditorReflectionState &st = hfEditor_ReflectionState();
		for ( int i = 0; i < st.count; i++ ) {
			if ( !strcmp( st.classes[i].name, name ) ) { return &st.classes[i]; }
		}
		return NULL;
	}

	static const hfEditorComponentClass *FindByTypeKey( const void *key ) {
		if ( !key ) { return NULL; }
		hfEditorReflectionState &st = hfEditor_ReflectionState();
		for ( int i = 0; i < st.count; i++ ) {
			if ( st.classes[i].typeKey == key ) { return &st.classes[i]; }
		}
		return NULL;
	}

	// ---- property access ------------------------------------------------

	static bool	ReadValue( const hfEditorComponentClass &cls, HF_Ecs::Registry &reg,
						   HF_Ecs::Entity ent, int propIndex, hfEditorValue &out ) {
		if ( propIndex < 0 || propIndex >= cls.propCount ) { return false; }
		void *base = cls.get ? cls.get( reg, ent ) : NULL;
		if ( !base ) { return false; }
		return ReadAt( base, cls.props[ propIndex ], out );
	}

	static bool	WriteValue( const hfEditorComponentClass &cls, HF_Ecs::Registry &reg,
						    HF_Ecs::Entity ent, int propIndex, const hfEditorValue &in ) {
		if ( propIndex < 0 || propIndex >= cls.propCount ) { return false; }
		void *base = cls.get ? cls.get( reg, ent ) : NULL;
		if ( !base ) { return false; }
		return WriteAt( base, cls.props[ propIndex ], in );
	}

	// offset-only accessors, exposed so a tool can bind a widget directly
	static bool	ReadAt( void *base, const hfEditorProperty &p, hfEditorValue &out ) {
		byte *mem = ( byte * )base + p.offset;
		out.type = p.type;
		switch ( p.type ) {
			case HF_EDIT_FLOAT:
				out.f[0] = *( float * )mem; return true;
			case HF_EDIT_INT:
				out.i = *( int * )mem; return true;
			case HF_EDIT_BOOL:
				out.b = *( bool * )mem; return true;
			case HF_EDIT_STRING:
				out.s = *( hfStr * )mem; return true;
			case HF_EDIT_VEC3: {
				float *v = ( float * )mem;
				out.f[0] = v[0]; out.f[1] = v[1]; out.f[2] = v[2]; out.f[3] = 0.0f;
				return true;
			}
			case HF_EDIT_QUAT: {
				float *v = ( float * )mem;
				out.f[0] = v[0]; out.f[1] = v[1]; out.f[2] = v[2]; out.f[3] = v[3];
				return true;
			}
			case HF_EDIT_ENUM:
				out.i = *( int * )mem;
				return ( p.enumNames && out.i >= 0 && out.i < p.enumCount );
		}
		return false;
	}

	static bool	WriteAt( void *base, const hfEditorProperty &p, const hfEditorValue &in ) {
		byte *mem = ( byte * )base + p.offset;
		switch ( p.type ) {
			case HF_EDIT_FLOAT:
				*( float * )mem = in.f[0]; return true;
			case HF_EDIT_INT:
				*( int * )mem = in.i; return true;
			case HF_EDIT_BOOL:
				*( bool * )mem = in.b; return true;
			case HF_EDIT_STRING:
				*( hfStr * )mem = in.s; return true;
			case HF_EDIT_VEC3: {
				float *v = ( float * )mem;
				v[0] = in.f[0]; v[1] = in.f[1]; v[2] = in.f[2];
				return true;
			}
			case HF_EDIT_QUAT: {
				float *v = ( float * )mem;
				v[0] = in.f[0]; v[1] = in.f[1]; v[2] = in.f[2]; v[3] = in.f[3];
				return true;
			}
			case HF_EDIT_ENUM:
				if ( !p.enumNames || in.i < 0 || in.i >= p.enumCount ) { return false; }
				*( int * )mem = in.i; return true;
		}
		return false;
	}
};

// --- builtin component tables --------------------------------------------
//
// One table per component the text scene format can hold. Fields the format
// does not persist are deliberately absent so the inspector never offers an
// edit that a save would throw away.

static const char *const hfEditor_LightTypeNames[] = { "point", "spot", "directional" };

static const hfEditorProperty hfEditor_TransformProps[] = {
	{ "Position",	HF_EDIT_VEC3,	( int )offsetof( HF_Scene::hfTransformComponent, position ), 3, "Transform", NULL, 0 },
	{ "Rotation",	HF_EDIT_QUAT,	( int )offsetof( HF_Scene::hfTransformComponent, rotation ), 4, "Transform", NULL, 0 },
	{ "Scale",		HF_EDIT_VEC3,	( int )offsetof( HF_Scene::hfTransformComponent, scale ),	 3, "Transform", NULL, 0 }
};

static const hfEditorProperty hfEditor_TagProps[] = {
	{ "Name",		HF_EDIT_STRING,	( int )offsetof( HF_Scene::hfTagComponent, name ),		 1, "Entity", NULL, 0 }
};

static const hfEditorProperty hfEditor_CameraProps[] = {
	{ "Field Of View",	HF_EDIT_FLOAT, ( int )offsetof( HF_Scene::hfCameraComponent, fovY ),		1, "Camera", NULL, 0 },
	{ "Aspect",			HF_EDIT_FLOAT, ( int )offsetof( HF_Scene::hfCameraComponent, aspect ),	1, "Camera", NULL, 0 },
	{ "Near Plane",		HF_EDIT_FLOAT, ( int )offsetof( HF_Scene::hfCameraComponent, nearZ ),	1, "Camera", NULL, 0 },
	{ "Far Plane",		HF_EDIT_FLOAT, ( int )offsetof( HF_Scene::hfCameraComponent, farZ ),		1, "Camera", NULL, 0 },
	{ "Clear Color",	HF_EDIT_VEC3,  ( int )offsetof( HF_Scene::hfCameraComponent, clearColor ),	3, "Camera", NULL, 0 }
};

static const hfEditorProperty hfEditor_LightProps[] = {
	{ "Type",			HF_EDIT_ENUM,	( int )offsetof( HF_Scene::hfLightComponent, type ),			1, "Light", hfEditor_LightTypeNames, 3 },
	{ "Color",			HF_EDIT_VEC3,	( int )offsetof( HF_Scene::hfLightComponent, color ),		3, "Light", NULL, 0 },
	{ "Intensity",		HF_EDIT_FLOAT,	( int )offsetof( HF_Scene::hfLightComponent, intensity ),		1, "Light", NULL, 0 },
	{ "Range",			HF_EDIT_FLOAT,	( int )offsetof( HF_Scene::hfLightComponent, range ),		1, "Light", NULL, 0 },
	{ "Spot Inner",		HF_EDIT_FLOAT,	( int )offsetof( HF_Scene::hfLightComponent, spotInnerDeg ),	1, "Light", NULL, 0 },
	{ "Spot Outer",		HF_EDIT_FLOAT,	( int )offsetof( HF_Scene::hfLightComponent, spotOuterDeg ),	1, "Light", NULL, 0 },
	{ "Casts Shadow",	HF_EDIT_BOOL,	( int )offsetof( HF_Scene::hfLightComponent, castsShadow ),	1, "Light", NULL, 0 },
	{ "Enabled",		HF_EDIT_BOOL,	( int )offsetof( HF_Scene::hfLightComponent, enabled ),		1, "Light", NULL, 0 }
};

static const hfEditorProperty hfEditor_MeshProps[] = {
	{ "Mesh Id",		HF_EDIT_INT,	 ( int )offsetof( HF_Scene::hfMeshRendererComponent, meshId ),			1, "Mesh", NULL, 0 },
	{ "Material Id",	HF_EDIT_INT,	 ( int )offsetof( HF_Scene::hfMeshRendererComponent, materialId ),		1, "Mesh", NULL, 0 },
	{ "Mesh Name",		HF_EDIT_STRING,	 ( int )offsetof( HF_Scene::hfMeshRendererComponent, meshName ),		1, "Mesh", NULL, 0 },
	{ "Casts Shadow",	HF_EDIT_BOOL,	 ( int )offsetof( HF_Scene::hfMeshRendererComponent, castsShadow ),	1, "Mesh", NULL, 0 },
	{ "Receives Shadow",HF_EDIT_BOOL,	 ( int )offsetof( HF_Scene::hfMeshRendererComponent, receiveShadow ),	1, "Mesh", NULL, 0 },
	{ "Visible",		HF_EDIT_BOOL,	 ( int )offsetof( HF_Scene::hfMeshRendererComponent, visible ),			1, "Mesh", NULL, 0 }
};

// typed thunks — one pair per component, so the editor never spells the type
HF_INLINE void *hfEditor_GetTransform( HF_Ecs::Registry &r, HF_Ecs::Entity e ) { return r.GetComponent< HF_Scene::hfTransformComponent >( e ); }
HF_INLINE void  hfEditor_AddTransform( HF_Ecs::Registry &r, HF_Ecs::Entity e ) { r.AddComponent< HF_Scene::hfTransformComponent >( e ); }
HF_INLINE bool  hfEditor_HasTransform( HF_Ecs::Registry &r, HF_Ecs::Entity e ) { return r.Has< HF_Scene::hfTransformComponent >( e ); }

HF_INLINE void *hfEditor_GetTag( HF_Ecs::Registry &r, HF_Ecs::Entity e ) { return r.GetComponent< HF_Scene::hfTagComponent >( e ); }
HF_INLINE void  hfEditor_AddTag( HF_Ecs::Registry &r, HF_Ecs::Entity e ) { r.AddComponent< HF_Scene::hfTagComponent >( e ); }
HF_INLINE bool  hfEditor_HasTag( HF_Ecs::Registry &r, HF_Ecs::Entity e ) { return r.Has< HF_Scene::hfTagComponent >( e ); }

HF_INLINE void *hfEditor_GetCamera( HF_Ecs::Registry &r, HF_Ecs::Entity e ) { return r.GetComponent< HF_Scene::hfCameraComponent >( e ); }
HF_INLINE void  hfEditor_AddCamera( HF_Ecs::Registry &r, HF_Ecs::Entity e ) { r.AddComponent< HF_Scene::hfCameraComponent >( e ); }
HF_INLINE bool  hfEditor_HasCamera( HF_Ecs::Registry &r, HF_Ecs::Entity e ) { return r.Has< HF_Scene::hfCameraComponent >( e ); }

HF_INLINE void *hfEditor_GetLight( HF_Ecs::Registry &r, HF_Ecs::Entity e ) { return r.GetComponent< HF_Scene::hfLightComponent >( e ); }
HF_INLINE void  hfEditor_AddLight( HF_Ecs::Registry &r, HF_Ecs::Entity e ) { r.AddComponent< HF_Scene::hfLightComponent >( e ); }
HF_INLINE bool  hfEditor_HasLight( HF_Ecs::Registry &r, HF_Ecs::Entity e ) { return r.Has< HF_Scene::hfLightComponent >( e ); }

HF_INLINE void *hfEditor_GetMesh( HF_Ecs::Registry &r, HF_Ecs::Entity e ) { return r.GetComponent< HF_Scene::hfMeshRendererComponent >( e ); }
HF_INLINE void  hfEditor_AddMesh( HF_Ecs::Registry &r, HF_Ecs::Entity e ) { r.AddComponent< HF_Scene::hfMeshRendererComponent >( e ); }
HF_INLINE bool  hfEditor_HasMesh( HF_Ecs::Registry &r, HF_Ecs::Entity e ) { return r.Has< HF_Scene::hfMeshRendererComponent >( e ); }

// idempotent; called by hfEditorDocument so the table exists before any use
HF_INLINE void hfEditorRegisterBuiltins() {
	if ( hfEditorReflection::FindByName( "Transform" ) ) { return; }

	hfEditorComponentClass c;

	c.name = "Transform";
	c.size = ( int )sizeof( HF_Scene::hfTransformComponent );
	c.typeKey = HF_Ecs::ComponentStore< HF_Scene::hfTransformComponent >::TypeKeyAddr();
	c.get = hfEditor_GetTransform; c.add = hfEditor_AddTransform; c.has = hfEditor_HasTransform;
	c.props = hfEditor_TransformProps;
	c.propCount = ( int )( sizeof( hfEditor_TransformProps ) / sizeof( hfEditor_TransformProps[0] ) );
	hfEditorReflection::Register( c );

	c.name = "Tag";
	c.size = ( int )sizeof( HF_Scene::hfTagComponent );
	c.typeKey = HF_Ecs::ComponentStore< HF_Scene::hfTagComponent >::TypeKeyAddr();
	c.get = hfEditor_GetTag; c.add = hfEditor_AddTag; c.has = hfEditor_HasTag;
	c.props = hfEditor_TagProps;
	c.propCount = ( int )( sizeof( hfEditor_TagProps ) / sizeof( hfEditor_TagProps[0] ) );
	hfEditorReflection::Register( c );

	c.name = "Camera";
	c.size = ( int )sizeof( HF_Scene::hfCameraComponent );
	c.typeKey = HF_Ecs::ComponentStore< HF_Scene::hfCameraComponent >::TypeKeyAddr();
	c.get = hfEditor_GetCamera; c.add = hfEditor_AddCamera; c.has = hfEditor_HasCamera;
	c.props = hfEditor_CameraProps;
	c.propCount = ( int )( sizeof( hfEditor_CameraProps ) / sizeof( hfEditor_CameraProps[0] ) );
	hfEditorReflection::Register( c );

	c.name = "Light";
	c.size = ( int )sizeof( HF_Scene::hfLightComponent );
	c.typeKey = HF_Ecs::ComponentStore< HF_Scene::hfLightComponent >::TypeKeyAddr();
	c.get = hfEditor_GetLight; c.add = hfEditor_AddLight; c.has = hfEditor_HasLight;
	c.props = hfEditor_LightProps;
	c.propCount = ( int )( sizeof( hfEditor_LightProps ) / sizeof( hfEditor_LightProps[0] ) );
	hfEditorReflection::Register( c );

	c.name = "MeshRenderer";
	c.size = ( int )sizeof( HF_Scene::hfMeshRendererComponent );
	c.typeKey = HF_Ecs::ComponentStore< HF_Scene::hfMeshRendererComponent >::TypeKeyAddr();
	c.get = hfEditor_GetMesh; c.add = hfEditor_AddMesh; c.has = hfEditor_HasMesh;
	c.props = hfEditor_MeshProps;
	c.propCount = ( int )( sizeof( hfEditor_MeshProps ) / sizeof( hfEditor_MeshProps[0] ) );
	hfEditorReflection::Register( c );
}

} // namespace HF_Editor

#endif
