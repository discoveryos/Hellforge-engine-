/*
** WIN_GAMMA.C
*/
#include <assert.h>
#include "hfwin_local.h"
#include "../../renderer/hftr_local.h"

static unsigned short s_oldHardwareGamma[3][256];

/*
** WG_GetOldGammaRamp
**
*/
void WG_GetOldGammaRamp( void )
{
	HDC			hDC;

	hDC = GetDC( GetDesktopWindow() );
	GetDeviceGammaRamp( hDC, s_oldHardwareGamma );
	ReleaseDC( GetDesktopWindow(), hDC );


/*
** GLimp_SetGamma
**
*/
void GLimp_SetGamma( unsigned char red[256], unsigned char green[256], unsigned char blue[256] )
{
	unsigned short table[3][256];
	int i;

	if ( !glw_state.hDC )
	{
		return;
	}

	for ( i = 0; i < 256; i++ )
	{
		table[0][i] = ( ( ( unsigned short ) red[i] ) << 8 ) | red[i];
		table[1][i] = ( ( ( unsigned short ) green[i] ) << 8 ) | green[i];
		table[2][i] = ( ( ( unsigned short ) blue[i] ) << 8 ) | blue[i];
	}

	if ( !SetDeviceGammaRamp( glw_state.hDC, table ) ) {
		common->Printf( "WARNING: SetDeviceGammaRamp failed.\n" );
	}
}

/*
** WG_RestoreGamma
*/
void WG_RestoreGamma( void )
{
	HDC hDC;

	// if we never read in a reasonable looking
	// table, don't write it out
	if ( s_oldHardwareGamma[0][255] == 0 ) {
		return;
	}

	hDC = GetDC( GetDesktopWindow() );
	SetDeviceGammaRamp( hDC, s_oldHardwareGamma );
	ReleaseDC( GetDesktopWindow(), hDC );
}


