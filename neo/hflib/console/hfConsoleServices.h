#ifndef __HF_CONSOLE_SERVICES_H__
#define __HF_CONSOLE_SERVICES_H__

#include "../hfprecompiled.h"

/*
===============================================================================

	Console platform services.

	This is the SDK-free seam a game-console target (PlayStation, Xbox, Switch)
	fills in.  A real port implements hfConsoleServices against its platform
	SDK (achievements, stats, voice, per-user storage) and installs it with
	HfConsole_InstallServices() before the game boots.  Until a platform is
	installed, HfConsole_GetServices() returns the deterministic offline stub,
	which is what the harness pins the contract against.

	Everything here is header-only, allocation-light and free of wall-clock
	time, so a fixed sequence of operations is bit-for-bit reproducible.

===============================================================================
*/

namespace hfconsole {

// one logical controller / signed-in account
struct hfConsoleUserId {
	uint64				value;

	explicit hfConsoleUserId( uint64 v = 0 ) : value( v ) {}
	bool				operator==( const hfConsoleUserId & o ) const { return value == o.value; }
	bool				operator!=( const hfConsoleUserId & o ) const { return value != o.value; }
};

enum hfConsoleVoiceState {
	HF_VOICE_OFF = 0,
	HF_VOICE_LISTENING = 1,
	HF_VOICE_TALKING = 2,
};

class hfConsoleServices {
public:
	static const int	HF_CONSOLE_MAX_USERS = 4;
	static const int	HF_CONSOLE_MAX_ACHIEVEMENTS = 128;
	static const int	HF_CONSOLE_MAX_STATS = 64;
	static const int	HF_CONSOLE_MAX_SLOTS = 8;
	static const int	HF_CONSOLE_MAX_SLOT_BYTES = 4096;

	virtual				~hfConsoleServices() {}

	// --- sign-in / local users ------------------------------------------
	virtual int			GetNumSignedInUsers() const = 0;
	virtual hfConsoleUserId	GetUserId( int index ) const = 0;
	virtual const char * GetGamerTag( int index ) const = 0;
	virtual bool		CanPlayOnline( int index ) const = 0;
	virtual int			GetInputDevice( int index ) const = 0;
	virtual void		Pump() {}

	// --- achievements ----------------------------------------------------
	virtual bool		IsInitialized() const = 0;
	virtual bool		IsAchievementLocked( hfConsoleUserId user, int id ) const = 0;
	virtual void		UnlockAchievement( hfConsoleUserId user, int id ) = 0;

	// --- stats -----------------------------------------------------------
	virtual void		SetStatInt( hfConsoleUserId user, int stat, int value ) = 0;
	virtual int			GetStatInt( hfConsoleUserId user, int stat ) const = 0;
	virtual void		SetStatFloat( hfConsoleUserId user, int stat, float value ) = 0;
	virtual float		GetStatFloat( hfConsoleUserId user, int stat ) const = 0;

	// --- voice chat ------------------------------------------------------
	virtual hfConsoleVoiceState	GetVoiceState( hfConsoleUserId user ) const = 0;
	virtual void		SetVoiceMuted( hfConsoleUserId user, bool muted ) = 0;
	virtual bool		IsVoiceMuted( hfConsoleUserId user ) const = 0;

	// --- per-user storage (console save segregation) --------------------
	// Returns false if the slot exceeds HF_CONSOLE_MAX_SLOT_BYTES.
	virtual bool		WriteSlot( hfConsoleUserId user, const char * slot, const byte * data, int dataSize ) = 0;
	// Returns the number of bytes read, or 0 if the slot does not exist.
	// Writes at most maxSize bytes into out.
	virtual int			ReadSlot( hfConsoleUserId user, const char * slot, byte * out, int maxSize ) const = 0;
	virtual int			SlotSize( hfConsoleUserId user, const char * slot ) const = 0;
	virtual void		DeleteSlot( hfConsoleUserId user, const char * slot ) = 0;
};

struct hfConsoleSlot {
				hfConsoleSlot() : userId( 0 ), dataSize( 0 ) {}

	hfConsoleUserId	userId;
	hfStr			name;
	byte			data[ hfConsoleServices::HF_CONSOLE_MAX_SLOT_BYTES ];
	int				dataSize;
};

// Deterministic offline implementation.  Every decision is a pure function of
// the operations applied to it, so two stubs driven identically are equal.
class hfConsoleServicesStub : public hfConsoleServices {
public:
						hfConsoleServicesStub();

	// --- sign-in / local users ------------------------------------------
	virtual int			GetNumSignedInUsers() const { return signedInUsers; }
	virtual hfConsoleUserId	GetUserId( int index ) const;
	virtual const char * GetGamerTag( int index ) const { return ( index >= 0 && index < HF_CONSOLE_MAX_USERS ) ? gamerTag : ""; }
	virtual bool		CanPlayOnline( int index ) const { return true; }
	virtual int			GetInputDevice( int index ) const { return index; }
	virtual void		Pump() {}

