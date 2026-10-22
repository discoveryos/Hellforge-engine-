#ifndef __RESOLUTIONSCALE_H__
#define __RESOLUTIONSCALE_H__

class hfResolutionScale {
public:
			hfResolutionScale();

	void	InitForMap( const char * mapName );

	// Returns a float from 0.5 to 1.0, representing
	// the estimated resolution downscale needed to
	// maintain the target framerate.
	void	GetCurrentResolutionScale( float &x, float &y );

	// This should be called after any discontinuous
	// view movement or force texture loading to prevent
	// the unusual frames from causing an excessively
	// low dynamic resolution.
	void	ResetToFullResolution();

	// Systems that don't have accurate GPU timing can pass 0
	// to this, which will effectively disable resolution scaling.
	void	SetCurrentGPUFrameTime( int microseconds );

	// return console display text
	void	GetConsoleText( hfStr &s );

private:
	float			dropMilliseconds;
	float			raiseMilliseconds;
	int				framesAboveRaise;
	float			currentResolution;
};

extern hfResolutionScale resolutionScale;

#endif // __RESOLUTIONSCALE_H__

