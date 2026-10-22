/*
** LINUX_GLIMP.C
**
** This file contains ALL Linux specific stuff having to do with the
** OpenGL refresh using GLX.  When a port is being made the following
** functions must be implemented by the port:
**
** GLimp_SwapBuffers
** GLimp_Init
** GLimp_Shutdown
** GLimp_SetGamma
*/

#pragma hdrstop
#include "../../hflib/hfprecompiled.h"

#include <GL/glx.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>

#include "hflinux_local.h"
#include "../../renderer/hftr_local.h"

// GLX_ARB_extensions_string
typedef const char * ( *GLXGETEXTENSIONSSTRINGARBPROC )( Display *, int );
// GLX_SGI_swap_control
typedef int ( *GLXSWAPINTERVALSGIPROC )( int );
// GLX_EXT_swap_control
typedef void ( *GLXSWAPINTERVALEXTPROC )( Display *, GLXDrawable, int );
// GLX_MESA_swap_control
typedef int ( *GLXMESSASWAPINTERVALPROC )( int );
// GLX_ARB_create_context
typedef GLXContext ( *GLXCREATECONTEXTATTRIBSARBPROC )( Display *, GLXFBConfig, GLXContext, Bool, const int * );

static PFNGLXSWAPINTERVALSGIPROC		pfn_glXSwapIntervalSGI = NULL;
static PFNGLXSWAPINTERVALEXTPROC		pfn_glXSwapIntervalEXT = NULL;
static PFNGLXCREATECONTEXTATTRIBSARBPROC	pfn_glXCreateContextAttribsARB = NULL;

// the global X11 state (declared extern in hflinux_local.h)
Display *	x_display = NULL;
Window		x_window = 0;
int			x_screen = 0;

hfCVar r_swapInterval( "r_swapInterval", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "0 = no sync, 1 = sync to refresh, 2 = sync to every other refresh" );

// the global X11 state
static GLXFBConfig			glx_glxfbConfig = NULL;
static unsigned short		oldHardwareGamma[3][256];

/*
====================
GLimp_GetOLDGammaRamp / GLimp_SaveGamma / GLimp_RestoreGamma
====================
*/
static void GLimp_SaveGamma() {
}

static void GLimp_RestoreGamma() {
	if ( oldHardwareGamma[0][255] == 0 ) {
		return;
	}
}

/*
=======================
GLimp_SetGamma

The renderer wants to know if the gamma table has changed
=======================
*/
void GLimp_SetGamma( unsigned short red[256], unsigned short green[256], unsigned short blue[256] ) {
	// no gamma tables on GLX, use the standard sRGB default
}

/*
=======================
GLimp_EnableLogging
=======================
*/
void GLimp_EnableLogging( bool enable ) {
}

/*
=======================
GLimp_ExtensionPointer
=======================
*/
GLExtension_t GLimp_ExtensionPointer( const char *name ) {
	void ( *proc )( ) = NULL;

	proc = (void ( * )( ))glXGetProcAddressARB( (const GLubyte *)name );

	if ( proc == NULL ) {
		common->Printf( "Couldn't find proc address for: %s\n", name );
	}
	return (GLExtension_t)proc;
}

