#ifndef __HF_RESOURCE_MANAGER_H__
#define __HF_RESOURCE_MANAGER_H__

// Typed resource registry over the packed asset store. Meshes, materials,
// textures and shaders are tracked by name as resources in one table, with
// ref counting for stream/editor lifecycles and a generation counter for hot
// reload (Bump marks a resource dirty; IsDirty/AckDirty gate re-upload).
// Mirrors the SynapseEngine Manager/ResourceManager role.

#include "../hfStr.h"
#include "../hfAssetStore.h"

namespace HF_Res {

enum hfResourceKind {
	HF_RK_MESH = 0,
	HF_RK_MATERIAL,
	HF_RK_TEXTURE,
	HF_RK_SHADER,
	HF_RK_COUNT
};

struct hfResourceSlot {
	unsigned int	nameHash;
	hfStr			name;
	hfResourceKind	kind;
	unsigned int	generation;		// bumped every hot reload
	bool			dirty;			// generation bumped since last AckDirty
	unsigned int	refCount;

	HF_INLINE hfResourceSlot() {
		nameHash = 0;
		kind = HF_RK_MESH;
		generation = 0;
		dirty = false;
		refCount = 0;
	}
};

static const int HF_MAX_RESOURCES = 4096;

class hfResourceManager {
public:
	hfResourceManager()
		: m_count( 0 ), m_store( NULL ), m_storeSize( 0 ) {}

	// The manager keeps a borrowed pointer to a PackedStore's data (the
	// caller / asset library owns the bytes) and validates them eagerly.
	bool LoadStore( const byte *store, int storeSize, hfStr *error = NULL ) {
		m_store = store;
		m_storeSize = storeSize;
		if ( !store || storeSize <= 0 ) {
			if ( error ) { *error = "empty store"; }
			return false;
		}
		if ( !HF_Asset::Validate( store, storeSize, error ) ) {
			m_store = NULL;
			m_storeSize = 0;
			return false;
		}
		return true;
	}

	bool HasStore() const { return m_store != NULL; }

	bool SetStore( const HF_Asset::PackedStore &pack ) {
		return LoadStore( pack.data, pack.size );
	}

	// Register a typed resource backed by the store. Returns its slot or -1.
	int Create( const char *name, hfResourceKind kind ) {
		if ( m_count >= HF_MAX_RESOURCES || !name || !name[0] ) { return -1; }
		for ( int i = 0; i < m_count; i++ ) {
			if ( m_slots[i].name.Icmp( name ) == 0 ) { return i; }	// idempotent
		}
		m_slots[m_count].name = name;
		m_slots[m_count].nameHash = HF_Asset::LowerCaseHash( name );
		m_slots[m_count].kind = kind;
		m_count++;
		return m_count - 1;
	}

	int Find( const char *name ) const {
		if ( !name ) { return -1; }
		for ( int i = 0; i < m_count; i++ ) {
			if ( m_slots[i].name.Icmp( name ) == 0 ) { return i; }
		}
		return -1;
	}

	bool Has( const char *name ) const { return Find( name ) >= 0; }

	// Pull the raw bytes for a resource straight out of the pack.
	bool Extract( const char *name, hfStr &out ) const {
		if ( !m_store ) { return false; }
		return HF_Asset::Extract( m_store, m_storeSize, name, out );
	}

	// ---- ref counting ----
	void IncRef( int slot ) {
		if ( slot >= 0 && slot < m_count ) { m_slots[slot].refCount++; }
	}
	void DecRef( int slot ) {
		if ( slot >= 0 && slot < m_count && m_slots[slot].refCount > 0 ) {
			m_slots[slot].refCount--;
		}
	}
	unsigned int GetRefCount( int slot ) const {
		return ( slot >= 0 && slot < m_count ) ? m_slots[slot].refCount : 0u;
	}

	// ---- hot reload ----
	void Bump( int slot ) {
		if ( slot >= 0 && slot < m_count ) { m_slots[slot].generation++; m_slots[slot].dirty = true; }
	}
	bool IsDirty( int slot ) const {
		return slot >= 0 && slot < m_count && m_slots[slot].dirty;
	}
	void AckDirty( int slot ) {
		if ( slot >= 0 && slot < m_count ) { m_slots[slot].dirty = false; }
	}
	unsigned int GetGeneration( int slot ) const {
		return ( slot >= 0 && slot < m_count ) ? m_slots[slot].generation : 0u;
	}

	int NumResources() const { return m_count; }
	const hfResourceSlot *GetSlot( int slot ) const {
		return ( slot >= 0 && slot < m_count ) ? &m_slots[slot] : NULL;
	}

private:
	hfResourceSlot		m_slots[ HF_MAX_RESOURCES ];
	int					m_count;
	const byte *		m_store;
	int					m_storeSize;
};

} // namespace HF_Res

#endif