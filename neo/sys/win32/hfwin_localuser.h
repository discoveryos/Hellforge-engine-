#ifndef __WIN_LOCALUSER_H__
#define __WIN_LOCALUSER_H__

// This is to quickly get/set the data needed for disc-swapping
typedef struct {
	int					inputDevice;
} winUserState_t;

/*
================================================
hfLocalUserWin 
================================================
*/
class hfLocalUserWin : public hfLocalUser {
public:
	static const int MAX_GAMERTAG = 64;			// max number of bytes for a gamertag
	static const int MAX_GAMERTAG_CHARS = 16;	// max number of UTF-8 characters to show

	hfLocalUserWin() : inputDevice( 0 ) {}

	//==========================================================================================
	// hfLocalUser interface
	//==========================================================================================
	virtual bool				IsProfileReady() const;
	virtual bool				IsOnline() const;
	virtual bool				IsInParty() const;
	virtual int					GetPartyCount() const;
	virtual uint32				GetOnlineCaps() const { return ( IsPersistent() && IsOnline() ) ? ( CAP_IS_ONLINE | CAP_CAN_PLAY_ONLINE ) : 0; }
	virtual int					GetInputDevice() const { return inputDevice; }
	virtual const char *		GetGamerTag() const { return gamertag.c_str(); }
	virtual void				PumpPlatform() {}

	//==========================================================================================
	// hfLocalUserWin interface
	//==========================================================================================
	void						SetInputDevice( int inputDevice_ ) { inputDevice = inputDevice_; }
	void						SetGamerTag( const char * gamerTag_ ) { gamertag = gamerTag_; }
	winUserState_t				GetUserState() { winUserState_t a = { inputDevice }; return a; }
	bool						VerifyUserState( winUserState_t & state );

	void						Init( int inputDevice_, const char * gamertag_, int numLocalUsers );

private:
	hfStrStatic< MAX_GAMERTAG >	gamertag;
	int							inputDevice;
};

#endif // __WIN_LOCALUSER_H__

