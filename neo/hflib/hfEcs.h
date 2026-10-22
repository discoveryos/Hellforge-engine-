#ifndef __HF_ECS_H__
#define __HF_ECS_H__

#include "hfStr.h"
#include "hfDict.h"

// Minimal entity-component-system + event bus with no dependencies on the
// renderer or the game module so the same code serves the engine, the
// headless harness, and future gameplay frameworks.
//
// Entities are 32-bit handles carrying a generation tag; destroying an entity
// frees its slot and reuses it with an incremented generation so stale handles
// never alias a live entity.

namespace HF_Ecs {

typedef unsigned int	Entity;

static const unsigned int ENTITY_NULL		= 0u;
static const unsigned int GENERATION_MASK	= 0xff000000u;
static const unsigned int INDEX_MASK		= 0x00ffffffu;

// --- Component storage ----------------------------------------------------
class IComponentStore {
public:
	virtual ~IComponentStore() {}
	virtual void		RemoveEntity( Entity ent ) = 0;
	virtual void		Clear() = 0;
	virtual const void *TypeKey() const = 0;
};

template< typename T >
class ComponentStore : public IComponentStore {
public:
	// Unique-per-T identity (address of a per-instantiation static).
	static const void *TypeKeyAddr() { static unsigned char key; return &key; }

	struct Slot {
		Entity			owner;
		T				data;
	};

	ComponentStore() : m_slots( NULL ), m_capacity( 0 ), m_count( 0 ) {}
	~ComponentStore() { delete[] m_slots; }

	virtual void		RemoveEntity( Entity ent ) {
		for ( int i = 0; i < m_count; i++ ) {
			if ( m_slots[i].owner == ent ) {
				m_slots[i] = m_slots[m_count - 1];
				m_count--;
				break;
			}
		}
	}
	virtual void		Clear() { m_count = 0; }
	virtual const void *TypeKey() const { return TypeKeyAddr(); }

	int					GetCount() const { return m_count; }
	Entity				GetOwner( int i ) const { return m_slots[i].owner; }
	T &					GetData( int i ) { return m_slots[i].data; }
	const T &			GetData( int i ) const { return m_slots[i].data; }

	// Grow the backing store so scenes are not limited to a fixed slot count.
	void				EnsureCapacity( int need ) {
		if ( need <= m_capacity ) { return; }
		int cap = m_capacity ? m_capacity : 8;
		while ( cap < need ) { cap *= 2; }
		Slot *grown = new Slot[ cap ];
		for ( int i = 0; i < m_count; i++ ) { grown[i] = m_slots[i]; }
		delete[] m_slots;
		m_slots = grown;
		m_capacity = cap;
	}

	Slot *				m_slots;
	int					m_capacity;
	int					m_count;
};

// --- Registry -------------------------------------------------------------
class Registry {
public:
	Registry() : m_generations( new unsigned int[ INDEX_MASK + 1 ] ),
	             m_alive( new byte[ INDEX_MASK + 1 ] ),
	             m_freeList( new unsigned int[ INDEX_MASK + 1 ] ),
	             m_freeCount( 0 ) {
		for ( unsigned int i = 0; i < INDEX_MASK + 1; i++ ) {
			m_generations[ i ] = 0;
			m_alive[ i ] = 0;
		}
	}
	~Registry() {
		delete[] m_freeList;
		delete[] m_alive;
		delete[] m_generations;
	}

	Entity				Create() {
		unsigned int idx;
		unsigned int gen;
		if ( m_freeCount > 0 ) {
			idx = m_freeList[--m_freeCount];
			gen = m_generations[idx] + 1;
		} else {
			if ( m_liveCount >= INDEX_MASK ) { return ENTITY_NULL; }
			idx = m_liveCount++;
			gen = 1;
		}
		m_generations[idx] = gen;
		m_alive[idx] = true;
		return ( gen << 24 ) | idx;
	}

	void				Destroy( Entity ent ) {
		unsigned int idx = ent & INDEX_MASK;
		if ( !IsAlive( ent ) ) { return; }
		m_alive[idx] = false;
		for ( int s = 0; s < m_storeCount; s++ ) {
			m_stores[s]->RemoveEntity( ent );
		}
		m_freeList[m_freeCount++] = idx;
	}

	bool				IsAlive( Entity ent ) const {
		unsigned int idx = ent & INDEX_MASK;
		if ( idx >= m_liveCount ) { return false; }
		if ( !m_alive[idx] ) { return false; }
		unsigned int gen = ent >> 24;
		if ( gen == 0 ) { return false; }
		return m_generations[idx] == gen;
	}

