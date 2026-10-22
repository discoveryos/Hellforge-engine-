#ifndef __GAME_SSD_WINDOW_H__
#define __GAME_SSD_WINDOW_H__

class hfGameSSDWindow;

class SSDCrossHair {

public:
	enum {
		CROSSHAIR_STANDARD = 0,
		CROSSHAIR_SUPER,
		CROSSHAIR_COUNT
	};
	const hfMaterial*	crosshairMaterial[CROSSHAIR_COUNT];
	int					currentCrosshair;
	float				crosshairWidth, crosshairHeight;

public:
				SSDCrossHair();
				~SSDCrossHair();

	virtual void	WriteToSaveGame( hfFile *savefile );
	virtual void	ReadFromSaveGame( hfFile *savefile );

	void		InitCrosshairs();
	void		Draw(const hfVec2& cursor);
};

enum {
	SSD_ENTITY_BASE = 0,
	SSD_ENTITY_ASTEROID,
	SSD_ENTITY_ASTRONAUT,
	SSD_ENTITY_EXPLOSION,
	SSD_ENTITY_POINTS,
	SSD_ENTITY_PROJECTILE,
	SSD_ENTITY_POWERUP
};

class SSDEntity  {

public:
	//SSDEntity Information
	int					type;
	int					id;
	hfStr				materialName;
	const hfMaterial*	material;
	hfVec3				position;
	hfVec2				size;
	float				radius;
	float				hitRadius;
	float				rotation;

	hfVec4				matColor;

	hfStr				text;
	float				textScale;
	hfVec4				foreColor;

	hfGameSSDWindow*	game;
	int					currentTime;
	int					lastUpdate;
	int					elapsed;

	bool				destroyed;
	bool				noHit;
	bool				noPlayerDamage;

	bool				inUse;

	
public:
						SSDEntity();
	virtual				~SSDEntity();

	virtual void		WriteToSaveGame( hfFile *savefile );
	virtual void		ReadFromSaveGame( hfFile *savefile,  hfGameSSDWindow* _game );

	void				EntityInit();

	void				SetGame(hfGameSSDWindow* _game);
	void				SetMaterial(const char* _name);
	void				SetPosition(const hfVec3& _position);
	void				SetSize(const hfVec2& _size);
	void				SetRadius(float _radius, float _hitFactor = 1.0f);
	void				SetRotation(float _rotation);

	void				Update();
	bool				HitTest(const hfVec2& pt);
	

	virtual void		EntityUpdate() {};
	virtual void		Draw();
	virtual void		DestroyEntity();

	virtual void		OnHit(int key) {};
	virtual void		OnStrikePlayer() {};

	hfBounds			WorldToScreen(const hfBounds worldBounds);
	hfVec3				WorldToScreen(const hfVec3& worldPos);

	hfVec3				ScreenToWorld(const hfVec3& screenPos);

};

/*
*****************************************************************************
* SSDMover	
****************************************************************************
*/

class SSDMover : public SSDEntity {

public:
	hfVec3				speed;
	float				rotationSpeed;

public:
						SSDMover();
	virtual				~SSDMover();

	virtual void	WriteToSaveGame( hfFile *savefile );
	virtual void	ReadFromSaveGame( hfFile *savefile,  hfGameSSDWindow* _game  );

	void				MoverInit(const hfVec3& _speed, float _rotationSpeed);

	virtual void		EntityUpdate();


};

/*
*****************************************************************************
* SSDAsteroid	
****************************************************************************
*/

#define MAX_ASTEROIDS 64

class SSDAsteroid : public SSDMover {

public:

	int					health;

public:
						SSDAsteroid();
						~SSDAsteroid();

	virtual void	WriteToSaveGame( hfFile *savefile );
	virtual void	ReadFromSaveGame( hfFile *savefile,  hfGameSSDWindow* _game  );

	void				Init(hfGameSSDWindow* _game, const hfVec3& startPosition, const hfVec2& _size, float _speed, float rotate, int _health);

