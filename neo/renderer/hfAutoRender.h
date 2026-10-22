#ifndef __AUTO_RENDER_H__
#define __AUTO_RENDER_H__

class hfAutoRender : public hfSysThread {
public:
		hfAutoRender();
		// hfSysThread interface
		int			Run();

		void		StartBackgroundAutoSwaps( autoRenderIconType_t iconType );
		void		EndBackgroundAutoSwaps();

		autoRenderIconType_t	GetCurrentIcon() { return autoRenderIcon; }
		
private:
		void		RenderFrame();
		void		RenderBackground();
		void		RenderLoadingIcon( float fracX, float fracY, float size, float speed );

		int			nextRotateTime;
		float		currentRotation;
		autoRenderIconType_t autoRenderIcon;
};

extern hfAutoRender rAutoRender;

#endif // __AUTO_RENDER_H__
