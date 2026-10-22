class hfWindow;

class hfUserInterfaceLocal : public hfUserInterface {
	friend class hfUserInterfaceManagerLocal;
public:
								hfUserInterfaceLocal();
	virtual						~hfUserInterfaceLocal();

	virtual const char *		Name() const;
	virtual const char *		Comment() const;
	virtual bool				IsInteractive() const;
	virtual bool				InitFromFile( const char *qpath, bool rebuild = true, bool cache = true );
	virtual const char *		HandleEvent( const sysEvent_t *event, int time, bool *updateVisuals );
	virtual void				HandleNamedEvent( const char* namedEvent );
	virtual void				Redraw( int time, bool hud );
	virtual void				DrawCursor();
	virtual const hfDict &		State() const;
	virtual void				DeleteStateVar( const char *varName );
	virtual void				SetStateString( const char *varName, const char *value );
	virtual void				SetStateBool( const char *varName, const bool value );
	virtual void				SetStateInt( const char *varName, const int value );
	virtual void				SetStateFloat( const char *varName, const float value );

	// Gets a gui state variable
	virtual const char*			GetStateString( const char *varName, const char* defaultString = "" ) const;
	virtual bool				GetStateBool( const char *varName, const char* defaultString = "0" ) const;
	virtual int					GetStateInt( const char *varName, const char* defaultString = "0" ) const;
	virtual float				GetStateFloat( const char *varName, const char* defaultString = "0" ) const;

	virtual void				StateChanged( int time, bool redraw );
	virtual const char *		Activate( bool activate, int time );
	virtual void				Trigger( int time );
	virtual void				ReadFromDemoFile( class hfDemoFile *f );
	virtual void				WriteToDemoFile( class hfDemoFile *f );
	virtual bool				WriteToSaveGame( hfFile *savefile ) const;
	virtual bool				ReadFromSaveGame( hfFile * savefile );
	virtual void				SetKeyBindingNames();
	virtual bool				IsUniqued() const { return uniqued; };
	virtual void				SetUniqued( bool b ) { uniqued = b; };
	virtual void				SetCursor( float x, float y );

	virtual float				CursorX() { return cursorX; }
	virtual float				CursorY() { return cursorY; }

	size_t						Size();

	hfDict *					GetStateDict() { return &state; }

	const char *				GetSourceFile() const { return source; }
	HF_TIME_T						GetTimeStamp() const { return timeStamp; }

	hfWindow *					GetDesktop() const { return desktop; }
	void						SetBindHandler( hfWindow *win ) { bindHandler = win; }
	bool						Active() const { return active; }
	int							GetTime() const { return time; }
	void						SetTime( int _time ) { time = _time; }

	void						ClearRefs() { refs = 0; }
	void						AddRef() { refs++; }
	int							GetRefs() { return refs; }

	void						RecurseSetKeyBindingNames( hfWindow *window );
	hfStr						&GetPendingCmd() { return pendingCmd; };
	hfStr						&GetReturnCmd() { return returnCmd; };

private:
	bool						active;
	bool						loading;
	bool						interactive;
	bool						uniqued;

	hfDict						state;
	hfWindow *					desktop;
	hfWindow *					bindHandler;

	hfStr						source;
	hfStr						activateStr;
	hfStr						pendingCmd;
	hfStr						returnCmd;
	HF_TIME_T						timeStamp;

	float						cursorX;
	float						cursorY;

	int							time;

	int							refs;
};



class hfUserInterfaceManagerLocal : public hfUserInterfaceManager {
	friend class hfUserInterfaceLocal;

public:
	virtual void				Init();
	virtual void				Shutdown();
	virtual void				SetDrawingDC();
	virtual void				Touch( const char *name );
	virtual void				WritePrecacheCommands( hfFile *f );
	virtual void				SetSize( float width, float height );
	virtual void				BeginLevelLoad();
	virtual void				EndLevelLoad( const char *mapName );
	virtual void				Preload( const char *mapName );
	virtual void				Reload( bool all );
	virtual void				ListGuis() const;
	virtual bool				CheckGui( const char *qpath ) const;
	virtual hfUserInterface *	Alloc() const;
	virtual void				DeAlloc( hfUserInterface *gui );
	virtual hfUserInterface *	FindGui( const char *qpath, bool autoLoad = false, bool needInteractive = false, bool forceUnique = false );
	virtual hfUserInterface *	FindDemoGui( const char *qpath );
	virtual	hfListGUI *			AllocListGUI() const;
	virtual void				FreeListGUI( hfListGUI *listgui );
	hfTokenParser &				GetBinaryParser() { return mapParser; }
private:
	hfRectangle					screenRect;
	hfDeviceContext				dcOld;
	hfDeviceContextOptimized	dcOptimized;

	hfList<hfUserInterfaceLocal*> guis;
	hfList<hfUserInterfaceLocal*> demoGuis;

	hfTokenParser				mapParser;
};

// These used to be in every window, but they all pointed at the same one in hfUserInterfaceManagerLocal.
// Made a global so it can be switched out dynamically to test optimized versions.
extern hfDeviceContext *dc;

