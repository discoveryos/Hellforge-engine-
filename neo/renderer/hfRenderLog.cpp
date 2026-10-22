#pragma hdrstop
#include "hftr_local.h"

/*
================================================================================================
Contains the RenderLog implementation.

TODO:	Emit statistics to the logfile at the end of views and frames.
================================================================================================
*/

hfCVar r_logLevel( "r_logLevel", "2", CVAR_INTEGER, "1 = blocks only, 2 = everything", 1, 2 );

static const int LOG_LEVEL_BLOCKS_ONLY	= 1;
static const int LOG_LEVEL_EVERYTHING	= 2;

const char * renderLogMainBlockLabels[] = {
	ASSERT_ENUM_STRING( MRB_NONE,							0 ),
	ASSERT_ENUM_STRING( MRB_BEGIN_DRAWING_VIEW,				1 ),
	ASSERT_ENUM_STRING( MRB_FILL_DEPTH_BUFFER,				2 ),
	ASSERT_ENUM_STRING( MRB_DRAW_INTERACTIONS,				3 ),
	ASSERT_ENUM_STRING( MRB_DRAW_SHADER_PASSES,				4 ),
	ASSERT_ENUM_STRING( MRB_FOG_ALL_LIGHTS,					5 ),
	ASSERT_ENUM_STRING( MRB_DRAW_SHADER_PASSES_POST,		6 ),
	ASSERT_ENUM_STRING( MRB_DRAW_DEBUG_TOOLS,				7 ),
	ASSERT_ENUM_STRING( MRB_CAPTURE_COLORBUFFER,			8 ),
	ASSERT_ENUM_STRING( MRB_POSTPROCESS,					9 ),
	ASSERT_ENUM_STRING( MRB_GPU_SYNC,						10 ),
	ASSERT_ENUM_STRING( MRB_END_FRAME,						11 ),
	ASSERT_ENUM_STRING( MRB_BINK_FRAME,						12 ),
	ASSERT_ENUM_STRING( MRB_BINK_NEXT_FRAME,				13 ),
	ASSERT_ENUM_STRING( MRB_TOTAL,							14 ),
	ASSERT_ENUM_STRING( MRB_MAX,							15 )
};

extern uint64 Sys_Microseconds();
/*
================================================================================================

PIX events on all platforms

================================================================================================
*/


/*
================================================
pixEvent_t 
================================================
*/
struct pixEvent_t {
	char		name[256];
	uint64		cpuTime;
	uint64		gpuTime;
};

hfCVar r_pix( "r_pix", "0", CVAR_INTEGER, "print GPU/CPU event timing" );

static const int	MAX_PIX_EVENTS = 256;
// defer allocation of this until needed, so we don't waste lots of memory
pixEvent_t *		pixEvents;	// [MAX_PIX_EVENTS]
int					numPixEvents;
int					numPixLevels;
static GLuint		timeQueryIds[MAX_PIX_EVENTS];

/*
========================
PC_BeginNamedEvent

FIXME: this is not thread safe on the PC
========================
*/
void PC_BeginNamedEvent( const char *szName, ... ) {
#if 0
	if ( !r_pix.GetBool() ) {
		return;
	}
	if ( !pixEvents ) {
		// lazy allocation to not waste memory
		pixEvents = (pixEvent_t *)Mem_ClearedAlloc( sizeof( *pixEvents ) * MAX_PIX_EVENTS, TAG_CRAP );
	}
	if ( numPixEvents >= MAX_PIX_EVENTS ) {
		hfLib::FatalError( "PC_BeginNamedEvent: event overflow" );
	}
	if ( ++numPixLevels > 1 ) {
		return;	// only get top level timing information
	}
	if ( !qglGetQueryObjectui64vEXT ) {
		return;
	}

	GL_CheckErrors();
	if ( timeQueryIds[0] == 0 ) {
		qglGenQueriesARB( MAX_PIX_EVENTS, timeQueryIds );
	}
	qglFinish();
	qglBeginQueryARB( GL_TIME_ELAPSED_EXT, timeQueryIds[numPixEvents] );
	GL_CheckErrors();

	pixEvent_t *ev = &pixEvents[numPixEvents++];
	strncpy( ev->name, szName, sizeof( ev->name ) - 1 );
	ev->cpuTime = Sys_Microseconds();
#endif
}

