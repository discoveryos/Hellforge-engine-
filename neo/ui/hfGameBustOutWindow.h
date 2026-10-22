#ifndef __GAME_BUSTOUT_WINDOW_H__
#define __GAME_BUSTOUT_WINDOW_H__

class hfGameBustOutWindow;

typedef enum {
	POWERUP_NONE = 0,
	POWERUP_BIGPADDLE,
	POWERUP_MULTIBALL
} powerupType_t;

class BOEntity {
public:
	bool					visible;

	hfStr					materialName;
	const hfMaterial *		material;
	float					width, height;
	hfVec4					color;
	hfVec2					position;
	hfVec2					velocity;

	powerupType_t			powerup;

	bool					removed;
	bool					fadeOut;

	hfGameBustOutWindow *	game;
	
public:
							BOEntity(hfGameBustOutWindow* _game);
	virtual					~BOEntity();

	virtual void			WriteToSaveGame( hfFile *savefile );
	virtual void			ReadFromSaveGame( hfFile *savefile, hfGameBustOutWindow* _game );

	void					SetMaterial(const char* name);
	void					SetSize( float _width, float _height );
	void					SetColor( float r, float g, float b, float a );
	void					SetVisible( bool isVisible );

	virtual void			Update( float timeslice, int guiTime );
	virtual void			Draw();

private:
};

typedef enum {
	COLLIDE_NONE = 0,
	COLLIDE_DOWN,
	COLLIDE_UP,
	COLLIDE_LEFT,
	COLLIDE_RIGHT
} collideDir_t;

class BOBrick {
public:
	float			x;
	float			y;
	float			width;
	float			height;
	powerupType_t	powerup;

	bool			isBroken;

	BOEntity		*ent;

public:
					BOBrick();
					BOBrick( BOEntity *_ent, float _x, float _y, float _width, float _height );
					~BOBrick();

	virtual void	WriteToSaveGame( hfFile *savefile );
	virtual void	ReadFromSaveGame( hfFile *savefile, hfGameBustOutWindow *game );

	void			SetColor( hfVec4 bcolor );
	collideDir_t	checkCollision( hfVec2 pos, hfVec2 vel );

private:
};

#define BOARD_ROWS 12

class hfGameBustOutWindow : public hfWindow {
public:
	hfGameBustOutWindow(hfUserInterfaceLocal *gui);
	~hfGameBustOutWindow();

	virtual void		WriteToSaveGame( hfFile *savefile );
	virtual void		ReadFromSaveGame( hfFile *savefile );

	virtual const char*	HandleEvent(const sysEvent_t *event, bool *updateVisuals);
	virtual void		PostParse();
	virtual void		Draw(int time, float x, float y);
	virtual const char*	Activate(bool activate);
	virtual hfWinVar *	GetWinVarByName	(const char *_name, bool winLookup = false, drawWin_t** owner = NULL);

	hfList<BOEntity*>	entities;

private:
	void				CommonInit();
	void				ResetGameState();

	void				ClearBoard();
	void				ClearPowerups();
	void				ClearBalls();

	void				LoadBoardFiles();
	void				SetCurrentBoard();
	void				UpdateGame();
	void				UpdatePowerups();
	void				UpdatePaddle();
	void				UpdateBall();
	void				UpdateScore();

	BOEntity *			CreateNewBall();
	BOEntity *			CreatePowerup( BOBrick *brick );

	virtual bool		ParseInternalVar(const char *name, hfTokenParser *src);

private:

	hfWinBool			gamerunning;
	hfWinBool			onFire;
	hfWinBool			onContinue;
	hfWinBool			onNewGame;
	hfWinBool			onNewLevel;

	float				timeSlice;
	bool				gameOver;

	int					numLevels;
	byte *				levelBoardData;
	bool				boardDataLoaded;

	int					numBricks;
	int					currentLevel;

	bool				updateScore;
	int					gameScore;
	int					nextBallScore;

	int					bigPaddleTime;
	float				paddleVelocity;

	float				ballSpeed;
	int					ballsRemaining;
	int					ballsInPlay;
	bool				ballHitCeiling;

	hfList<BOEntity*>	balls;
	hfList<BOEntity*>	powerUps;

	BOBrick				*paddle;
	hfList<BOBrick*>	board[BOARD_ROWS];
};

#endif //__GAME_BUSTOUT_WINDOW_H__

