#pragma hdrstop
#include "../../hflib/hfprecompiled.h"

//#if defined( HF_VS2010 )
//#include "../../../libs/dxsdk_June2010/include/xinput.h"
//#else
#include <Xinput.h>
//#endif

static const int MAX_JOYSTICKS = 4;

/*
================================================================================================

	Joystick Win32

================================================================================================
*/

struct controllerState_t {
	// the current states are updated by the input thread at 250 hz
	XINPUT_STATE	current;

	// the previous state is latched at polling time
	XINPUT_STATE	previous;

	// The current button bits are or'd into this at the high sampling rate, then
	// zero'd by the main thread when a usercmd_t is created.  This prevents the
	// complete missing of a button press that went down and up between two usercmd_t
	// creations, although it can add sn extra frame of latency to sensing a release.
	int				buttonBits;

	// Only valid controllers will have their rumble set
	bool			valid;
};


class hfJoystickWin32 : hfJoystick {
public:
					hfJoystickWin32();

	virtual bool	Init();
	virtual void	SetRumble( int deviceNum, int rumbleLow, int rumbleHigh );
	virtual int		PollInputEvents( int inputDeviceNum );
	virtual int		ReturnInputEvent( const int n, int &action, int &value );
	virtual void	EndInputEvents() {}

protected:
	friend void		JoystickSamplingThread( void *data );

	void 			PushButton( int inputDeviceNum, int key, bool value );
	void 			PostInputEvent( int inputDeviceNum, int event, int value, int range = 16384 );

	hfSysMutex				mutexXis;		// lock this before using currentXis or stickIntegrations
	HANDLE					timer;			// fire every 4 msec

	int						numEvents;

	struct {
		int event;
		int value;
	}						events[ MAX_JOY_EVENT ];

	controllerState_t		controllers[ MAX_JOYSTICKS ];

	// should these be per-controller?
	bool					buttonStates[MAX_INPUT_DEVICES][K_LAST_KEY];	// For keeping track of button up/down events
	int						joyAxis[MAX_INPUT_DEVICES][MAX_JOYSTICK_AXIS];			// For keeping track of joystick axises
};