/*
========================
PC_EndNamedEvent
========================
*/
void PC_EndNamedEvent() {
#if 0
	if ( !r_pix.GetBool() ) {
		return;
	}
	if ( numPixLevels <= 0 ) {
		hfLib::FatalError( "PC_EndNamedEvent: level underflow" );
	}
	if ( --numPixLevels > 0 ) {
		// only do timing on top level events
		return;
	}
	if ( !qglGetQueryObjectui64vEXT ) {
		return;
	}

	pixEvent_t *ev = &pixEvents[numPixEvents-1];
	ev->cpuTime = Sys_Microseconds() - ev->cpuTime;

	GL_CheckErrors();
	qglEndQueryARB( GL_TIME_ELAPSED_EXT );
	GL_CheckErrors();
#endif
}

/*
========================
PC_EndFrame
========================
*/
void PC_EndFrame() {
#if 0
	if ( !r_pix.GetBool() ) {
		return;
	}

	int64 totalGPU = 0;
	int64 totalCPU = 0;

	hfLib::Printf( "----- GPU Events -----\n" );
	for ( int i = 0 ; i < numPixEvents ; i++ ) {
		pixEvent_t *ev = &pixEvents[i];

		int64 gpuTime = 0;
		qglGetQueryObjectui64vEXT( timeQueryIds[i], GL_QUERY_RESULT, (GLuint64EXT *)&gpuTime );
		ev->gpuTime = gpuTime;

		hfLib::Printf( "%2d: %1.2f (GPU) %1.3f (CPU) = %s\n", i, ev->gpuTime / 1000000.0f, ev->cpuTime / 1000.0f, ev->name );
		totalGPU += ev->gpuTime;
		totalCPU += ev->cpuTime;
	}
	hfLib::Printf( "%2d: %1.2f (GPU) %1.3f (CPU) = total\n", numPixEvents, totalGPU / 1000000.0f, totalCPU / 1000.0f );
	memset( pixEvents, 0, numPixLevels * sizeof( pixEvents[0] ) );
	
	numPixEvents = 0;
	numPixLevels = 0;
#endif
}


/*
================================================================================================

hfRenderLog

================================================================================================
*/

hfRenderLog	renderLog;

#if !defined( STUB_RENDER_LOG )

/*
========================
hfRenderLog::hfRenderLog
========================
*/
hfRenderLog::hfRenderLog() {
	activeLevel = 0;
	indentString[0] = '\0';
	indentLevel = 0;
	logFile = NULL;

	frameStartTime = 0;
	closeBlockTime = 0;
	logLevel = 0;
}

/*
========================
hfRenderLog::StartFrame
========================
*/
void hfRenderLog::StartFrame() {
	if ( r_logFile.GetInteger() == 0 ) {
		return;
	}

	// open a new logfile
	indentLevel = 0;
	indentString[0] = '\0';
	activeLevel = r_logLevel.GetInteger();

	struct tm		*newtime;
	time_t			aclock;

	char ospath[ MAX_OSPATH ];

	char qpath[128];
	sprintf( qpath, "renderlogPC_%04i.txt", r_logFile.GetInteger() );
	hfStr finalPath = fileSystem->RelativePathToOSPath( qpath );		
	sprintf( ospath, "%s", finalPath.c_str() );
	/*
	for ( int i = 0; i < 9999 ; i++ ) {
		char qpath[128];
		sprintf( qpath, "renderlog_%04i.txt", r_logFile.GetInteger() );
		hfStr finalPath = fileSystem->RelativePathToOSPath( qpath );
		fileSystem->RelativePathToOSPath( qpath, ospath, MAX_OSPATH ,FSPATH_BASE );
		if ( !fileSystem->FileExists( finalPath.c_str() ) ) {
			break; // use this name
		}
	}
	*/

	common->SetRefreshOnPrint( false );	// problems are caused if this print causes a refresh...

	if ( logFile != NULL ) {
		fileSystem->CloseFile( logFile );
		logFile = NULL;
	}

	logFile = fileSystem->OpenFileWrite( ospath );	
	if ( logFile == NULL ) {
		hfLib::Warning( "Failed to open logfile %s", ospath );
		return;
	}
	hfLib::Printf( "Opened logfile %s\n", ospath );

	// write the time out to the top of the file
	time( &aclock );
	newtime = localtime( &aclock );
	const char *str = asctime( newtime );
	logFile->Printf( "// %s", str );
	logFile->Printf( "// %s\n\n", com_version.GetString() );

	frameStartTime = Sys_Microseconds();
	closeBlockTime = frameStartTime;
	OpenBlock( "Frame" );
}

