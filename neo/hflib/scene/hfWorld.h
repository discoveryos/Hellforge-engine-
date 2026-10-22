#ifndef __HF_SCENE_WORLD_H__
#define __HF_SCENE_WORLD_H__

// The scene/world container: owns the entity registry, the built-in core
// systems (hierarchy, transforms, cameras) and any user-registered systems,
// and advances them all each Update().  Mirrors the role of SynapseEngine's
// Scene (plus its System/ pipeline) — one world = one game scene.

#include "../hfEcs.h"
#include "../hfStr.h"
#include "hfSystem.h"
#include "hfTransformComponent.h"
#include "hfHierarchyComponent.h"
#include "hfTagComponent.h"
#include "hfCameraComponent.h"
#include "hfHierarchySystem.h"
#include "hfTransformSystem.h"
#include "hfCameraSystem.h"

namespace HF_Scene {

class hfWorld {
public:
	HF_INLINE hfWorld() : m_systemCount( 0 ), m_time( 0.0f ) {
		for ( int i = 0; i < 32; i++ ) { m_systems[i] = NULL; }
		m_transforms.LinkHierarchy( &m_hierarchy );
		RegisterSystem( &m_hierarchy );
		RegisterSystem( &m_transforms );
		RegisterSystem( &m_cameras );
	}

	HF_INLINE ~hfWorld() {}

	HF_Ecs::Registry &			GetRegistry() { return m_registry; }
	const HF_Ecs::Registry &	GetRegistry() const { return m_registry; }

	// ---- entities ----------------------------------------------------
	HF_Ecs::Entity				CreateEntity() {
		HF_Ecs::Entity e = m_registry.Create();
		if ( e != HF_Ecs::ENTITY_NULL ) {
			m_registry.AddComponent< hfHierarchyComponent >( e );
			m_registry.AddComponent< hfTransformComponent >( e );
		}
		return e;
	}
	HF_Ecs::Entity				CreateEntity( const char *tag ) {
		HF_Ecs::Entity e = CreateEntity();
		if ( e != HF_Ecs::ENTITY_NULL ) {
			m_registry.AddComponent< hfTagComponent >( e ).name = tag;
		}
		return e;
	}
	void						DestroyEntity( HF_Ecs::Entity e ) {
		m_hierarchy.PreDestroy( e );
		m_registry.Destroy( e );
	}
	HF_Ecs::Entity				FindEntityByTag( const char *tag ) {
		HF_Ecs::Entity found = HF_Ecs::ENTITY_NULL;
		m_registry.Each< hfTagComponent >(
			[&]( HF_Ecs::Entity e, hfTagComponent & t ) {
				if ( found == HF_Ecs::ENTITY_NULL && !hfStr::Icmp( t.name.c_str(), tag ) ) { found = e; }
			} );
		return found;
	}
	void						Clear() {
		m_hierarchy.Clear();
		m_registry.Clear();
		m_time = 0.0f;
	}

	// ---- components --------------------------------------------------
	template< typename T >
	T &							AddComponent( HF_Ecs::Entity e ) { return m_registry.AddComponent< T >( e ); }
	template< typename T >
	T *							GetComponent( HF_Ecs::Entity e ) { return m_registry.GetComponent< T >( e ); }
	template< typename T >
	bool						HasComponent( HF_Ecs::Entity e ) const { return m_registry.Has< T >( e ); }

	// ---- hierarchy convenience --------------------------------------
	void						SetParent( HF_Ecs::Entity child, HF_Ecs::Entity parent ) { m_hierarchy.SetParent( child, parent ); }
	HF_Ecs::Entity				GetParent( HF_Ecs::Entity e ) const { return m_hierarchy.GetParent( e ); }
	int							GetChildren( HF_Ecs::Entity e, HF_Ecs::Entity *out, int max ) const { return m_hierarchy.GetChildren( e, out, max ); }

	// ---- systems -----------------------------------------------------
	void						RegisterSystem( hfSystem *system ) {
		if ( !system || m_systemCount >= 32 ) { return; }
		int i = m_systemCount;
		while ( i > 0 && m_systems[i - 1]->GetPriority() > system->GetPriority() ) {
			m_systems[i] = m_systems[i - 1];
			i--;
		}
		m_systems[i] = system;
		m_systemCount++;
		system->OnRegister( m_registry );
	}
	hfSystem *					FindSystem( const char *name ) {
		for ( int i = 0; i < m_systemCount; i++ ) {
			if ( !hfStr::Icmp( m_systems[i]->GetName(), name ) ) { return m_systems[i]; }
		}
		return NULL;
	}
	void						Update( float dt ) {
		m_time += dt;
		for ( int i = 0; i < m_systemCount; i++ ) {
			m_systems[i]->Update( m_registry, dt );
		}
	}
	float						GetTime() const { return m_time; }

	// ---- built-in core systems (owned) ------------------------------
	HF_Scene::hfHierarchySystem &	GetHierarchy() { return m_hierarchy; }
	HF_Scene::hfTransformSystem &	GetTransforms() { return m_transforms; }
	HF_Scene::hfCameraSystem &		GetCameras() { return m_cameras; }

private:
	HF_Ecs::Registry			m_registry;
	hfHierarchySystem			m_hierarchy;
	hfTransformSystem			m_transforms;
	hfCameraSystem				m_cameras;
	hfSystem *					m_systems[ 32 ];
	int							m_systemCount;
	float						m_time;
};

} // namespace HF_Scene

#endif