	virtual void		EntityUpdate();
	static SSDAsteroid*	GetNewAsteroid(hfGameSSDWindow* _game, const hfVec3& startPosition, const hfVec2& _size, float _speed, float rotate, int _health);
	static SSDAsteroid*	GetSpecificAsteroid(int id);
	static void			WriteAsteroids(hfFile* savefile);
	static void			ReadAsteroids(hfFile* savefile, hfGameSSDWindow* _game);


	
protected:
	static SSDAsteroid	asteroidPool[MAX_ASTEROIDS];
	
};

/*
*****************************************************************************
* SSDAstronaut	
****************************************************************************
*/
#define MAX_ASTRONAUT 8

class SSDAstronaut : public SSDMover {

public:

	int					health;

public:
							SSDAstronaut();
							~SSDAstronaut();
	
	virtual void	WriteToSaveGame( hfFile *savefile );
	virtual void	ReadFromSaveGame( hfFile *savefile,  hfGameSSDWindow* _game  );

	void					Init(hfGameSSDWindow* _game, const hfVec3& startPosition, float _speed, float rotate, int _health);

	static SSDAstronaut*	GetNewAstronaut(hfGameSSDWindow* _game, const hfVec3& startPosition, float _speed, float rotate, int _health);
	static SSDAstronaut*	GetSpecificAstronaut(int id);
	static void				WriteAstronauts(hfFile* savefile);
	static void				ReadAstronauts(hfFile* savefile, hfGameSSDWindow* _game);

protected:
	static SSDAstronaut	astronautPool[MAX_ASTRONAUT];

};

/*
*****************************************************************************
* SSDExplosion	
****************************************************************************
*/
#define MAX_EXPLOSIONS 64

class SSDExplosion : public SSDEntity {

public:
	hfVec2	finalSize;
	int		length;
	int		beginTime;
	int		endTime;
	int		explosionType;

	//The entity that is exploding
	SSDEntity*			buddy;
	bool				killBuddy;
	bool				followBuddy;

	enum {
		EXPLOSION_NORMAL = 0,
		EXPLOSION_TELEPORT = 1
	};

public:
	SSDExplosion();
	~SSDExplosion();

	virtual void	WriteToSaveGame( hfFile *savefile );
	virtual void	ReadFromSaveGame( hfFile *savefile,  hfGameSSDWindow* _game  );

	void				Init(hfGameSSDWindow* _game, const hfVec3& _position, const hfVec2& _size, int _length, int _type, SSDEntity* _buddy, bool _killBuddy = true, bool _followBuddy = true);

	virtual void		EntityUpdate();
	static SSDExplosion*	GetNewExplosion(hfGameSSDWindow* _game, const hfVec3& _position, const hfVec2& _size, int _length, int _type, SSDEntity* _buddy, bool _killBuddy = true, bool _followBuddy = true);
	static SSDExplosion*	GetSpecificExplosion(int id);
	static void				WriteExplosions(hfFile* savefile);
	static void				ReadExplosions(hfFile* savefile, hfGameSSDWindow* _game);

protected:
	static SSDExplosion	explosionPool[MAX_EXPLOSIONS];
};

#define MAX_POINTS 16

class SSDPoints : public SSDEntity {

	int		length;
	int		distance;
	int		beginTime;
	int		endTime;

	hfVec3	beginPosition;
	hfVec3	endPosition;

	hfVec4	beginColor;
	hfVec4	endColor;

	
public:
	SSDPoints();
	~SSDPoints();

	virtual void	WriteToSaveGame( hfFile *savefile );
	virtual void	ReadFromSaveGame( hfFile *savefile,  hfGameSSDWindow* _game  );

	void				Init(hfGameSSDWindow* _game, SSDEntity* _ent, int _points, int _length, int _distance, const hfVec4& color);
	virtual void		EntityUpdate();

	static SSDPoints*	GetNewPoints(hfGameSSDWindow* _game, SSDEntity* _ent, int _points, int _length, int _distance, const hfVec4& color);
	static SSDPoints*	GetSpecificPoints(int id);
	static void			WritePoints(hfFile* savefile);
	static void			ReadPoints(hfFile* savefile, hfGameSSDWindow* _game);

protected:
	static SSDPoints	pointsPool[MAX_POINTS];
};

#define MAX_PROJECTILES 64

class SSDProjectile : public SSDEntity {

	hfVec3	dir;
	hfVec3	speed;
	int		beginTime;
	int		endTime;