/*
========================
hfRenderLog::EndFrame
========================
*/
void hfRenderLog::EndFrame() {
	PC_EndFrame();

	if ( logFile != NULL ) {
		if ( r_logFile.GetInteger() == 1 ) {
			Close();
		}
		// log is open, so decrement r_logFile and stop if it is zero
		r_logFile.SetInteger( r_logFile.GetInteger() - 1 );
		hfLib::Printf( "Frame logged.\n" );
		return;
	}
}

/*
========================
hfRenderLog::Close
========================
*/
void hfRenderLog::Close() {
	if ( logFile != NULL ) {
		CloseBlock();
		hfLib::Printf( "Closing logfile\n" );
		fileSystem->CloseFile( logFile );
		logFile = NULL;
		activeLevel = 0;
	}
}

/*
========================
hfRenderLog::OpenMainBlock
========================
*/
void hfRenderLog::OpenMainBlock( renderLogMainBlock_t block ) {
}

/*
========================
hfRenderLog::CloseMainBlock
========================
*/
void hfRenderLog::CloseMainBlock() {
}

/*
========================
hfRenderLog::OpenBlock
========================
*/
void hfRenderLog::OpenBlock( const char *label ) {
	// Allow the PIX functionality even when logFile is not running.
	PC_BeginNamedEvent( label );

	if ( logFile != NULL ) {
		va_list args;
		memset( &args, 0, sizeof( args ) );
		LogOpenBlock( RENDER_LOG_INDENT_MAIN_BLOCK, label, args );
	}
}

/*
========================
hfRenderLog::CloseBlock
========================
*/
void hfRenderLog::CloseBlock() {
	PC_EndNamedEvent();

	if ( logFile != NULL ) {
		LogCloseBlock( RENDER_LOG_INDENT_MAIN_BLOCK );
	}
}

/*
========================
hfRenderLog::Printf
========================
*/
void hfRenderLog::Printf( const char *fmt, ... ) {
	if ( activeLevel <= LOG_LEVEL_BLOCKS_ONLY ) {
		return;
	}

	if ( logFile == NULL ) {
		return;
	}

	va_list marker;
	logFile->Printf( "%s", indentString );
	va_start( marker, fmt );
	logFile->VPrintf( fmt, marker );
	va_end( marker );
//	logFile->Flush();		this makes it take waaaay too long
}

/*
========================
hfRenderLog::LogOpenBlock
========================
*/
void hfRenderLog::LogOpenBlock( renderLogIndentLabel_t label, const char * fmt, va_list args ) {

	uint64 now = Sys_Microseconds();

	if ( logFile != NULL ) {
		if ( now - closeBlockTime >= 1000 ) {
			logFile->Printf( "%s%1.1f msec gap from last closeblock\n", indentString, ( now - closeBlockTime ) * ( 1.0f / 1000.0f ) );
		}
		logFile->Printf( "%s", indentString );
		logFile->VPrintf( fmt, args );
		logFile->Printf( " {\n" );
	}

	Indent( label );

	if ( logLevel >= MAX_LOG_LEVELS ) {
		hfLib::Warning( "logLevel %d >= MAX_LOG_LEVELS", logLevel );
	}


	logLevel++;
}

/*
========================
hfRenderLog::LogCloseBlock
========================
*/
void hfRenderLog::LogCloseBlock( renderLogIndentLabel_t label ) {
	closeBlockTime = Sys_Microseconds();

	assert( logLevel > 0 );
	logLevel--;

	Outdent( label );

	if ( logFile != NULL ) {
	}
}

#else	// !STUB_RENDER_LOG

/*
========================
hfRenderLog::OpenBlock
========================
*/
void hfRenderLog::OpenBlock( const char *label ) {
	PC_BeginNamedEvent( label );
}

/*
========================
hfRenderLog::CloseBlock
========================
*/
void hfRenderLog::CloseBlock() {
	PC_EndNamedEvent();
}

#endif // !STUB_RENDER_LOG

