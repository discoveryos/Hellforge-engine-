#ifndef __CINEMATIC_H__
#define __CINEMATIC_H__

/*
===============================================================================

	cinematic

	Multiple hfCinematics can run simultaniously.
	A single hfCinematic can be reused for multiple files if desired.

===============================================================================
*/

// cinematic states
typedef enum {
	FMV_IDLE,
	FMV_PLAY,			// play
	FMV_EOF,			// all other conditions, i.e. stop/EOF/abort
	FMV_ID_BLT,
	FMV_ID_IDLE,
	FMV_LOOPED,
	FMV_ID_WAIT
} cinStatus_t;

class hfImage;

// a cinematic stream generates an image buffer, which the caller will upload to a texture
typedef struct {
	int					imageWidth;
	int					imageHeight;	// will be a power of 2
	hfImage*			imageY;
	hfImage*			imageCr;
	hfImage*			imageCb;
	int					status;
} cinData_t;

class hfCinematic {
public:
	// initialize cinematic play back data
	static void			InitCinematic( void );

	// shutdown cinematic play back data
	static void			ShutdownCinematic( void );

	// allocates and returns a private subclass that implements the methods
	// This should be used instead of new
	static hfCinematic	*Alloc();

	// frees all allocated memory
	virtual				~hfCinematic();

	// returns false if it failed to load
	virtual bool		InitFromFile( const char *qpath, bool looping );

	// returns the length of the animation in milliseconds
	virtual int			AnimationLength();

	// the pointers in cinData_t will remain valid until the next UpdateForTime() call
	virtual cinData_t	ImageForTime( int milliseconds );

	// closes the file and frees all allocated memory
	virtual void		Close();

	// sets the cinematic to start at that time (can be in the past)
	virtual void		ResetTime(int time);

	// gets the time the cinematic started
	virtual int			GetStartTime();

	virtual void		ExportToTGA( bool skipExisting = true );

	virtual float		GetFrameRate() const;
};

/*
===============================================

	Sound meter.

===============================================
*/

class hfSndWindow : public hfCinematic {
public:
	
						hfSndWindow() { showWaveform = false; }
						~hfSndWindow() {}

	bool				InitFromFile( const char *qpath, bool looping );
	cinData_t			ImageForTime( int milliseconds );
	int					AnimationLength();

private:
	bool				showWaveform;
};

#endif /* !__CINEMATIC_H__ */

