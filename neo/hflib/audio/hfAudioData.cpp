#include "hfprecompiled.h"
#include "hfAudioData.h"

namespace HF_Audio {

void hfSoundBank::Init() {
	m_numSlots = 0;
	m_numSamples = 0;
	for ( int i = 0; i < MAX_SLOTS; i++ ) {
		m_slots[i].key = 0;
		m_slots[i].sampleId = 0;
		m_slots[i].generation = 0;
		m_slots[i].alive = false;
	}
	for ( int i = 0; i < 256; i++ ) { m_sampleNames[i] = ""; }
}

unsigned int hfSoundBank::Alloc( unsigned int sampleId ) {
	// reuse a freed slot first
	for ( int i = 0; i < MAX_SLOTS; i++ ) {
		if ( !m_slots[i].alive ) {
			m_slots[i].alive = true;
			m_slots[i].sampleId = sampleId;
			m_slots[i].generation++;
			m_slots[i].key = hfSoundKey::Make( i, m_slots[i].generation );
			if ( m_numSlots < i + 1 ) { m_numSlots = i + 1; }
			return m_slots[i].key;
		}
	}
	return 0;	// bank full
}

void hfSoundBank::Free( unsigned int key ) {
	if ( !key ) { return; }
	unsigned int i = hfSoundKey::IndexOf( key );
	if ( i >= ( unsigned int )MAX_SLOTS ) { return; }
	Slot &s = m_slots[i];
	if ( s.alive && s.key == key ) { s.alive = false; }
}

bool hfSoundBank::IsAlive( unsigned int key ) const {
	if ( !key ) { return false; }
	unsigned int i = hfSoundKey::IndexOf( key );
	if ( i >= ( unsigned int )MAX_SLOTS ) { return false; }
	return m_slots[i].alive && m_slots[i].key == key;
}

hfSoundBank::Slot *hfSoundBank::Get( unsigned int key ) {
	if ( !IsAlive( key ) ) { return NULL; }
	return &m_slots[ hfSoundKey::IndexOf( key ) ];
}

const hfSoundBank::Slot *hfSoundBank::Get( unsigned int key ) const {
	if ( !IsAlive( key ) ) { return NULL; }
	return &m_slots[ hfSoundKey::IndexOf( key ) ];
}

unsigned int hfSoundBank::AddSample( const char *name ) {
	if ( !name || !name[0] ) { return 0; }
	hfStr lower = name;
	lower.ToLower();
	for ( int i = 0; i < m_numSamples; i++ ) {
		if ( m_sampleNames[i].Icmp( lower.c_str() ) == 0 ) { return ( unsigned int )i; }
	}
	if ( m_numSamples >= 256 ) { return 0; }
	m_sampleNames[m_numSamples] = lower;
	return ( unsigned int )m_numSamples++;
}

int hfSoundBank::FindSample( const char *name ) const {
	if ( !name ) { return -1; }
	hfStr lower = name;
	lower.ToLower();
	for ( int i = 0; i < m_numSamples; i++ ) {
		if ( m_sampleNames[i].Icmp( lower.c_str() ) == 0 ) { return i; }
	}
	return -1;
}

} // namespace HF_Audio