	// --- achievements ----------------------------------------------------
	virtual bool		IsInitialized() const { return true; }
	virtual bool		IsAchievementLocked( hfConsoleUserId user, int id ) const;
	virtual void		UnlockAchievement( hfConsoleUserId user, int id );

	// --- stats -----------------------------------------------------------
	virtual void		SetStatInt( hfConsoleUserId user, int stat, int value );
	virtual int			GetStatInt( hfConsoleUserId user, int stat ) const;
	virtual void		SetStatFloat( hfConsoleUserId user, int stat, float value );
	virtual float		GetStatFloat( hfConsoleUserId user, int stat ) const;

	// --- voice chat ------------------------------------------------------
	virtual hfConsoleVoiceState	GetVoiceState( hfConsoleUserId user ) const;
	virtual void		SetVoiceMuted( hfConsoleUserId user, bool muted );
	virtual bool		IsVoiceMuted( hfConsoleUserId user ) const;

	// --- per-user storage ------------------------------------------------
	virtual bool		WriteSlot( hfConsoleUserId user, const char * slot, const byte * data, int dataSize );
	virtual int			ReadSlot( hfConsoleUserId user, const char * slot, byte * out, int maxSize ) const;
	virtual int			SlotSize( hfConsoleUserId user, const char * slot ) const;
	virtual void		DeleteSlot( hfConsoleUserId user, const char * slot );

private:
	static constexpr uint64	USER_ID_BASE = 0x01000000;

	int					FindSlot( hfConsoleUserId user, const char * slot ) const;
	int					UserIndex( hfConsoleUserId user ) const;

	int					signedInUsers;
	hfStr				gamerTag;

