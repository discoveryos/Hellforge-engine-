#ifndef __GAME_BEARSHOOT_WINDOW_H__
#define __GAME_BEARSHOOT_WINDOW_H__

class hfGameBearShootWindow;

class BSEntity {
public:
	const hfMaterial *		material;
	hfStr					materialName;
	float					width, height;
	bool					visible;

	hfVec4					entColor;
	hfVec2					position;
	float					rotation;
	float					rotationSpeed;
	hfVec2					velocity;

	bool					fadeIn;
	bool					fadeOut;

	hfGameBearShootWindow *	game;
	
public:
						BSEntity(hfGameBearShootWindow* _game);
	virtual				~BSEntity();

	virtual void		WriteToSaveGame( hfFile *savefile );
	virtual void		ReadFromSaveGame( hfFile *savefile, hfGameBearShootWindow* _game );

	void				SetMaterial(const char* name);
	void				SetSize( float _width, float _height );
	void				SetVisible( bool isVisible );

	virtual void		Update( float timeslice );
	virtual void		Draw();

private:
};


class hfGameBearShootWindow : public hfWindow {
public:
	hfGameBearShootWindow(hfUserInterfaceLocal *gui);
	~hfGameBearShootWindow();

	virtual void		WriteToSaveGame( hfFile *savefile );
	virtual void		ReadFromSaveGame( hfFile *savefile );

	virtual const char*	HandleEvent(const sysEvent_t *event, bool *updateVisuals);
	virtual void		PostParse();
	virtual void		Draw(int time, float x, float y);
	virtual const char*	Activate(bool activate);
	virtual hfWinVar *	GetWinVarByName	(const char *_name, bool winLookup = false, drawWin_t** owner = NULL);

private:
	void				CommonInit();
	void				ResetGameState();

	void				UpdateBear();
	void				UpdateHelicopter();
	void				UpdateTurret();
	void				UpdateButtons();
	void				UpdateGame();
	void				UpdateScore();

	virtual bool		ParseInternalVar(const char *name, hfTokenParser *src);

private:

	hfWinBool			gamerunning;
	hfWinBool			onFire;
	hfWinBool			onContinue;
	hfWinBool			onNewGame;

	float				timeSlice;
	float				timeRemaining;
	bool				gameOver;

	int					currentLevel;
	int					goalsHit;
	bool				updateScore;
	bool				bearHitTarget;

	float				bearScale;
	bool				bearIsShrinking;
	int					bearShrinkStartTime;

	float				turretAngle;
	float				turretForce;

	float				windForce;
	int					windUpdateTime;

	hfList<BSEntity*>	entities;

	BSEntity			*turret;
	BSEntity			*bear;
	BSEntity			*helicopter;
	BSEntity			*goal;
	BSEntity			*wind;
	BSEntity			*gunblast;
};

#endif //__GAME_BEARSHOOT_WINDOW_H__

