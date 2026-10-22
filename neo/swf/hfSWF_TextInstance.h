#ifndef __SWF_TEXTINSTANCE_H__
#define __SWF_TEXTINSTANCE_H__

struct subTimingWordData_t {
	subTimingWordData_t() {
		startTime = 0;
		forceBreak = false;
	}

	hfStr phrase;
	int startTime;
	bool forceBreak;
};

class hfSWFTextInstance {
public:
	hfSWFTextInstance();
	~hfSWFTextInstance();

	void Init( hfSWFEditText * editText, hfSWF * _swf );

	hfSWFScriptObject * GetScriptObject() { return &scriptObject; }

	bool	GetHasDropShadow() { return useDropShadow; }
	bool	HasStroke() { return useStroke; }
	float	GetStrokeStrength() { return strokeStrength; }
	float	GetStrokeWeight() { return strokeWeight; }

	// used for when text has random render mode set 
	bool	IsGeneratingRandomText() { return generatingText; }
	void	StartRandomText( int time );	
	hfStr	GetRandomText( int time );
	void	StartParagraphText( int time );
	hfStr	GetParagraphText( int time );
	bool	NeedsGenerateRandomText() { return triggerGenerate; }
	bool	NeedsSoundPlayed();
	void	ClearPlaySound() { needsSoundUpdate = false; }
	hfStr	GetSoundClip() { return soundClip; }
	void	SetIgnoreColor( bool ignore ) { ignoreColor = ignore; }

	void	SetStrokeInfo( bool use, float strength = 0.75f, float weight = 1.75f );
	int		CalcMaxScroll( int numLines = -1 );
	int		CalcNumLines();

	// subtitle functions
	void	SwitchSubtitleText( int time );
	bool	UpdateSubtitle( int time );
	bool	IsSubtitle() { return isSubtitle; }
	bool	IsUpdatingSubtitle() { return subUpdating; }
	void	SetSubEndIndex( int endChar, int time );
	int		GetLastWordIndex() { return subLastWordIndex; }
	int		GetPrevLastWordIndex() { return subPrevLastWordIndex; }
	void	LastWordChanged( int wordCount, int time );
	void	SetSubStartIndex( int value ) { subCharStartIndex = value; }
	int		GetSubEndIndex() { return subCharEndIndex; }
	int		GetSubStartIndex() { return subCharStartIndex; }
	void	SetSubNextStartIndex( int value );
	int		GetApporoximateSubtitleBreak( int time );
	bool	SubNeedsSwitch() { return subNeedsSwitch; }
	hfStr	GetPreviousText() { return subtitleText.c_str(); }
	void	SubtitleComplete();
	int		GetSubAlignment() { return subAlign; }
	hfStr	GetSpeaker() { return subSpeaker.c_str(); }
	void	SubtitleCleanup();
	float	GetTextLength();
	int		GetInputStartChar( ) { return inputTextStartChar; }
	void	SetInputStartCharacter( int c ) { inputTextStartChar = c; }

	const hfSWFEditText * GetEditText() const { return editText; }
	void	SetText( hfStr val ) { text = val; lengthCalculated = false; }

	// Removing the private access control statement due to cl 214702
	// Apparently MS's C++ compiler supports the newer C++ standard, and GCC supports C++03
	// In the new C++ standard, nested members of a friend class have access to private/protected members of the class granting friendship
	// In C++03, nested members defined in a friend class do NOT have access to private/protected members of the class granting friendship

	hfSWFEditText * editText;
	hfSWF *	swf;

	// this text instance's script object
	hfSWFScriptObject  scriptObject;

	hfStr text;
	hfStr randomtext;
	hfStr variable;
	swfColorRGBA_t color;

	bool visible;
	bool tooltip;

	int selectionStart;
	int selectionEnd;
	bool ignoreColor;

	int scroll;
	int scrollTime;
	int maxscroll;
	int maxLines;
	float glyphScale;
	swfRect_t bounds;
	float linespacing;

	bool shiftHeld;
	int lastInputTime;

	bool useDropShadow;
	bool useStroke;

	float strokeStrength;
	float strokeWeight;

	int		textLength;
	bool	lengthCalculated;

	swfTextRenderMode_t renderMode;
	bool		generatingText;
	int			rndSpotsVisible;
	int			rndSpacesVisible;
	int			charMultiplier;
	int			textSpotsVisible;
	int			rndTime;
	int			startRndTime;
	int			prevReplaceIndex;
	bool		triggerGenerate;
	int			renderDelay;
	bool		scrollUpdate;
	hfStr		soundClip;
	bool		needsSoundUpdate;
	hfList<int, TAG_SWF>	indexArray;
	hfRandom2	rnd;

