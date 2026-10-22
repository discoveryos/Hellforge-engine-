#ifndef __LINUX_LOCAL_H__
#define __LINUX_LOCAL_H__

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#include "../../renderer/OpenGL/hfglext.h"

// X11 / GLX globals
extern Display *	x_display;
extern Window		x_window;
extern int			x_screen;

struct LinuxVars_t {
	Window			wnd;
	Display *		dpy;
	int				screen;
	GC				gc;

	bool			activeApp;
	bool			mouseReleased;
	bool			movingWindow;
	bool			mouseGrabbed;

	cpuid_t			cpuid;

	int				sysMsgTime;

	// OpenGL
	GLXContext		glxContext;
	XVisualInfo *	visual;
	bool			hasGLX;

	int				desktopWidth, desktopHeight;
	int				desktopBitsPixel;
	int				cdsFullscreen;

	hfFileHandle	log_fp;

	mutexHandle_t	criticalSections[MAX_CRITICAL_SECTIONS];

	static hfCVar	sys_arch;
	static hfCVar	sys_cpustring;
	static hfCVar	in_mouse;
	static hfCVar	linux_allowAltTab;
	static hfCVar	linux_username;
	static hfCVar	linux_outputEditString;

	int				glErrors;
	int				smpData;

};

// The glibc headers define 'linux' as '1'; undef it so the global can be named that.
#undef linux
extern LinuxVars_t	linux;

void	Sys_QueEvent( sysEventType_t type, int value, int value2, int ptrLength, void *ptr, int inputDeviceNum );

char	*Sys_ConsoleInput();
char	*Sys_GetCurrentUser();

cpuid_t	Sys_GetCPUId();

unsigned long Sys_XKeyToKeyNum( KeySym key );

void	IN_Init();
void	IN_Shutdown();
void	IN_Frame();

uint64 Syst_Microseconds();

#endif /* !__LINUX_LOCAL_H__ */