	int					GetLiveCount() const { return m_liveCount - m_freeCount; }

	template< typename T >
	T &					AddComponent( Entity ent ) {
		ComponentStore< T > *store = ( ComponentStore< T > * )( IComponentStore * )GetStore< T >();
		int idx = store->GetCount();
		store->EnsureCapacity( idx + 1 );
		store->m_slots[idx].owner = ent;
		store->m_count = idx + 1;
		return store->m_slots[idx].data;
	}

	template< typename T >
	T *					GetComponent( Entity ent ) {
		ComponentStore< T > *store = ( ComponentStore< T > * )( IComponentStore * )GetStore< T >();
		for ( int i = 0; i < store->GetCount(); i++ ) {
			if ( store->GetOwner( i ) == ent ) { return &store->GetData( i ); }
		}
		return NULL;
	}

	template< typename T >
	bool				Has( Entity ent ) const { return const_cast< Registry * >( this )->GetComponent< T >( ent ) != NULL; }

	// Call callback( Entity ent, T& ) for every entity that has a T.
	template< typename T, typename Fn >
	void				Each( const Fn &callback ) {
		ComponentStore< T > *store = ( ComponentStore< T > * )( IComponentStore * )GetStore< T >();
		for ( int i = 0; i < store->GetCount(); i++ ) {
			callback( store->GetOwner( i ), store->GetData( i ) );
		}
	}

	void				Clear() {
		for ( int s = 0; s < m_storeCount; s++ ) { m_stores[s]->Clear(); }
		for ( unsigned int i = 0; i < m_liveCount; i++ ) { m_alive[i] = false; }
		m_liveCount = 0;
		m_freeCount = 0;
	}

protected:
	template< typename T >
	IComponentStore *	GetStore() {
		const void *key = ComponentStore< T >::TypeKeyAddr();
		for ( int s = 0; s < m_storeCount; s++ ) {
			if ( m_stores[s]->TypeKey() == key ) { return m_stores[s]; }
		}
		ComponentStore< T > *store = new ComponentStore< T >();
		if ( m_storeCount < 64 ) { m_stores[m_storeCount++] = store; }
		return store;
	}

	unsigned int		m_liveCount = 0;
	unsigned int *		m_generations;			// heap-backed; INDEX_MASK+1 entries each
	byte *				m_alive;
	unsigned int *		m_freeList;
	unsigned int		m_freeCount = 0;
	IComponentStore *	m_stores[64];
	int					m_storeCount = 0;
};

// --- Event bus ------------------------------------------------------------
// Simple in-process publish/subscribe. Events are named; handlers receive a
// typed payload (data + size). Ordered by subscription.
typedef void ( *eventHandler_t )( const char *eventName, const void *data, int dataSize, void *user );

class EventBus {
public:
	void				Subscribe( const char *eventName, eventHandler_t handler, void *user ) {
		int idx = FindChannel( eventName );
		if ( idx < 0 ) {
			if ( m_channelCount >= 16 ) { return; }
			idx = m_channelCount++;
			m_channels[idx].name = eventName;
		}
		Channel &ch = m_channels[idx];
		if ( ch.handlerCount < 8 ) {
			ch.handlers[ch.handlerCount].fn = handler;
			ch.handlers[ch.handlerCount].user = user;
			ch.handlerCount++;
		}
	}

	void				Publish( const char *eventName, const void *data = NULL, int dataSize = 0 ) {
		int idx = FindChannel( eventName );
		if ( idx < 0 ) { return; }
		Channel &ch = m_channels[idx];
		for ( int i = 0; i < ch.handlerCount; i++ ) {
			ch.handlers[i].fn( eventName, data, dataSize, ch.handlers[i].user );
		}
	}

	int					GetChannelCount() const { return m_channelCount; }

protected:
	struct Handler { eventHandler_t fn; void *user; };
	// handlerCount must be zero-initialized: Channel is default-constructed as
	// part of the m_channels array, and Subscribe indexes handlers[] by it.
	struct Channel {
		hfStr				name;
		Handler				handlers[8];
		int					handlerCount = 0;
	};

	int					FindChannel( const char *eventName ) const {
		for ( int i = 0; i < m_channelCount; i++ ) {
			if ( !hfStr::Icmp( m_channels[i].name.c_str(), eventName ) ) { return i; }
		}
		return -1;
	}

	Channel				m_channels[16];
	int					m_channelCount = 0;
};

} // namespace HF_Ecs

#endif