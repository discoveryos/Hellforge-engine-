#ifndef __SYS_LOCALUSER_H__
#define __SYS_LOCALUSER_H__

#include "hfsys_profile.h"

struct achievementDescription_t;
class hfPlayerProfile;
class hfProfileMgr;

enum onlineCaps_t {
	CAP_IS_ONLINE			= BIT( 0 ),
	CAP_BLOCKED_PERMISSION	= BIT( 1 ),
	CAP_CAN_PLAY_ONLINE		= BIT( 2 ),
};

class hfSerializer;

/*
================================================
localUserHandle_t
================================================
*/
struct localUserHandle_t {
public:
	typedef uint32 userHandleType_t;

	localUserHandle_t() : handle( 0 ) {}

	explicit localUserHandle_t( userHandleType_t handle_ ) : handle( handle_ ) {}

	bool operator == ( const localUserHandle_t & other ) const {
		return handle == other.handle;
	}

	bool operator < ( const localUserHandle_t & other ) const {
		return handle < other.handle;
	}

	bool IsValid() const { return handle > 0; }

	void WriteToMsg( hfBitMsg & msg ) {
		msg.WriteLong( handle );
	}

	void ReadFromMsg( const hfBitMsg & msg ) {
		handle = msg.ReadLong();
	}

	void Serialize( hfSerializer & ser );

private:
	userHandleType_t	handle;
};

/*
================================================
hfLocalUser 
An hfLocalUser is a user holding a controller.
It represents someone controlling the menu or game.
They may not necessarily be in a game (which would be a session user of TYPE_GAME).
A controller user references an input device (which is a gamepad, keyboard, etc).
================================================
*/
class hfLocalUser { 
public:	
								hfLocalUser();
	virtual						~hfLocalUser() {}

			void				Pump();
	virtual void				PumpPlatform() = 0;

	virtual bool				IsPersistent() const { return IsProfileReady(); }	// True if this user is a persistent user, and can save stats, etc (signed in)
	virtual bool				IsProfileReady() const = 0;							// True if IsPersistent is true AND profile is signed into LIVE service
	virtual bool				IsOnline() const = 0;								// True if this user has online capabilities
	virtual uint32				GetOnlineCaps() const = 0;							// Returns combination of onlineCaps_t flags
	virtual bool				HasOwnerChanged() const { return false; }			// Whether or not the original persistent owner has changed since it was first registered
	virtual int					GetInputDevice() const = 0;							// Input device of controller
	virtual const char *		GetGamerTag() const = 0;							// Gamertag of user
	virtual bool				IsInParty() const = 0;								// True if the user is in a party (do we support this on pc and ps3? )
	virtual int					GetPartyCount() const = 0;							// Gets the amount of users in the party

	// Storage related
	virtual bool				IsStorageDeviceAvailable() const;					// Only false if the player has chosen to play without a storage device, only possible on 360, if available, everything needs to check for available space
	virtual void				ResetStorageDevice();
	virtual bool				StorageSizeAvailable( uint64 minSizeInBytes, int64 & neededBytes );

	// These set stats within the profile as a enum/value pair
	virtual void				SetStatInt( int stat, int value );
	virtual void				SetStatFloat( int stat, float value );
	virtual int					GetStatInt( int stat );
	virtual float				GetStatFloat( int stat);

	virtual hfPlayerProfile *	GetProfile() { return GetProfileMgr().GetProfile(); }
	const hfPlayerProfile *		GetProfile() const { return const_cast< hfLocalUser * >( this )->GetProfile(); }

	hfProfileMgr &				GetProfileMgr() { return profileMgr; }

	// Helper state to determine if the user is joining a party lobby or not
	void						SetJoiningLobby( int lobbyType, bool value ) { joiningLobby[lobbyType] = value; }
	bool						IsJoiningLobby( int lobbyType ) const { return joiningLobby[lobbyType]; }

	bool						CanPlayOnline() const { return ( GetOnlineCaps() & CAP_CAN_PLAY_ONLINE ) > 0; }

	localUserHandle_t			GetLocalUserHandle() const { return localUserHandle; }
	void						SetLocalUserHandle( localUserHandle_t newHandle ) { localUserHandle = newHandle; }

	// Creates a new profile if one not already there
	void						LoadProfileSettings();
	void						SaveProfileSettings();

	// Will attempt to sync the achievement bits between the server and the localUser when the achievement system is ready
	void						RequestSyncAchievements() { syncAchievementsRequested = true; }

private:
	bool						joiningLobby[2];
	localUserHandle_t			localUserHandle;
	hfProfileMgr				profileMgr;

	bool						syncAchievementsRequested;
};

#endif // __SYS_LOCALUSER_H__