/*
====================
R_GetModeListForDisplay

On Linux we can't easily enumerate monitor modes with XRandR from
a static list, so provide a reasonable set of common resolutions.
====================
*/
bool R_GetModeListForDisplay( const int requestedDisplayNum, hfList<vidMode_t> & modeList ) {
	// only the primary display is supported for now
	if ( requestedDisplayNum != 0 ) {
		modeList.Clear();
		return false;
	}
	modeList.Clear();

	// Common 16:9 / 16:10 desktop resolutions at 60Hz
	struct knownRes_t {
		int width;
		int height;
	};
	static const knownRes_t knownResolutions[] = {
		{ 1280, 720 },
		{ 1280, 800 },
		{ 1366, 768 },
		{ 1600, 900 },
		{ 1920, 1080 },
		{ 2560, 1440 },
		{ 3840, 2160 },
	};

	// add the current desktop resolution first, if it is >= 720p
	int screenW = 0, screenH = 0;
	if ( x_display != NULL ) {
		screenW = DisplayWidth( x_display, DefaultScreen( x_display ) );
		screenH = DisplayHeight( x_display, DefaultScreen( x_display ) );
	}
	if ( screenH >= 720 ) {
		vidMode_t mode;
		mode.width = screenW;
		mode.height = screenH;
		mode.displayHz = 60;
		modeList.AddUnique( mode );
	}

	for ( int i = 0; i < (int)( sizeof( knownResolutions ) / sizeof( knownResolutions[0] ) ); i++ ) {
		if ( knownResolutions[i].height < 720 ) {
			continue;
		}
		if ( knownResolutions[i].width == screenW && knownResolutions[i].height == screenH ) {
			continue;
		}
		vidMode_t mode;
		mode.width = knownResolutions[i].width;
		mode.height = knownResolutions[i].height;
		mode.displayHz = 60;
		modeList.AddUnique( mode );
	}

	return true;
}

/*
=====================
X11 error/io handler to keep the app from dying on X server close
=====================
*/
static int x11_error_handler( Display * dpy, XErrorEvent * ev ) {
	return 0;
}

static int x11_io_error_handler( Display * dpy ) {
	return 0;
}

/*
====================
GLW_GetWindowDimensions
====================
*/
static void GLW_GetWindowDimensions( const glimpParms_t & parms, int &x, int &y, int &w, int &h ) {
	if ( parms.fullScreen != 0 ) {
		// fill the whole desktop
		w = DisplayWidth( x_display, x_screen );
		h = DisplayHeight( x_display, x_screen );
		x = 0;
		y = 0;
	} else {
		w = parms.width;
		h = parms.height;
		x = parms.x;
		y = parms.y;
	}
}

/*
====================
GLW_ChooseVisual
====================
*/
static XVisualInfo * GLW_ChooseVisual( const glimpParms_t & parms ) {
	// try multisample configs first
	GLXFBConfig * fbConfigs = NULL;
	int numConfigs = 0;

	// prefer 4x MSAA, then 2x, then no multisampling
	for ( int tryMode = 0; tryMode < 3; tryMode++ ) {
		GLXFBConfig * cfgs = NULL;
		int n = 0;

		if ( tryMode == 0 ) {
			static int attribs4[] = {
				GLX_X_RENDERABLE, True,
				GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
				GLX_RENDER_TYPE, GLX_RGBA_BIT,
				GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
				GLX_RED_SIZE, 8,
				GLX_GREEN_SIZE, 8,
				GLX_BLUE_SIZE, 8,
				GLX_ALPHA_SIZE, 8,
				GLX_DEPTH_SIZE, 24,
				GLX_STENCIL_SIZE, 8,
				GLX_DOUBLEBUFFER, True,
				GLX_SAMPLE_BUFFERS, 1,
				GLX_SAMPLES, 4,
				None
			};
			cfgs = glXChooseFBConfig( x_display, x_screen, attribs4, &n );
		} else if ( tryMode == 1 ) {
			// same as above but 2 samples
			static int attribs2[] = {
				GLX_X_RENDERABLE, True,
				GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
				GLX_RENDER_TYPE, GLX_RGBA_BIT,
				GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
				GLX_RED_SIZE, 8,
				GLX_GREEN_SIZE, 8,
				GLX_BLUE_SIZE, 8,
				GLX_ALPHA_SIZE, 8,
				GLX_DEPTH_SIZE, 24,
				GLX_STENCIL_SIZE, 8,
				GLX_DOUBLEBUFFER, True,
				GLX_SAMPLE_BUFFERS, 1,
				GLX_SAMPLES, 2,
				None
			};
			cfgs = glXChooseFBConfig( x_display, x_screen, attribs2, &n );
		} else {
			static int attribs0[] = {
				GLX_X_RENDERABLE, True,
				GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
				GLX_RENDER_TYPE, GLX_RGBA_BIT,
				GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
				GLX_RED_SIZE, 8,
				GLX_GREEN_SIZE, 8,
				GLX_BLUE_SIZE, 8,
				GLX_ALPHA_SIZE, 8,
				GLX_DEPTH_SIZE, 24,
				GLX_STENCIL_SIZE, 8,
				GLX_DOUBLEBUFFER, True,
				None
			};
			cfgs = glXChooseFBConfig( x_display, x_screen, attribs0, &n );
		}

		if ( cfgs == NULL || n == 0 ) {
			continue;
		}
		glx_glxfbConfig = cfgs[0];
		XFree( cfgs );

		// if we have a GLXFBConfig, use1.3 functions when available
		XVisualInfo * vi = glXGetVisualFromFBConfig( x_display, glx_glxfbConfig );
		if ( vi != NULL ) {
			linux.desktopBitsPixel = vi->depth;
			linux.desktopWidth = vi->screen;
			// get the actual res
			linux.desktopWidth = DisplayWidth( x_display, x_screen );
			linux.desktopHeight = DisplayHeight( x_display, x_screen );
			return vi;
		}
	}

	// fallback to old-style glXChooseVisual
	int singleBufferAttribs[] = {
		GLX_RGBA,
		GLX_RED_SIZE, 8,
		GLX_GREEN_SIZE, 8,
		GLX_BLUE_SIZE, 8,
		GLX_DEPTH_SIZE, 24,
		GLX_STENCIL_SIZE, 8,
		GLX_DOUBLEBUFFER,
		None
	};
	return glXChooseVisual( x_display, x_screen, singleBufferAttribs );
}

