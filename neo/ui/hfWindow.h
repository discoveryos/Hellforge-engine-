#ifndef __WINDOW_H__
#define __WINDOW_H__

#include "hfRectangle.h"
#include "hfDeviceContext.h"
#include "hfRegExp.h"
#include "hfWinvar.h"
#include "hfGuiScript.h"
#include "hfSimpleWindow.h"

const int WIN_CHILD			= 0x00000001;
const int WIN_CAPTION		= 0x00000002;
const int WIN_BORDER		= 0x00000004;
const int WIN_SIZABLE		= 0x00000008;
const int WIN_MOVABLE		= 0x00000010;
const int WIN_FOCUS			= 0x00000020;
const int WIN_CAPTURE		= 0x00000040;
const int WIN_HCENTER		= 0x00000080;
const int WIN_VCENTER		= 0x00000100;
const int WIN_MODAL			= 0x00000200;
const int WIN_INTRANSITION	= 0x00000400;
const int WIN_CANFOCUS		= 0x00000800;
const int WIN_SELECTED		= 0x00001000;
const int WIN_TRANSFORM		= 0x00002000;
const int WIN_HOLDCAPTURE	= 0x00004000;
const int WIN_NOWRAP		= 0x00008000;
const int WIN_NOCLIP		= 0x00010000;
const int WIN_INVERTRECT	= 0x00020000;
const int WIN_NATURALMAT	= 0x00040000;
const int WIN_NOCURSOR		= 0x00080000;
const int WIN_MENUGUI		= 0x00100000;
const int WIN_ACTIVE		= 0x00200000;
const int WIN_SHOWCOORDS	= 0x00400000;
const int WIN_SHOWTIME		= 0x00800000;
const int WIN_WANTENTER		= 0x01000000;

const int WIN_DESKTOP		= 0x10000000;

const char CAPTION_HEIGHT[] = "16.0";
const char SCROLLER_SIZE[] = "16.0";
const int SCROLLBAR_SIZE = 16;

const int MAX_WINDOW_NAME = 32;
const int MAX_LIST_ITEMS = 1024;

const char DEFAULT_BACKCOLOR[] = "1 1 1 1";
const char DEFAULT_FORECOLOR[] = "0 0 0 1";
const char DEFAULT_BORDERCOLOR[] = "0 0 0 1";
const char DEFAULT_TEXTSCALE[] = "0.4";

typedef enum {
	WOP_TYPE_ADD,
	WOP_TYPE_SUBTRACT,
	WOP_TYPE_MULTIPLY,
	WOP_TYPE_DIVIDE,
	WOP_TYPE_MOD,
	WOP_TYPE_TABLE,
	WOP_TYPE_GT,
	WOP_TYPE_GE,
	WOP_TYPE_LT,
	WOP_TYPE_LE,
	WOP_TYPE_EQ,
	WOP_TYPE_NE,
	WOP_TYPE_AND,
	WOP_TYPE_OR,
	WOP_TYPE_VAR,
	WOP_TYPE_VARS,
	WOP_TYPE_VARF,
	WOP_TYPE_VARI,
	WOP_TYPE_VARB,
	WOP_TYPE_COND
} wexpOpType_t;

typedef enum {
	WEXP_REG_TIME,
	WEXP_REG_NUM_PREDEFINED
} wexpRegister_t;

typedef struct {
	wexpOpType_t opType;	
	int	a, b, c, d;
} wexpOp_t;

struct hfRegEntry {
	const char *name;
	hfRegister::REGTYPE type;
	int index;
};


class rvGEWindowWrapper;
class hfWindow;

struct hfTimeLineEvent {
	hfTimeLineEvent() {
		event = new (TAG_OLD_UI) hfGuiScriptList;
	}
	~hfTimeLineEvent() {
		delete event;
	}
	int time;
	hfGuiScriptList *event;
	bool pending;
	size_t Size() {
		return sizeof(*this) + event->Size();
	}
};

class rvNamedEvent
{
public:

