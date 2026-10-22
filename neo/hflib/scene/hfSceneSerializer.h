#ifndef __HF_SCENE_SERIALIZER_H__
#define __HF_SCENE_SERIALIZER_H__

// Text scene file save/load (round-trippable). Format is one directive per
// line, '#'-comments, quoted strings for names:
//
//   entity
//   tag "root"
//   parent "root"          (may reference any entity, bound on load)
//   pos 0 0 0
//   rot 0 0 -0.70710678 0.70710678      (quat x y z w)
//   scale 1 1 1
//   camera 75 1.7777 0.1 1000 0.05 0.05 0.08 1     (fov aspect near far rgba)
//   light point 1 1 1 1 10 20 30 1 1               (r g b intensity range inner outer shadow enabled)
//   mesh 13 7 -1 -1 -1 1 1 1 "box" 1 1 1           (id mat bounds name casts receives visible)
//
// A slim, dependency-free reader/writer (no filesystem, no GL) — the scene
// text can live in an asset pack or on disk, mirroring Synapse's
// Scene/Loader role.

#include "hfWorld.h"
#include "hfLightComponent.h"
#include "hfMeshRendererComponent.h"

namespace HF_Scene {

class hfSceneWriter {
public:
	// Serialize every tagged/transform entity in 'world' to 'out'.
	static bool Write( HF_Scene::hfWorld &world, hfStr &out ) {
		out = "";
		HF_Ecs::Registry &reg = world.GetRegistry();
		int wrote = 0;
		reg.Each< hfTransformComponent >(
			[ &world, &reg, &out, &wrote ]( HF_Ecs::Entity e, hfTransformComponent &t ) {
				out += "entity\n";
				hfTagComponent *tag = reg.GetComponent< hfTagComponent >( e );
				if ( tag && tag->name.Length() ) { out += va( "tag \"%s\"\n", tag->name.c_str() ); }
				HF_Ecs::Entity parent = world.GetParent( e );
				if ( parent != HF_Ecs::ENTITY_NULL ) {
					hfTagComponent *pt = reg.GetComponent< hfTagComponent >( parent );
					if ( pt && pt->name.Length() ) { out += va( "parent \"%s\"\n", pt->name.c_str() ); }
				}
				out += va( "pos %.7g %.7g %.7g\n", t.position.x, t.position.y, t.position.z );
				out += va( "rot %.7g %.7g %.7g %.7g\n", t.rotation.x, t.rotation.y, t.rotation.z, t.rotation.w );
				out += va( "scale %.7g %.7g %.7g\n", t.scale.x, t.scale.y, t.scale.z );
				hfCameraComponent *cam = reg.GetComponent< hfCameraComponent >( e );
				if ( cam ) {
					out += va( "camera %.7g %.7g %.7g %.7g %.7g %.7g %.7g %.7g\n",
						cam->fovY, cam->aspect, cam->nearZ, cam->farZ,
						cam->clearColor.x, cam->clearColor.y, cam->clearColor.z, cam->clearColor.w );
				}
				hfLightComponent *light = reg.GetComponent< hfLightComponent >( e );
				if ( light ) {
					const char *kind = light->type == HF_LIGHT_SPOT ? "spot"
								 : ( light->type == HF_LIGHT_DIRECTIONAL ? "directional" : "point" );
					out += va( "light %s %.7g %.7g %.7g %.7g %.7g %.7g %.7g %d %d\n",
						kind, light->color.x, light->color.y, light->color.z,
						light->intensity, light->range, light->spotInnerDeg, light->spotOuterDeg,
						light->castsShadow ? 1 : 0, light->enabled ? 1 : 0 );
				}
				hfMeshRendererComponent *mr = reg.GetComponent< hfMeshRendererComponent >( e );
				if ( mr ) {
					out += va( "mesh %d %d %.7g %.7g %.7g %.7g %.7g %.7g \"%s\" %d %d %d\n",
						mr->meshId, mr->materialId,
						mr->localBounds[0].x, mr->localBounds[0].y, mr->localBounds[0].z,
						mr->localBounds[1].x, mr->localBounds[1].y, mr->localBounds[1].z,
						mr->meshName.c_str(),
						mr->castsShadow ? 1 : 0, mr->receiveShadow ? 1 : 0, mr->visible ? 1 : 0 );
				}
				wrote++;
			} );
		return wrote > 0;
	}
};

class hfSceneReader {
public:
	struct PendingParent {
		HF_Ecs::Entity	child;
		hfStr			tag;
	};

	static HF_INLINE bool NextToken( const char *&cur, hfStr &out ) {
		while ( *cur ) {
			if ( *cur == ' ' || *cur == '\t' || *cur == '\r' || *cur == '\n' ) { cur++; continue; }
			if ( *cur == '#' ) { while ( *cur && *cur != '\n' ) { cur++; } continue; }
			break;
		}
		if ( !*cur ) { return false; }
		out = "";
		if ( *cur == '"' ) {
			cur++;
			while ( *cur && *cur != '"' ) { out += *cur++ ; }
			if ( *cur == '"' ) { cur++; }
			return true;
		}
		while ( *cur && *cur != ' ' && *cur != '\t' && *cur != '\r' && *cur != '\n' && *cur != '#' ) {
			out += *cur++;
		}
		return true;
	}