/*
====================
GLW_CreateWindow

Create the X11 window with the selected visual
====================
*/
static bool GLW_CreateWindow( const glimpParms_t parms ) {
	int x, y, w, h;
	GLW_GetWindowDimensions( parms, x, y, w, h );

	XSetWindowAttributes wa;
	memset( &wa, 0, sizeof( wa ) );

	wa.colormap = XCreateColormap( x_display, RootWindow( x_display, x_screen ),
		linux.visual->visual, AllocNone );
	wa.background_pixel = 0;
	wa.border_pixel = 0;
	wa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask
		| ButtonReleaseMask | PointerMotionMask | StructureNotifyMask | FocusChangeMask
		| VisibilityChangeMask;

	unsigned long mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

	linux.wnd = XCreateWindow( x_display, RootWindow( x_display, x_screen ),
		x, y, w, h, 0, linux.visual->depth, InputOutput, linux.visual->visual, mask, &wa );

	if ( linux.wnd == 0 ) {
		common->Printf( "^3GLW_CreateWindow() - Couldn't create X window^0\n" );
		return false;
	}

	XSetStandardProperties( x_display, linux.wnd, GAME_NAME, GAME_NAME, None, NULL, 0, NULL );

	if ( parms.fullScreen != 0 ) {
		// set fullscreen hints
		Atom wm_state = XInternAtom( x_display, "_NET_WM_STATE", False );
		Atom fullscreen = XInternAtom( x_display, "_NET_WM_STATE_FULLSCREEN", False );
		if ( wm_state != None && fullscreen != None ) {
			XChangeProperty( x_display, linux.wnd, wm_state, XA_ATOM, 32, PropModeReplace,
				(unsigned char *)&fullscreen, 1 );
		}
	}

	XMapRaised( x_display, linux.wnd );
	XFlush( x_display );

	// set the WM_PROTOCOLS so we get WM_DELETE_WINDOW
	Atom wm_delete = XInternAtom( x_display, "WM_DELETE_WINDOW", False );
	XSetWMProtocols( x_display, linux.wnd, &wm_delete, 1 );

	common->Printf( "...created window @ %d,%d (%dx%d)\n", x, y, w, h );
	return true;
}

