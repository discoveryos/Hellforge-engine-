#ifndef __GAMEWINDOW_H__
#define __GAMEWINDOW_H__

class hfGameWindowProxy : public hfWindow {
public:
				hfGameWindowProxy( hfDeviceContext *d, hfUserInterfaceLocal *gui );
	void		Draw( int time, float x, float y );
};

#endif