	hfVec3	endPosition;

public:
	SSDProjectile();
	~SSDProjectile();

	virtual void	WriteToSaveGame( hfFile *savefile );
	virtual void	ReadFromSaveGame( hfFile *savefile,  hfGameSSDWindow* _game  );

	void				Init(hfGameSSDWindow* _game, const hfVec3& _beginPosition, const hfVec3& _endPosition, float _speed, float _size);
	virtual void		EntityUpdate();

	static SSDProjectile* GetNewProjectile(hfGameSSDWindow* _game, const hfVec3& _beginPosition, const hfVec3& _endPosition, float _speed, float _size);
	static SSDProjectile* GetSpecificProjectile(int id);
	static void				WriteProjectiles(hfFile* savefile);
	static void				ReadProjectiles(hfFile* savefile, hfGameSSDWindow* _game);

protected:
	static SSDProjectile	projectilePool[MAX_PROJECTILES];
};


#define MAX_POWERUPS 64

/** 
* Powerups work in two phases:
*	1.) Closed container hurls at you
*		If you shoot the container it open
*	3.) If an opened powerup hits the player he aquires the powerup
* Powerup Types:
*	Health - Give a specific amount of health
*	Super Blaster - Increases the power of the blaster (lasts a specific amount of time)
*	Asteroid Nuke - Destroys all asteroids on screen as soon as it is aquired
*	Rescue Powerup - Rescues all astronauts as soon as it is acquited
*	Bonus Points - Gives some bonus points when acquired
*/
class SSDPowerup : public SSDMover {

	enum {
		POWERUP_STATE_CLOSED = 0,
		POWERUP_STATE_OPEN
	};

	enum {
		POWERUP_TYPE_HEALTH = 0,
		POWERUP_TYPE_SUPER_BLASTER,
		POWERUP_TYPE_ASTEROID_NUKE,
		POWERUP_TYPE_RESCUE_ALL,
		POWERUP_TYPE_BONUS_POINTS,
		POWERUP_TYPE_DAMAGE,
		POWERUP_TYPE_MAX
	};

	int powerupState;
	int powerupType;


public:


public:
	SSDPowerup();
	virtual ~SSDPowerup();

	virtual void	WriteToSaveGame( hfFile *savefile );
	virtual void	ReadFromSaveGame( hfFile *savefile,  hfGameSSDWindow* _game  );

	virtual void		OnHit(int key);
	virtual void		OnStrikePlayer();

	void	OnOpenPowerup();
	void	OnActivatePowerup();

	

	void	Init(hfGameSSDWindow* _game, float _speed, float _rotation);

	static SSDPowerup* GetNewPowerup(hfGameSSDWindow* _game, float _speed, float _rotation);
	static SSDPowerup* GetSpecificPowerup(int id);
	static void			WritePowerups(hfFile* savefile);
	static void			ReadPowerups(hfFile* savefile, hfGameSSDWindow* _game);

protected:
	static SSDPowerup	powerupPool[MAX_POWERUPS];

};


typedef struct {
	float	spawnBuffer;
	int		needToWin;
} SSDLevelData_t;

typedef struct {
	float	speedMin, speedMax;
	float	sizeMin, sizeMax;
	float	rotateMin, rotateMax;
	int		spawnMin, spawnMax;
	int		asteroidHealth;
	int		asteroidPoints;
	int		asteroidDamage;
} SSDAsteroidData_t;

typedef struct {
	float	speedMin, speedMax;
	float	rotateMin, rotateMax;
	int		spawnMin, spawnMax;
	int		health;
	int		points;
	int		penalty;
} SSDAstronautData_t;

typedef struct {
	float	speedMin, speedMax;
	float	rotateMin, rotateMax;
	int		spawnMin, spawnMax;
} SSDPowerupData_t;

typedef struct {
	float	speed;
	int		damage;
	int		size;
} SSDWeaponData_t;

/** 
* SSDLevelStats_t
*	Data that is used for each level. This data is reset
*	each new level.
*/
typedef struct {
	int					shotCount;
	int					hitCount;
	int					destroyedAsteroids;
	int					nextAsteroidSpawnTime;

	int					killedAstronauts;
	int					savedAstronauts;

	//Astronaut Level Data
	int					nextAstronautSpawnTime;

	//Powerup Level Data
	int					nextPowerupSpawnTime;

	SSDEntity*			targetEnt;
} SSDLevelStats_t;