	rvNamedEvent(const char* name)
	{
		mEvent = new (TAG_OLD_UI) hfGuiScriptList;
		mName  = name;
	}
	~rvNamedEvent()
	{
		delete mEvent;
	}
	size_t Size() 
	{
		return sizeof(*this) + mEvent->Size();
	}
	
	hfStr				mName;
	hfGuiScriptList*	mEvent;
};

struct hfTransitionData {
	hfWinVar *data;
	int	offset;
	hfInterpolateAccelDecelLinear<hfVec4> interp;
};


class hfUserInterfaceLocal;
class hfWindow {
public:
	hfWindow(hfUserInterfaceLocal *gui);
	virtual ~hfWindow();

	enum {
		ON_MOUSEENTER = 0,
		ON_MOUSEEXIT,
		ON_ACTION,
		ON_ACTIVATE,
		ON_DEACTIVATE,
		ON_ESC,
		ON_FRAME,
		ON_TRIGGER,
		ON_ACTIONRELEASE,
		ON_ENTER,
		ON_ENTERRELEASE,
		SCRIPT_COUNT
	};

	enum {
		ADJUST_MOVE = 0,
		ADJUST_TOP,
		ADJUST_RIGHT,
		ADJUST_BOTTOM,
		ADJUST_LEFT,
		ADJUST_TOPLEFT,
		ADJUST_BOTTOMRIGHT,
		ADJUST_TOPRIGHT,
		ADJUST_BOTTOMLEFT
	};

	static const char *ScriptNames[SCRIPT_COUNT];

	static const hfRegEntry RegisterVars[];
	static const int		NumRegisterVars;

	hfWindow *SetFocus(hfWindow *w, bool scripts = true);

	hfWindow *SetCapture(hfWindow *w);
	void SetParent(hfWindow *w);
	void SetFlag(unsigned int f);
	void ClearFlag(unsigned int f);
	unsigned GetFlags() {return flags;};
	void Move(float x, float y);
	void BringToTop(hfWindow *w);
	void Adjust(float xd, float yd);
	void SetAdjustMode(hfWindow *child);
	void Size(float x, float y, float w, float h);
	void SetupFromState();
	void SetupBackground();
	drawWin_t *FindChildByName(const char *name);
	hfSimpleWindow *FindSimpleWinByName(const char *_name);
	hfWindow *GetParent() { return parent; }
	hfUserInterfaceLocal *GetGui() {return gui;};
	bool Contains(float x, float y);
	size_t Size();
	virtual size_t Allocated();
	hfStr* GetStrPtrByName(const char *_name);

	virtual hfWinVar *GetWinVarByName	(const char *_name, bool winLookup = false, drawWin_t** owner = NULL);

	int  GetWinVarOffset( hfWinVar *wv, drawWin_t *dw );
	float GetMaxCharHeight();
	float GetMaxCharWidth();
	void SetFont();
	void SetInitialState(const char *_name);
	void AddChild(hfWindow *win);
	void DebugDraw(int time, float x, float y);
	void CalcClientRect(float xofs, float yofs);
	void CommonInit();
	void CleanUp();
	void DrawBorderAndCaption(const hfRectangle &drawRect);
	void DrawCaption(int time, float x, float y);
	void SetupTransforms(float x, float y);
	bool Contains(const hfRectangle &sr, float x, float y);
	const char *GetName() { return name; };

	virtual bool Parse( hfTokenParser *src, bool rebuild = true);
	virtual const char *HandleEvent(const sysEvent_t *event, bool *updateVisuals);
	void	CalcRects(float x, float y);
	virtual void Redraw(float x, float y, bool hud);