	bool				achievementLocked[ HF_CONSOLE_MAX_USERS ][ HF_CONSOLE_MAX_ACHIEVEMENTS ];
	int					statInt[ HF_CONSOLE_MAX_USERS ][ HF_CONSOLE_MAX_STATS ];
	float				statFloat[ HF_CONSOLE_MAX_USERS ][ HF_CONSOLE_MAX_STATS ];
	hfConsoleVoiceState	voiceState[ HF_CONSOLE_MAX_USERS ];
	bool				voiceMuted[ HF_CONSOLE_MAX_USERS ];
	hfStaticList< hfConsoleSlot, HF_CONSOLE_MAX_SLOTS > slots;
};

// --- active platform ------------------------------------------------------

// A real console target installs its SDK-backed implementation once at boot.
// Without an install the offline stub is used, so the engine runs unchanged
// on a dev box and the harness can validate the contract headlessly.
HF_INLINE hfConsoleServices * & HfConsole_PlatformSlot() {
	static hfConsoleServices * platform = NULL;
	return platform;
}

HF_INLINE void HfConsole_InstallServices( hfConsoleServices * services ) {
	HfConsole_PlatformSlot() = services;
}

HF_INLINE hfConsoleServices * HfConsole_GetServices() {
	if ( HfConsole_PlatformSlot() != NULL ) {
		return HfConsole_PlatformSlot();
	}
	static hfConsoleServicesStub stub;
	return &stub;
}

HF_INLINE hfConsoleServicesStub::hfConsoleServicesStub() : signedInUsers( 1 ) {
	gamerTag = "console-local-user";
	for ( int u = 0; u < HF_CONSOLE_MAX_USERS; u++ ) {
		for ( int i = 0; i < HF_CONSOLE_MAX_ACHIEVEMENTS; i++ ) {
			achievementLocked[ u ][ i ] = true;
		}
		for ( int s = 0; s < HF_CONSOLE_MAX_STATS; s++ ) {
			statInt[ u ][ s ] = 0;
			statFloat[ u ][ s ] = 0.0f;
		}
		voiceState[ u ] = HF_VOICE_OFF;
		voiceMuted[ u ] = false;
	}
}

HF_INLINE int hfConsoleServicesStub::UserIndex( hfConsoleUserId user ) const {
	if ( user.value < USER_ID_BASE || user.value >= USER_ID_BASE + HF_CONSOLE_MAX_USERS ) {
		return -1;
	}
	return (int)( user.value - USER_ID_BASE );
}

HF_INLINE hfConsoleUserId hfConsoleServicesStub::GetUserId( int index ) const {
	return hfConsoleUserId( ( uint64 )USER_ID_BASE + index );
}

HF_INLINE bool hfConsoleServicesStub::IsAchievementLocked( hfConsoleUserId user, int id ) const {
	int u = UserIndex( user );
	if ( id < 0 || id >= HF_CONSOLE_MAX_ACHIEVEMENTS || u < 0 ) {
		return true;
	}
	return achievementLocked[ u ][ id ];
}

HF_INLINE void hfConsoleServicesStub::UnlockAchievement( hfConsoleUserId user, int id ) {
	int u = UserIndex( user );
	if ( id >= 0 && id < HF_CONSOLE_MAX_ACHIEVEMENTS && u >= 0 ) {
		achievementLocked[ u ][ id ] = false;
	}
}

HF_INLINE void hfConsoleServicesStub::SetStatInt( hfConsoleUserId user, int stat, int value ) {
	int u = UserIndex( user );
	if ( stat >= 0 && stat < HF_CONSOLE_MAX_STATS && u >= 0 ) {
		statInt[ u ][ stat ] = value;
	}
}

HF_INLINE int hfConsoleServicesStub::GetStatInt( hfConsoleUserId user, int stat ) const {
	int u = UserIndex( user );
	if ( stat < 0 || stat >= HF_CONSOLE_MAX_STATS || u < 0 ) {
		return 0;
	}
	return statInt[ u ][ stat ];
}

HF_INLINE void hfConsoleServicesStub::SetStatFloat( hfConsoleUserId user, int stat, float value ) {
	int u = UserIndex( user );
	if ( stat >= 0 && stat < HF_CONSOLE_MAX_STATS && u >= 0 ) {
		statFloat[ u ][ stat ] = value;
	}
}

HF_INLINE float hfConsoleServicesStub::GetStatFloat( hfConsoleUserId user, int stat ) const {
	int u = UserIndex( user );
	if ( stat < 0 || stat >= HF_CONSOLE_MAX_STATS || u < 0 ) {
		return 0.0f;
	}
	return statFloat[ u ][ stat ];
}

HF_INLINE hfConsoleVoiceState hfConsoleServicesStub::GetVoiceState( hfConsoleUserId user ) const {
	int u = UserIndex( user );
	if ( u < 0 ) {
		return HF_VOICE_OFF;
	}
	return voiceState[ u ];
}

HF_INLINE void hfConsoleServicesStub::SetVoiceMuted( hfConsoleUserId user, bool muted ) {
	int u = UserIndex( user );
	if ( u >= 0 ) {
		voiceMuted[ u ] = muted;
	}
}

HF_INLINE bool hfConsoleServicesStub::IsVoiceMuted( hfConsoleUserId user ) const {
	int u = UserIndex( user );
	if ( u < 0 ) {
		return false;
	}
	return voiceMuted[ u ];
}

HF_INLINE int hfConsoleServicesStub::FindSlot( hfConsoleUserId user, const char * slot ) const {
	for ( int i = 0; i < slots.Num(); i++ ) {
		if ( slots[ i ].userId == user && hfStr::Icmp( slots[ i ].name.c_str(), slot ) == 0 ) {
			return i;
		}
	}
	return -1;
}

HF_INLINE bool hfConsoleServicesStub::WriteSlot( hfConsoleUserId user, const char * slot, const byte * data, int dataSize ) {
	if ( slot == NULL || dataSize < 0 || dataSize > HF_CONSOLE_MAX_SLOT_BYTES || UserIndex( user ) < 0 ) {
		return false;
	}
	int index = FindSlot( user, slot );
	if ( index < 0 ) {
		if ( slots.Num() >= HF_CONSOLE_MAX_SLOTS ) {
			return false;
		}
		hfConsoleSlot * newSlot = slots.Alloc();
		if ( newSlot == NULL ) {
			return false;
		}
		newSlot->userId = user;
		newSlot->name = slot;
		newSlot->dataSize = 0;
		index = slots.Num() - 1;
	}
	if ( data != NULL && dataSize > 0 ) {
		memcpy( slots[ index ].data, data, dataSize );
	}
	slots[ index ].dataSize = dataSize;
	return true;
}

HF_INLINE int hfConsoleServicesStub::ReadSlot( hfConsoleUserId user, const char * slot, byte * out, int maxSize ) const {
	int index = FindSlot( user, slot );
	if ( index < 0 ) {
		return 0;
	}
	int count = slots[ index ].dataSize;
	if ( count > maxSize ) {
		count = maxSize;
	}
	if ( out != NULL && count > 0 ) {
		memcpy( out, slots[ index ].data, count );
	}
	return count;
}

HF_INLINE int hfConsoleServicesStub::SlotSize( hfConsoleUserId user, const char * slot ) const {
	int index = FindSlot( user, slot );
	if ( index < 0 ) {
		return 0;
	}
	return slots[ index ].dataSize;
}

HF_INLINE void hfConsoleServicesStub::DeleteSlot( hfConsoleUserId user, const char * slot ) {
	int index = FindSlot( user, slot );
	if ( index >= 0 ) {
		slots.RemoveIndex( index );
	}
}

} // namespace hfconsole

#endif /* !__HF_CONSOLE_SERVICES_H__ */