/** 
* SSDGameStats_t
*	Data that is used for the game that is currently running. Memset this
*	to completely reset the game
*/
typedef struct {
	bool				gameRunning;

	int					score;
	int					prebonusscore;

	int					health;

	int					currentWeapon;
	int					currentLevel;
	int					nextLevel;

	SSDLevelStats_t		levelStats;
} SSDGameStats_t;


class hfGameSSDWindow : public hfWindow {
public:
	hfGameSSDWindow(hfUserInterfaceLocal *gui);
	~hfGameSSDWindow();

	virtual void	WriteToSaveGame( hfFile *savefile );
	virtual void	ReadFromSaveGame( hfFile *savefile );

	virtual const char*	HandleEvent(const sysEvent_t *event, bool *updateVisuals);
	virtual hfWinVar*	GetWinVarByName	(const char *_name, bool winLookup = false, drawWin_t** owner = NULL);
	
	
	virtual void		Draw(int time, float x, float y);

	void				AddHealth(int health);
	void				AddScore(SSDEntity* ent, int points);
	void				AddDamage(int damage);

	void				OnNuke();
	void				OnRescueAll();
	void				OnSuperBlaster();

	SSDEntity*			GetSpecificEntity(int type, int id);

	void				PlaySound(const char* sound);




	static hfRandom		random;	
	int					ssdTime;
	
private:
	
	//Initialization
	virtual bool		ParseInternalVar(const char *name, hfTokenParser *src);
	void				ParseLevelData(int level, const hfStr& levelDataString);
	void				ParseAsteroidData(int level, const hfStr& asteroidDataString);
	void				ParseWeaponData(int weapon, const hfStr& weaponDataString);
	void				ParseAstronautData(int level, const hfStr& astronautDataString);
	void				ParsePowerupData(int level, const hfStr& powerupDataString);

	void				CommonInit();
	void				ResetGameStats();
	void				ResetLevelStats();
	void				ResetEntities();

	//Game Running Methods
	void				StartGame();
	void				StopGame();
	void				GameOver();

	//Starting the Game
	void				BeginLevel(int level);
	void				ContinueGame();

	//Stopping the Game
	void				LevelComplete();
	void				GameComplete();

	

	void				UpdateGame();
	void				CheckForHits();
	void				ZOrderEntities();

	void				SpawnAsteroid();

	void				FireWeapon(int key);
	SSDEntity*			EntityHitTest(const hfVec2& pt);

	void				HitAsteroid(SSDAsteroid* asteroid, int key);
	void				AsteroidStruckPlayer(SSDAsteroid* asteroid);

	
	

	
	void				RefreshGuiData();

	hfVec2				GetCursorWorld();

	//Astronaut Methods
	void				SpawnAstronaut();
	void				HitAstronaut(SSDAstronaut* astronaut, int key);
	void				AstronautStruckPlayer(SSDAstronaut* astronaut);

	//Powerup Methods
	void				SpawnPowerup();


	void				StartSuperBlaster();
	void				StopSuperBlaster();

	//void				FreeSoundEmitter( bool immediate );

	


public:

	//WinVars used to call functions from the guis
	hfWinBool					beginLevel;
	hfWinBool					resetGame;
	hfWinBool					continueGame;
	hfWinBool					refreshGuiData;

	SSDCrossHair				crosshair;
	hfBounds					screenBounds;

	//Level Data
	int							levelCount;
	hfList<SSDLevelData_t>		levelData;
	hfList<SSDAsteroidData_t>	asteroidData;
	hfList<SSDAstronautData_t>	astronautData;
	hfList<SSDPowerupData_t>	powerupData;

	
	//Weapon Data
	int							weaponCount;
	hfList<SSDWeaponData_t>		weaponData;

	int							superBlasterTimeout;

	//All current game data is stored in this structure (except the entity list)
	SSDGameStats_t				gameStats;
	hfList<SSDEntity*>			entities;

	int							currentSound;
	
};

#endif //__GAME_SSD_WINDOW_H__

