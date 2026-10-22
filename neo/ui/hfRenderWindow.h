#ifndef __RENDERWINDOW_H
#define __RENDERWINDOW_H

class hfUserInterfaceLocal;
class hfRenderWindow : public hfWindow {
public:
	hfRenderWindow(hfUserInterfaceLocal *gui);
	virtual ~hfRenderWindow();

	virtual void PostParse();
	virtual void Draw(int time, float x, float y);
	virtual size_t Allocated(){return hfWindow::Allocated();};
// 
//  
	virtual hfWinVar *GetWinVarByName(const char *_name, bool winLookup = false, drawWin_t** owner = NULL);
// 
	
private:
	void CommonInit();
	virtual bool ParseInternalVar(const char *name, hfTokenParser *src);
	void Render(int time);
	void PreRender();
	void BuildAnimation(int time);
	renderView_t refdef;
	hfRenderWorld *world;
	renderEntity_t worldEntity;
	renderLight_t rLight;
	void *modelAnim;

	qhandle_t	worldModelDef;
	qhandle_t	lightDef;
	qhandle_t   modelDef;
	hfWinStr modelName;
	hfWinStr animName;
	hfStr	 animClass;
	hfWinVec4 lightOrigin;
	hfWinVec4 lightColor;
	hfWinVec4 modelOrigin;
	hfWinVec4 modelRotate;
	hfWinVec4 viewOffset;
	hfWinBool needsRender;
	int animLength;
	int animEndTime;
	bool updateAnimation;
};

#endif // __RENDERWINDOW_H

