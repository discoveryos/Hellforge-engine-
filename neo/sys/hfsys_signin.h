#ifndef __SYS_SIGNIN_H__
#define __SYS_SIGNIN_H__

/*
================================================
hfSignInManagerBase 
================================================
*/
class hfSignInManagerBase {
public:

	hfSignInManagerBase() :
		  minDesiredLocalUsers( 0 ),
		  maxDesiredLocalUsers( 0 ),
		  defaultProfile( NULL ) {}
	  virtual							~hfSignInManagerBase() {}

	  virtual void					Pump() = 0;
	  virtual int					GetNumLocalUsers() const = 0;
	  virtual hfLocalUser *			GetLocalUserByIndex( int index ) = 0;
	  virtual const hfLocalUser *	GetLocalUserByIndex( int index ) const = 0;
	  virtual void					RemoveLocalUserByIndex( int index ) = 0;
	  virtual void					RegisterLocalUser( int inputDevice ) = 0;								// Register a local controller user to the passed in input device
	  virtual hfLocalUser *			GetRegisteringUser() { return NULL; }									// This is a user that has started the registration process but is not yet a local user.
	  virtual hfLocalUser *			GetRegisteringUserByInputDevice( int inputDevice ) { return NULL; }
	  virtual void					SignIn() {}
	  virtual bool					IsDeviceBeingRegistered( int intputDevice ) { return false; }
	  virtual bool					IsAnyDeviceBeingRegistered() { return false; }
	  virtual void					Shutdown() {}

	  // Outputs all the local users and other debugging information from the sign in manager
	  virtual void					DebugOutputLocalUserInfo() {}

	  //================================================================================
	  // Common helper functions
	  //================================================================================

	  void 					SetDesiredLocalUsers( int minDesiredLocalUsers, int maxDesiredLocalUsers ) { this->minDesiredLocalUsers = minDesiredLocalUsers; this->maxDesiredLocalUsers = maxDesiredLocalUsers; }
	  bool 					ProcessInputEvent( const sysEvent_t * ev );
	  hfPlayerProfile *		GetDefaultProfile();

	  // Master user always index 0
	  hfLocalUser *			GetMasterLocalUser() { return ( GetNumLocalUsers() > 0 ) ? GetLocalUserByIndex( 0 ) : NULL; }
	  const hfLocalUser *	GetMasterLocalUser() const { return ( GetNumLocalUsers() > 0 ) ? GetLocalUserByIndex( 0 ) : NULL; }

	  bool 					IsMasterLocalUserPersistent() const { return ( GetMasterLocalUser() != NULL ) ? GetMasterLocalUser()->IsPersistent() : false; }
	  bool 					IsMasterLocalUserOnline() const { return ( GetMasterLocalUser() != NULL ) ? GetMasterLocalUser()->IsOnline() : false; }
	  int					GetMasterInputDevice() const { return ( GetMasterLocalUser() != NULL ) ? GetMasterLocalUser()->GetInputDevice() : -1; }
	  localUserHandle_t		GetMasterLocalUserHandle() const { return ( GetMasterLocalUser() != NULL ) ? GetMasterLocalUser()->GetLocalUserHandle() : localUserHandle_t(); }
	  hfLocalUser *			GetLocalUserByInputDevice( int index );
	  hfLocalUser *			GetLocalUserByHandle( localUserHandle_t handle );
	  hfPlayerProfile *		GetPlayerProfileByInputDevice( int index );
	  bool					RemoveLocalUserByInputDevice( int index );
	  bool					RemoveLocalUserByHandle( localUserHandle_t handle );
	  void					RemoveAllLocalUsers();
	  void					SaveUserProfiles();

	  // This will remove local players that are not signed into a profile.
	  // If requiredOnline: This removes the users who cannot play online
	  void					ValidateLocalUsers( bool requireOnline );

	  bool					RequirePersistentMaster();

	  localUserHandle_t		GetUniqueLocalUserHandle( const char * name );

protected:
	int					minDesiredLocalUsers;
	int					maxDesiredLocalUsers;
	hfPlayerProfile *	defaultProfile;
};

#endif	// __SYS_SIGNIN_H__