	// Build a fresh copy of the scene into 'world'. Returns false on error.
	static bool Read( HF_Scene::hfWorld &world, const char *text ) {
		if ( !text ) { return false; }
		HF_Ecs::Entity cur = HF_Ecs::ENTITY_NULL;
		HF_Ecs::Entity pending[ 512 ];
		for ( int i = 0; i < 512; i++ ) { pending[i] = HF_Ecs::ENTITY_NULL; }
		hfStr parentTags[ 512 ];
		int pendingCount = 0;

		const char *p = text;
		hfStr tok;
		while ( NextToken( p, tok ) ) {
			if ( tok == "entity" ) {
				cur = world.CreateEntity();
			} else if ( cur == HF_Ecs::ENTITY_NULL ) {
				continue;	// stray directive before the first entity
			} else if ( tok == "tag" ) {
				hfStr name;
				if ( NextToken( p, name ) ) { world.AddComponent< hfTagComponent >( cur ).name = name; }
			} else if ( tok == "parent" ) {
				hfStr name;
				if ( NextToken( p, name ) && pendingCount < 512 ) {
					pending[pendingCount] = cur;
					parentTags[pendingCount] = name;
					pendingCount++;
				}
			} else if ( tok == "pos" ) {
				float x, y, z;
				if ( NextFloat( p, x ) && NextFloat( p, y ) && NextFloat( p, z ) ) {
					world.GetComponent< hfTransformComponent >( cur )->SetPosition( hfVec3( x, y, z ) );
				}
			} else if ( tok == "rot" ) {
				float x, y, z, w;
				if ( NextFloat( p, x ) && NextFloat( p, y ) && NextFloat( p, z ) && NextFloat( p, w ) ) {
					world.GetComponent< hfTransformComponent >( cur )->SetRotation( hfQuat( x, y, z, w ) );
				}
			} else if ( tok == "scale" ) {
				float x, y, z;
				if ( NextFloat( p, x ) && NextFloat( p, y ) && NextFloat( p, z ) ) {
					world.GetComponent< hfTransformComponent >( cur )->SetScale( hfVec3( x, y, z ) );
				}
			} else if ( tok == "camera" ) {
				hfCameraComponent &cam = world.AddComponent< hfCameraComponent >( cur );
				float v;
				if ( NextFloat( p, v ) ) { cam.fovY = v; }
				if ( NextFloat( p, v ) ) { cam.aspect = v; }
				if ( NextFloat( p, v ) ) { cam.nearZ = v; }
				if ( NextFloat( p, v ) ) { cam.farZ = v; }
				if ( NextFloat( p, v ) ) { cam.clearColor.x = v; }
				if ( NextFloat( p, v ) ) { cam.clearColor.y = v; }
				if ( NextFloat( p, v ) ) { cam.clearColor.z = v; }
				if ( NextFloat( p, v ) ) { cam.clearColor.w = v; }
				cam.ComputeProjection();
			} else if ( tok == "light" ) {
				hfStr kind;
				if ( !NextToken( p, kind ) ) { continue; }
				hfLightComponent &light = world.AddComponent< hfLightComponent >( cur );
				light.type = kind == "spot" ? HF_LIGHT_SPOT
						 : ( kind == "directional" ? HF_LIGHT_DIRECTIONAL : HF_LIGHT_POINT );
				float v;
				if ( NextFloat( p, v ) ) { light.color.x = v; }
				if ( NextFloat( p, v ) ) { light.color.y = v; }
				if ( NextFloat( p, v ) ) { light.color.z = v; }
				if ( NextFloat( p, v ) ) { light.intensity = v; }
				if ( NextFloat( p, v ) ) { light.range = v; }
				if ( NextFloat( p, v ) ) { light.spotInnerDeg = v; }
				if ( NextFloat( p, v ) ) { light.spotOuterDeg = v; }
				int cs = 1;
				if ( NextInt( p, cs ) ) { light.castsShadow = cs != 0; }
				int en = 1;
				if ( NextInt( p, en ) ) { light.enabled = en != 0; }
			} else if ( tok == "mesh" ) {
				hfMeshRendererComponent &mr = world.AddComponent< hfMeshRendererComponent >( cur );
				int mi, ma;
				if ( NextInt( p, mi ) ) { mr.meshId = mi; }
				if ( NextInt( p, ma ) ) { mr.materialId = ma; }
				float v;
				for ( int c = 0; c < 6 && NextFloat( p, v ); c++ ) {
					( &mr.localBounds[0].x )[c] = v;	// mins.x..maxs.z in order
				}
				hfStr name;
				if ( NextToken( p, name ) ) { mr.meshName = name; }
				int flag = 1;
				if ( NextInt( p, flag ) ) { mr.castsShadow = flag != 0; }
				if ( NextInt( p, flag ) ) { mr.receiveShadow = flag != 0; }
				if ( NextInt( p, flag ) ) { mr.visible = flag != 0; }
			}
		}

		// Second pass: bind parent references by tag.
		for ( int i = 0; i < pendingCount; i++ ) {
			HF_Ecs::Entity par = world.FindEntityByTag( parentTags[i].c_str() );
			if ( par != HF_Ecs::ENTITY_NULL ) { world.SetParent( pending[i], par ); }
		}
		return true;
	}

private:
	static HF_INLINE bool NextFloat( const char *&p, float &out ) {
		hfStr tok;
		if ( !NextToken( p, tok ) ) { return false; }
		out = ( float )atof( tok.c_str() );
		return true;
	}
	static HF_INLINE bool NextInt( const char *&p, int &out ) {
		hfStr tok;
		if ( !NextToken( p, tok ) ) { return false; }
		out = atoi( tok.c_str() );
		return true;
	}
};

} // namespace HF_Scene

#endif