	virtual void ArchiveToDictionary(hfDict *dict, bool useNames = true);
	virtual void InitFromDictionary(hfDict *dict, bool byName = true);
	virtual void PostParse();
	virtual void Activate( bool activate, hfStr &act );
	virtual void Trigger();
	virtual void GainFocus();
	virtual void LoseFocus();
	virtual void GainCapture();
	virtual void LoseCapture();
	virtual void Sized();
	virtual void Moved();
	virtual void Draw(int time, float x, float y);
	virtual void MouseExit();
	virtual void MouseEnter();
	virtual void DrawBackground(const hfRectangle &drawRect);
	virtual hfWindow * GetChildWithOnAction( float xd, float yd );
	virtual const char *RouteMouseCoords(float xd, float yd);
	virtual void SetBuddy(hfWindow *buddy) {};
	virtual void HandleBuddyUpdate(hfWindow *buddy) {};
	virtual void StateChanged( bool redraw );
	virtual void ReadFromDemoFile( class hfDemoFile *f, bool rebuild = true );
	virtual void WriteToDemoFile( class hfDemoFile *f );

	// SaveGame support
	void			WriteSaveGameString( const char *string, hfFile *savefile );
	void			WriteSaveGameTransition( hfTransitionData &trans, hfFile *savefile );
	virtual void	WriteToSaveGame( hfFile *savefile );
	void			ReadSaveGameString( hfStr &string, hfFile *savefile );
	void			ReadSaveGameTransition( hfTransitionData & trans, hfFile *savefile );
	virtual void	ReadFromSaveGame( hfFile *savefile );
	void			FixupTransitions();
	virtual void HasAction(){};
	virtual void HasScripts(){};

	void FixupParms();
	void GetScriptString(const char *name, hfStr &out);
	void SetScriptParams();
	bool HasOps() {	return (ops.Num() > 0); };
	float EvalRegs(int test = -1, bool force = false);
	void StartTransition();
	void AddTransition(hfWinVar *dest, hfVec4 from, hfVec4 to, int time, float accelTime, float decelTime);
	void ResetTime(int time);
	void ResetCinematics();

	int NumTransitions();

	bool ParseScript(hfTokenParser *src, hfGuiScriptList &list, int *timeParm = NULL, bool allowIf = false);
	bool RunScript(int n);
	bool RunScriptList(hfGuiScriptList *src);
	void SetRegs(const char *key, const char *val);
	int ParseExpression( hfTokenParser *src, hfWinVar *var = NULL, int component = 0 );
	int ExpressionConstant(float f);
	hfRegisterList *RegList() { return &regList; }
	void AddCommand(const char *cmd);
	void AddUpdateVar(hfWinVar *var);
	bool Interactive();
	bool ContainsStateVars();
	void SetChildWinVarVal(const char *name, const char *var, const char *val);
	hfWindow *GetFocusedChild();
	hfWindow *GetCaptureChild();
	const char *GetComment() { return comment;  }
	void SetComment( const char * p) { comment = p; }

	hfStr cmd;

	virtual void RunNamedEvent		( const char* eventName );

	void		AddDefinedVar		( hfWinVar* var );

	hfWindow*	FindChildByPoint	( float x, float y, hfWindow* below = NULL );
	int			GetChildIndex		( hfWindow* window );
	int			GetChildCount		();
	hfWindow*	GetChild			( int index );
	void		RemoveChild			( hfWindow *win );
	bool		InsertChild			( hfWindow *win, hfWindow* before );

	void		ScreenToClient		( hfRectangle* rect );
	void		ClientToScreen		( hfRectangle* rect );

	bool		UpdateFromDictionary ( hfDict& dict );

protected:

	friend		class rvGEWindowWrapper;

	hfWindow*	FindChildByPoint	( float x, float y, hfWindow** below );
	void		SetDefaults			();

	friend class hfSimpleWindow;
	friend class hfUserInterfaceLocal;
	bool IsSimple();
	void UpdateWinVars();
	void DisableRegister(const char *_name);
	void Transition();
	void Time();
	bool RunTimeEvents(int time);
	void Dump();