/*
====================
GLW_InitDriver

Create the GLX context and make it current
====================
*/
static bool GLW_InitDriver( const glimpParms_t parms ) {
	GLXContext ctx = NULL;

	// pull up extension entry points before creating the context
	pfn_glXCreateContextAttribsARB =
		(PFNGLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddressARB( (const GLubyte *)"glXCreateContextAttribsARB" );
	pfn_glXSwapIntervalSGI =
		(PFNGLXSWAPINTERVALSGIPROC)glXGetProcAddressARB( (const GLubyte *)"glXSwapIntervalSGI" );
	pfn_glXSwapIntervalEXT =
		(PFNGLXSWAPINTERVALEXTPROC)glXGetProcAddressARB( (const GLubyte *)"glXSwapIntervalEXT" );

	// try an OpenGL 3.2 compatibility context
	const int contextAttribs[] = {
		GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
		GLX_CONTEXT_MINOR_VERSION_ARB, 2,
		GLX_CONTEXT_FLAGS_ARB, 0,
		GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
		None
	};
	if ( glx_glxfbConfig != NULL && pfn_glXCreateContextAttribsARB != NULL ) {
		ctx = pfn_glXCreateContextAttribsARB( x_display, glx_glxfbConfig, NULL, True, contextAttribs );
		if ( ctx == NULL ) {
			// retry without profile mask
			const int contextAttribs2[] = {
				GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
				GLX_CONTEXT_MINOR_VERSION_ARB, 2,
				GLX_CONTEXT_FLAGS_ARB, 0,
				None
			};
			ctx = pfn_glXCreateContextAttribsARB( x_display, glx_glxfbConfig, NULL, True, contextAttribs2 );
		}
	}
	if ( ctx == NULL ) {
		// fallback to a legacy context
		ctx = glXCreateContext( x_display, linux.visual, NULL, True );
	}
	if ( ctx == NULL ) {
		common->Printf( "^3GLW_InitDriver() - glXCreateContext failed^0\n" );
		return false;
	}

	linux.glxContext = ctx;

	if ( !glXMakeCurrent( x_display, linux.wnd, linux.glxContext ) ) {
		common->Printf( "^3GLW_InitDriver() - glXMakeCurrent failed^0\n" );
		glXDestroyContext( x_display, linux.glxContext );
		linux.glxContext = NULL;
		return false;
	}

	// set swap interval
	int interval = 0;
	if ( r_swapInterval.GetInteger() == 1 ) {
		if ( pfn_glXSwapIntervalSGI != NULL ) {
			interval = 1;	// glConfig.swapControlTearAvailable ? -1 : 1
		}
	} else if ( r_swapInterval.GetInteger() == 2 ) {
		interval = 1;
	}
	if ( pfn_glXSwapIntervalSGI != NULL ) {
		pfn_glXSwapIntervalSGI( interval );
	}
	if ( pfn_glXSwapIntervalEXT != NULL ) {
		pfn_glXSwapIntervalEXT( x_display, linux.wnd, interval );
	}

	return true;
}

/*
====================
GLimp_Init
====================
*/
bool GLimp_Init( glimpParms_t parms ) {
	common->Printf( "Initializing OpenGL subsystem with multisamples:%i stereo:%i fullscreen:%i\n",
		parms.multiSamples, parms.stereo, parms.fullScreen );

	// set error handlers
	XSetErrorHandler( x11_error_handler );
	XSetIOErrorHandler( x11_io_error_handler );

	// the display should already be open (from Sys_Init / main)
	if ( x_display == NULL ) {
		common->Printf( "^3GLimp_Init() - no X11 display^0\n" );
		return false;
	}

	// save the hardware gamma so it can be restored on exit
	GLimp_SaveGamma();

	// choose a visual/fbconfig
	linux.visual = GLW_ChooseVisual( parms );
	if ( linux.visual == NULL ) {
		common->Printf( "^3GLimp_Init() - no suitable GLX visual^0\n" );
		return false;
	}

	// create the window
	if ( !GLW_CreateWindow( parms ) ) {
		return false;
	}

	// create the context
	if ( !GLW_InitDriver( parms ) ) {
		return false;
	}

	glConfig.isFullscreen = parms.fullScreen;
	glConfig.isStereoPixelFormat = parms.stereo;
	glConfig.nativeScreenWidth = parms.width;
	glConfig.nativeScreenHeight = parms.height;
	glConfig.multisamples = parms.multiSamples;
	glConfig.pixelAspect = 1.0f;

	int w = DisplayWidth( x_display, x_screen );
	int h = DisplayHeight( x_display, x_screen );
	glConfig.physicalScreenWidthInCentimeters = (float)w;
	if ( w > 0 ) {
		// X11 gives us the mm size via XScreen
		Screen * s = DefaultScreenOfDisplay( x_display );
		if ( s != NULL ) {
			int mm = WidthMMOfScreen( s );
			glConfig.physicalScreenWidthInCentimeters = mm > 0 ? 0.1f * mm : 100.0f;
		}
	}

	GLimp_EnableLogging( ( r_logFile.GetInteger() != 0 ) );

	return true;
}

/*
====================
GLimp_SetScreenParms
====================
*/
bool GLimp_SetScreenParms( glimpParms_t parms ) {
	// resize the window if necessary
	if ( x_display != NULL && linux.wnd != 0 ) {
		int x, y, w, h;
		GLW_GetWindowDimensions( parms, x, y, w, h );
		XResizeWindow( x_display, linux.wnd, w, h );
		XMoveWindow( x_display, linux.wnd, x, y );
		XFlush( x_display );
	}

	glConfig.isFullscreen = parms.fullScreen;
	glConfig.nativeScreenWidth = parms.width;
	glConfig.nativeScreenHeight = parms.height;
	glConfig.pixelAspect = 1.0f;

	return true;
}

/*
====================
GLimp_Shutdown
====================
*/
void GLimp_Shutdown() {
	common->Printf( "Shutting down OpenGL subsystem\n" );

	// restore gamma before going away
	GLimp_RestoreGamma();

	if ( x_display != NULL ) {
		if ( linux.glxContext ) {
			glXMakeCurrent( x_display, None, NULL );
			glXDestroyContext( x_display, linux.glxContext );
			linux.glxContext = NULL;
		}
		if ( linux.wnd != 0 ) {
			XDestroyWindow( x_display, linux.wnd );
			linux.wnd = 0;
		}
		if ( linux.visual != NULL ) {
			XFree( linux.visual );
			linux.visual = NULL;
		}
		glx_glxfbConfig = NULL;
	}
}

/*
====================
GLimp_SwapBuffers
====================
*/
void GLimp_SwapBuffers() {
	// set swap interval if changed by cvar
	if ( r_swapInterval.IsModified() ) {
		r_swapInterval.ClearModified();
		int interval = 0;
		if ( r_swapInterval.GetInteger() == 1 ) {
			interval = 1;
		} else if ( r_swapInterval.GetInteger() == 2 ) {
			interval = 1;
		}
		if ( pfn_glXSwapIntervalSGI != NULL ) {
			pfn_glXSwapIntervalSGI( interval );
		}
		if ( pfn_glXSwapIntervalEXT != NULL ) {
			pfn_glXSwapIntervalEXT( x_display, linux.wnd, interval );
		}
	}

	glXSwapBuffers( x_display, linux.wnd );
}

/*
====================
SMP acceleration
====================
*/
static void ( * glimpRenderThread )() = NULL;
static bool renderThreadActive = false;

static signalHandle_t	renderCommandsEvent;
static signalHandle_t	renderCompletedEvent;
static signalHandle_t	renderActiveEvent;

static uintptr_t		renderThreadHandle;
static int				smpData;

/*
====================
GLimp_RenderThreadWrapper
====================
*/
static void * GLimp_RenderThreadWrapper( void * ) {
	Sys_SetCurrentThreadName( "renderer" );
	if ( glimpRenderThread != NULL ) {
		glimpRenderThread();
	}
	// unbind the context before we die
	glXMakeCurrent( x_display, None, NULL );
	return NULL;
}

/*
====================
GLimp_SpawnRenderThread

Returns false if the system only has a single processor
====================
*/
bool GLimp_SpawnRenderThread( void (*function)() ) {
	long nproc = sysconf( _SC_NPROCESSORS_ONLN );
	if ( nproc < 2 ) {
		return false;
	}

	// create the IPC elements
	Sys_SignalCreate( renderCommandsEvent, false );
	Sys_SignalCreate( renderCompletedEvent, false );
	Sys_SignalCreate( renderActiveEvent, false );
	Sys_SignalClear( renderCommandsEvent );
	Sys_SignalClear( renderCompletedEvent );
	Sys_SignalClear( renderActiveEvent );

	glimpRenderThread = function;

	Sys_CreateThread( (xthread_t)GLimp_RenderThreadWrapper, NULL, THREAD_NORMAL, "renderer", CORE_ANY, 0 );

	if ( renderThreadHandle == 0 ) {
		common->Error( "GLimp_SpawnRenderThread: failed" );
	}

	renderThreadActive = false;
	return true;
}

/*
====================
GLimp_BackEndSleep
====================
*/
void *GLimp_BackEndSleep() {
	renderThreadActive = false;

	// after this, the front end can exit GLimp_FrontEndSleep
	Sys_SignalRaise( renderCompletedEvent );

	Sys_SignalWait( renderCommandsEvent, -1 );

	void * data = (void *)(intptr_t)smpData;

	// after this, the main thread can exit GLimp_WakeBackEnd
	renderThreadActive = true;
	Sys_SignalRaise( renderActiveEvent );

	return data;
}

/*
====================
GLimp_FrontEndSleep
====================
*/
void GLimp_FrontEndSleep() {
	Sys_SignalWait( renderCompletedEvent, -1 );
}

/*
====================
GLimp_WakeBackEnd
====================
*/
void GLimp_WakeBackEnd( void *data ) {
	smpData = (int)(intptr_t)data;

	if ( renderThreadActive ) {
		common->FatalError( "GLimp_WakeBackEnd: already active" );
	}

	Sys_SignalRaise( renderCommandsEvent );

	if ( !Sys_SignalWait( renderActiveEvent, 5000 ) ) {
		common->FatalError( "GLimp_WakeBackEnd: WAIT_TIMEOUT" );
	}
}

/*
====================
GLimp_ActivateContext
====================
*/
void GLimp_ActivateContext() {
	if ( !glXMakeCurrent( x_display, linux.wnd, linux.glxContext ) ) {
		linux.glErrors++;
	}
}

/*
====================
GLimp_DeactivateContext
====================
*/
void GLimp_DeactivateContext() {
	glXMakeCurrent( x_display, None, NULL );
}

/*
====================
Sys_ShowWindow / Sys_IsWindowVisible
====================
*/
void Sys_ShowWindow( bool show ) {
	if ( x_display == NULL || linux.wnd == 0 ) {
		return;
	}
	if ( show ) {
		XMapRaised( x_display, linux.wnd );
	} else {
		XUnmapWindow( x_display, linux.wnd );
	}
	XFlush( x_display );
}

bool Sys_IsWindowVisible() {
	return linux.activeApp;
}

/*
====================
GetScreenSeparationForGuis
====================
*/
float GetScreenSeparationForGuis() {
	return 0.0f;
}

/*
====================
DumpAllDisplayDevices
====================
*/
void DumpAllDisplayDevices() {
	Display * display = XOpenDisplay( NULL );
	if ( display == NULL ) {
		common->Printf( "display device: unable to open X display\n" );
		return;
	}

	const int numScreens = XScreenCount( display );
	common->Printf( "Dumping display devices\n" );
	for ( int screenNum = 0 ; screenNum < numScreens ; screenNum++ ) {
		Screen * screen = XScreenOfDisplay( display, screenNum );
		common->Printf( "display device: %i ( %ix%i, %i bits )\n", screenNum,
			XWidthOfScreen( screen ), XHeightOfScreen( screen ), XDefaultDepth( display, screenNum ) );
	}

	XCloseDisplay( display );
}