	// used for subtitles
	bool		isSubtitle;
	int			subLength;
	int			subCharDisplayTime;
	int			subAlign;
	bool		subUpdating;
	int			subCharStartIndex;
	int			subNextStartIndex;
	int			subCharEndIndex;
	int			subDisplayTime;
	int			subStartTime;
	int			subSourceID;
	hfStr		subtitleText;
	bool		subNeedsSwitch;
	bool		subForceKillQueued;
	bool		subForceKill;
	int			subKillTimeDelay;
	int			subSwitchTime;
	int			subLastWordIndex;
	int			subPrevLastWordIndex;
	hfStr		subSpeaker;
	bool		subWaitClear;
	bool		subInitialLine;

	// input text
	int			inputTextStartChar;
	
	hfList< subTimingWordData_t, TAG_SWF > subtitleTimingInfo;
};

/*
================================================
This is the prototype object that all the text instance script objects reference
================================================
*/
class hfSWFScriptObject_TextInstancePrototype : public hfSWFScriptObject {
public:
	hfSWFScriptObject_TextInstancePrototype();

	//----------------------------------
	// Native Script Functions
	//----------------------------------
#define SWF_TEXT_FUNCTION_DECLARE( x ) \
	class hfSWFScriptFunction_##x : public hfSWFScriptFunction_RefCounted { \
	public: \
		void			AddRef() {} \
		void			Release() {} \
		hfSWFScriptVar Call( hfSWFScriptObject * thisObject, const hfSWFParmList & parms ); \
	} scriptFunction_##x;

	SWF_TEXT_FUNCTION_DECLARE( onKey );
	SWF_TEXT_FUNCTION_DECLARE( onChar );
	SWF_TEXT_FUNCTION_DECLARE( generateRnd );
	SWF_TEXT_FUNCTION_DECLARE( calcNumLines );

	SWF_NATIVE_VAR_DECLARE( text );
	SWF_NATIVE_VAR_DECLARE( autoSize );
	SWF_NATIVE_VAR_DECLARE( dropShadow );
	SWF_NATIVE_VAR_DECLARE( _stroke );
	SWF_NATIVE_VAR_DECLARE( _strokeStrength );
	SWF_NATIVE_VAR_DECLARE( _strokeWeight );
	SWF_NATIVE_VAR_DECLARE( variable );
	SWF_NATIVE_VAR_DECLARE( _alpha );
	SWF_NATIVE_VAR_DECLARE( textColor );
	SWF_NATIVE_VAR_DECLARE( _visible );
	SWF_NATIVE_VAR_DECLARE( scroll );
	SWF_NATIVE_VAR_DECLARE( maxscroll );
	SWF_NATIVE_VAR_DECLARE( selectionStart );
	SWF_NATIVE_VAR_DECLARE( selectionEnd );
	SWF_NATIVE_VAR_DECLARE( isTooltip );
	SWF_NATIVE_VAR_DECLARE( mode );
	SWF_NATIVE_VAR_DECLARE( delay );
	SWF_NATIVE_VAR_DECLARE( renderSound );
	SWF_NATIVE_VAR_DECLARE( updateScroll );
	SWF_NATIVE_VAR_DECLARE( subtitle );
	SWF_NATIVE_VAR_DECLARE( subtitleAlign );
	SWF_NATIVE_VAR_DECLARE( subtitleSourceID );
	SWF_NATIVE_VAR_DECLARE( subtitleSpeaker );
	
	SWF_NATIVE_VAR_DECLARE_READONLY( _textLength );

	SWF_TEXT_FUNCTION_DECLARE( subtitleSourceCheck );
	SWF_TEXT_FUNCTION_DECLARE( subtitleStart );
	SWF_TEXT_FUNCTION_DECLARE( subtitleLength );
	SWF_TEXT_FUNCTION_DECLARE( killSubtitle );
	SWF_TEXT_FUNCTION_DECLARE( forceKillSubtitle );
	SWF_TEXT_FUNCTION_DECLARE( subLastLine );
	SWF_TEXT_FUNCTION_DECLARE( addSubtitleInfo );
	SWF_TEXT_FUNCTION_DECLARE( terminateSubtitle );
	SWF_TEXT_FUNCTION_DECLARE( clearTimingInfo );
};

#endif // !__SWF_TEXTINSTANCE_H__