	int ExpressionTemporary();
	wexpOp_t *ExpressionOp();
	int EmitOp( int a, int b, wexpOpType_t opType, wexpOp_t **opp = NULL );
	int ParseEmitOp( hfTokenParser *src, int a, wexpOpType_t opType, int priority, wexpOp_t **opp = NULL );
	int ParseTerm( hfTokenParser *src, hfWinVar *var = NULL, int component = 0 );
	int ParseExpressionPriority( hfTokenParser *src, int priority, hfWinVar *var = NULL, int component = 0 );
	void EvaluateRegisters(float *registers);
	void SaveExpressionParseState();
	void RestoreExpressionParseState();
	void ParseBracedExpression(hfTokenParser *src);
	bool ParseScriptEntry(const char *name, hfTokenParser *src);
	bool ParseRegEntry(const char *name, hfTokenParser *src);
	virtual bool ParseInternalVar(const char *name, hfTokenParser *src);
	void ParseString(hfTokenParser *src, hfStr &out);
	void ParseVec4(hfTokenParser *src, hfVec4 &out);
	void ConvertRegEntry(const char *name, hfTokenParser *src, hfStr &out, int tabs);

	float actualX;					// physical coords
	float actualY;					// ''
	int	  childID;					// this childs id
	unsigned int flags;             // visible, focus, mouseover, cursor, border, etc.. 
	int lastTimeRun;				//
	hfRectangle drawRect;			// overall rect
	hfRectangle clientRect;			// client area
	hfVec2	origin;

	int timeLine;					// time stamp used for various fx
	float xOffset;			
	float yOffset;
	float forceAspectWidth;
	float forceAspectHeight;
	float matScalex;
	float matScaley;
	float borderSize;
	float textAlignx;
	float textAligny;
	hfStr	name;
	hfStr	comment;
	hfVec2	shear;

	class hfFont * font;
	signed char	textShadow;
	unsigned char cursor;					//
	signed char	textAlign;

	hfWinBool	noTime;					// 
	hfWinBool	visible;				// 
	hfWinBool	noEvents;
	hfWinRectangle rect;				// overall rect
	hfWinVec4	backColor;
	hfWinVec4	matColor;
	hfWinVec4	foreColor;
	hfWinVec4	hoverColor;
	hfWinVec4	borderColor;
	hfWinFloat	textScale;
	hfWinFloat	rotate;
	hfWinStr	text;
	hfWinBackground	backGroundName;			// 

	hfList<hfWinVar*, TAG_OLD_UI> definedVars;
	hfList<hfWinVar*, TAG_OLD_UI> updateVars;

	hfRectangle textRect;			// text extented rect
	const hfMaterial *background;         // background asset  

	hfWindow *parent;				// parent window
	hfList<hfWindow*, TAG_OLD_UI> children;		// child windows	
	hfList<drawWin_t, TAG_OLD_UI> drawWindows;		

	hfWindow *focusedChild;			// if a child window has the focus
	hfWindow *captureChild;			// if a child window has mouse capture
	hfWindow *overChild;			// if a child window has mouse capture
	bool hover;

	hfUserInterfaceLocal *gui;

	static hfCVar gui_debug;
	static hfCVar gui_edit;

	hfGuiScriptList *scripts[SCRIPT_COUNT];
	bool *saveTemps;

	hfList<hfTimeLineEvent*, TAG_OLD_UI> timeLineEvents;
	hfList<hfTransitionData, TAG_OLD_UI> transitions;

	static bool registerIsTemporary[MAX_EXPRESSION_REGISTERS]; // statics to assist during parsing

	hfList<wexpOp_t, TAG_OLD_UI> ops;			   	// evaluate to make expressionRegisters
	hfList<float, TAG_OLD_UI> expressionRegisters;
	hfList<wexpOp_t, TAG_OLD_UI> *saveOps;			   	// evaluate to make expressionRegisters
	hfList<rvNamedEvent*, TAG_OLD_UI>		namedEvents;		//  added named events
	hfList<float, TAG_OLD_UI> *saveRegs;

	hfRegisterList regList;

	hfWinBool	hideCursor;
};

HF_INLINE void hfWindow::AddDefinedVar( hfWinVar* var ) {
	definedVars.AddUnique( var );
}

#endif /* !__WINDOW_H__ */

