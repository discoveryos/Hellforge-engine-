#ifndef __HF_UI_FRONTEND_H__
#define __HF_UI_FRONTEND_H__

/*
===============================================================================

	Phase 16 — hfUiFrontend.

	The screen-flow layer that turns the Phase 14/15 widget kit into a game
	that starts at a title screen instead of falling straight into a session:

		BOOT -> TITLE -> MENU            (options)
		          TITLE -> PLAYING <-> PAUSED
		          PLAYING -> RESULTS -> PLAYING | TITLE
		          any -> LOADING -> PLAYING

	Each state owns a separate UI tree.  A transition clears the registry and
	rebuilds, so no entity from the previous screen can leak into the next and
	the frontend never needs a per-widget teardown path.

	The frontend owns its registry and UI system, which means a host can hand
	them to the renderer with hfUiDrawPass::SetSource() and keep the pointers
	stable for the lifetime of the frontend.

	Input arrives either as absolute/relative system events (HandleSysEvent)
	or as direct calls (SetMouse/SetButton), which is what makes the whole
	state machine drivable from the harness with no window.

	The arena simulation is optional: when one is attached the frontend steps
	it at a fixed 60 Hz while PLAYING, mirrors score/health/wave into the HUD,
	and jumps to RESULTS when the sim reports game over.  Attach nothing and
	the frontend is still a complete menu system.

===============================================================================
*/

#include "../hfprecompiled.h"
#include "../hfEcs.h"
#include "hfCanvasComponent.h"
#include "hfUiWidget.h"
#include "hfUiEvents.h"
#include "hfUiBuilder.h"
#include "hfUiSystem.h"
#include "hfUiInput.h"
#include "hfUiTheme.h"
#include "hfUiSaveSlots.h"
#include "hfUiKeybinds.h"
#include "../game/hfArena.h"
#include <stdio.h>

namespace HF_Ui {

class hfUiFrontend {
public:
	enum hfUiFrontendState {
		HF_UI_FRONTEND_BOOT = 0,
		HF_UI_FRONTEND_TITLE,
		HF_UI_FRONTEND_MENU,
		HF_UI_FRONTEND_LOADING,
		HF_UI_FRONTEND_PLAYING,
		HF_UI_FRONTEND_PAUSED,
		HF_UI_FRONTEND_RESULTS,
		HF_UI_FRONTEND_SAVES,
		HF_UI_FRONTEND_STATE_COUNT
	};

	enum hfUiFrontendAction {
		HF_UI_ACTION_NONE = 0,
		HF_UI_ACTION_START,
		HF_UI_ACTION_OPTIONS,
		HF_UI_ACTION_BACK,
		HF_UI_ACTION_PAUSE,
		HF_UI_ACTION_RESUME,
		HF_UI_ACTION_RESTART,
		HF_UI_ACTION_MAIN_MENU,
		HF_UI_ACTION_QUIT,
		HF_UI_ACTION_SAVES,
		HF_UI_ACTION_SAVE_SLOT_LOAD,
		HF_UI_ACTION_SAVE_SLOT_WRITE,
		HF_UI_ACTION_SAVE_SLOT_DELETE,
		HF_UI_ACTION_COUNT
	};

	enum hfUiHudLabel {
		HF_UI_HUD_SCORE = 0,
		HF_UI_HUD_HEALTH,
		HF_UI_HUD_WAVE,
		HF_UI_HUD_COUNT
	};

	typedef void ( *hfUiActionHandler )( int action, void *user );

	hfUiFrontend() {
		m_bus = NULL;
		m_arena = NULL;
		m_actionHandler = NULL;
		m_actionUser = NULL;
		m_state = HF_UI_FRONTEND_BOOT;
		m_pendingAction = HF_UI_ACTION_NONE;
		m_width = 1024.0f;
		m_height = 768.0f;
		m_seed = 1;
		m_arenaAccum = 0.0f;
		m_quitRequested = false;
		m_hostHandled = false;
		m_joyRepeatCooldown = 0;
		m_joyAxis[ 0 ] = m_joyAxis[ 1 ] = 0;
		m_bindCount = 0;
		m_logCount = 0;
		m_root = HF_Ecs::ENTITY_NULL;
		m_resultLabel = HF_Ecs::ENTITY_NULL;
		m_musicSlider = HF_Ecs::ENTITY_NULL;
		m_sfxSlider = HF_Ecs::ENTITY_NULL;
		m_fullscreenToggle = HF_Ecs::ENTITY_NULL;
		for ( int i = 0; i < HF_UI_HUD_COUNT; i++ ) { m_hud[ i ] = HF_Ecs::ENTITY_NULL; }
		m_musicVolume = 70.0f;
		m_sfxVolume = 85.0f;
		m_fullscreen = false;
		m_lastSaveSlot = -1;
		m_lastSaveKind = -1;
		m_pendingBindRow = -1;
		m_keybindRowCount = 0;
		m_keybindStatus = HF_Ecs::ENTITY_NULL;
		m_keybinds.SetDefaults();
	}

	// Initialise once.  The event bus must outlive the frontend.  The
	// registry starts empty; SetState(TITLE) is called for you.
	void Init( HF_Ecs::EventBus &bus, float width, float height ) {
		m_bus = &bus;
		m_width = width;
		m_height = height;
		m_ui.SetViewport( width, height );
		m_bus->Subscribe( "ui.click", &hfUiFrontend::OnUiEvent, this );
		m_bus->Subscribe( "ui.change", &hfUiFrontend::OnUiEvent, this );
		m_keybinds.SetDefaults();
		SetState( HF_UI_FRONTEND_TITLE );
	}

	void Shutdown() {
		m_registry.Clear();
		m_input.Reset();
		m_bindCount = 0;
		m_bus = NULL;
		m_state = HF_UI_FRONTEND_BOOT;
		m_hostHandled = false;
		m_joyRepeatCooldown = 0;
		m_joyAxis[ 0 ] = m_joyAxis[ 1 ] = 0;
		m_root = HF_Ecs::ENTITY_NULL;
		m_resultLabel = HF_Ecs::ENTITY_NULL;
		m_pendingBindRow = -1;
		m_keybindRowCount = 0;
		m_keybindStatus = HF_Ecs::ENTITY_NULL;
		for ( int i = 0; i < HF_UI_HUD_COUNT; i++ ) { m_hud[ i ] = HF_Ecs::ENTITY_NULL; }
	}

	// --- host wiring -----------------------------------------------------

	void SetArena( HfArena *arena ) { m_arena = arena; }
	HfArena *			GetArena() const { return m_arena; }
	HF_Ecs::Registry *	RegistryPtr() { return &m_registry; }
	hfUiSystem *		SystemPtr() { return &m_ui; }
	hfUiSystem &		System() { return m_ui; }
	hfUiInput &			Input() { return m_input; }
	hfUiFrontendState	GetState() const { return m_state; }
	bool				QuitRequested() const { return m_quitRequested; }
	void				ClearQuitRequest() { m_quitRequested = false; }

	HF_Ecs::Entity	GetHudLabel( int which ) const {
		if ( which < 0 || which >= HF_UI_HUD_COUNT ) { return HF_Ecs::ENTITY_NULL; }
		return m_hud[ which ];
	}
	HF_Ecs::Entity	GetMusicSlider() const { return m_musicSlider; }
	HF_Ecs::Entity	GetSfxSlider() const { return m_sfxSlider; }
	HF_Ecs::Entity	GetFullscreenToggle() const { return m_fullscreenToggle; }
	HF_Ecs::Entity	GetResultLabel() const { return m_resultLabel; }
	hfUiSaveSlots &	SaveSlots() { return m_saveSlots; }
	const hfUiSaveSlots & SaveSlots() const { return m_saveSlots; }
	int GetLastSaveSlot() const { return m_lastSaveSlot; }
	int GetLastSaveKind() const { return m_lastSaveKind; }

	hfUiKeybinds &			Keybinds() { return m_keybinds; }
	const hfUiKeybinds &	Keybinds() const { return m_keybinds; }
	int						GetPendingBindRow() const { return m_pendingBindRow; }
	int						GetKeybindRowCount() const { return m_keybindRowCount; }
	HF_Ecs::Entity			GetKeybindButton( int row ) const {
		if ( row < 0 || row >= m_keybindRowCount ) { return HF_Ecs::ENTITY_NULL; }
		return m_keybindRows[ row ].entity;
	}
	HF_Ecs::Entity			GetKeybindStatusLabel() const { return m_keybindStatus; }

	float	GetMusicVolume() const { return m_musicVolume; }
	float	GetSfxVolume() const { return m_sfxVolume; }
	bool	GetFullscreen() const { return m_fullscreen; }

	void SetMusicVolume( float v ) {
		m_musicVolume = v;
		if ( m_state == HF_UI_FRONTEND_MENU ) { SetState( HF_UI_FRONTEND_MENU ); }
	}
	void SetSfxVolume( float v ) {
		m_sfxVolume = v;
		if ( m_state == HF_UI_FRONTEND_MENU ) { SetState( HF_UI_FRONTEND_MENU ); }
	}
	void SetFullscreen( bool on ) {
		m_fullscreen = on;
		if ( m_state == HF_UI_FRONTEND_MENU ) { SetState( HF_UI_FRONTEND_MENU ); }
	}

	// --- theme ------------------------------------------------------------

	const hfUiTheme &	GetTheme() const { return m_theme; }

	// Reskin every screen.  Rebuilding the current screen is what makes a
	// theme change take effect immediately without any widget code knowing
	// that themes exist.
	void SetTheme( const hfUiTheme &theme ) {
		m_theme = theme;
		if ( m_state != HF_UI_FRONTEND_BOOT ) { SetState( m_state ); }
	}

	void SetActionHandler( hfUiActionHandler handler, void *user ) {
		m_actionHandler = handler;
		m_actionUser = user;
	}

	// A host handler calls this when it has taken over an action.  START is
	// the important case: a real map load owns the LOADING -> PLAYING
	// transition, so the frontend must not reset its arena and jump straight
	// to PLAYING behind the host's back.  The flag is cleared at the start of
	// every action, so it cannot leak into a later button press.
	void MarkHostHandled() { m_hostHandled = true; }
	bool WasHostHandled() const { return m_hostHandled; }

	// --- action log (deterministic; what the harness compares) ------------

	int		GetActionLogCount() const { return m_logCount; }
	int		GetActionLog( int i ) const {
		if ( i < 0 || i >= m_logCount ) { return HF_UI_ACTION_NONE; }
		return m_log[ i ];
	}

	// First button bound to an action in the current state (ENTITY_NULL when
	// the action is not on screen).  Tests use it to click real controls.
	HF_Ecs::Entity FindAction( int action ) const {
		for ( int i = 0; i < m_bindCount; i++ ) {
			if ( m_binds[ i ].action == action ) { return m_binds[ i ].entity; }
		}
		return HF_Ecs::ENTITY_NULL;
	}

	// --- transitions ------------------------------------------------------

	void SetState( hfUiFrontendState state ) {
		const hfUiFrontendState previous = m_state;
		m_state = state;
		m_pendingAction = HF_UI_ACTION_NONE;
		m_arenaAccum = 0.0f;
		m_joyRepeatCooldown = 0;
		m_joyAxis[ 0 ] = m_joyAxis[ 1 ] = 0;

		// One registry per frontend, cleared per screen: cheap, leak-free and
		// it invalidates every stale entity reference in one step.
		m_registry.Clear();
		m_input.Reset();
		m_bindCount = 0;
		m_root = HF_Ecs::ENTITY_NULL;
		m_resultLabel = HF_Ecs::ENTITY_NULL;
		m_musicSlider = HF_Ecs::ENTITY_NULL;
		m_sfxSlider = HF_Ecs::ENTITY_NULL;
		m_fullscreenToggle = HF_Ecs::ENTITY_NULL;
		m_pendingBindRow = -1;
		m_keybindRowCount = 0;
		m_keybindStatus = HF_Ecs::ENTITY_NULL;
		for ( int i = 0; i < HF_UI_HUD_COUNT; i++ ) { m_hud[ i ] = HF_Ecs::ENTITY_NULL; }

		BuildState( state );
		m_theme.Apply( m_registry );
		SyncHud();
		m_ui.Update( m_registry, 0.0f );

		if ( m_bus != NULL ) {
			hfUiEventState ev = { ( int )state, ( int )previous };
			m_bus->Publish( "ui.state", &ev, sizeof( ev ) );
		}
	}

	// Load a game: reset the arena (if any) and enter PLAYING.
	void BeginGame() {
		if ( m_arena != NULL ) {
			m_arena->Reset( m_seed++ );
		}
		SetState( HF_UI_FRONTEND_PLAYING );
	}

	void PerformAction( int action ) {
		if ( action == HF_UI_ACTION_NONE ) {
			return;
		}
		LogAction( action );
		m_hostHandled = false;
		if ( m_actionHandler != NULL ) {
			m_actionHandler( action, m_actionUser );
		}

		switch ( action ) {
			case HF_UI_ACTION_START:
				if ( !m_hostHandled ) {
					BeginGame();
				}
				break;
			case HF_UI_ACTION_OPTIONS:
				SetState( HF_UI_FRONTEND_MENU );
				break;
			case HF_UI_ACTION_SAVES:
				SetState( HF_UI_FRONTEND_SAVES );
				break;
			case HF_UI_ACTION_BACK:
			case HF_UI_ACTION_MAIN_MENU:
				SetState( HF_UI_FRONTEND_TITLE );
				break;
			case HF_UI_ACTION_PAUSE:
				if ( m_state == HF_UI_FRONTEND_PLAYING ) { SetState( HF_UI_FRONTEND_PAUSED ); }
				break;
			case HF_UI_ACTION_RESUME:
				if ( m_state == HF_UI_FRONTEND_PAUSED ) { SetState( HF_UI_FRONTEND_PLAYING ); }
				break;
			case HF_UI_ACTION_RESTART:
				if ( m_arena != NULL ) { m_arena->Reset( m_seed++ ); }
				SetState( HF_UI_FRONTEND_PLAYING );
				break;
			case HF_UI_ACTION_QUIT:
				m_quitRequested = true;
				if ( m_bus != NULL ) { m_bus->Publish( "ui.quit" ); }
				break;
			case HF_UI_ACTION_SAVE_SLOT_DELETE:
				if ( m_lastSaveSlot >= 0 ) {
					m_saveSlots.Delete( m_lastSaveSlot );
					m_saveSlots.Refresh( m_registry, m_ui );
				}
				break;
			case HF_UI_ACTION_SAVE_SLOT_LOAD:
			case HF_UI_ACTION_SAVE_SLOT_WRITE:
				// Policy lives in the host; the handler sees the action and
				// GetLastSaveSlot().
				break;
			default:
				break;
		}
	}

	// Escape / gamepad-B behaviour for the current screen.
	void Back() {
		switch ( m_state ) {
			case HF_UI_FRONTEND_PLAYING:	PerformAction( HF_UI_ACTION_PAUSE ); break;
			case HF_UI_FRONTEND_PAUSED:		PerformAction( HF_UI_ACTION_RESUME ); break;
			case HF_UI_FRONTEND_MENU:		PerformAction( HF_UI_ACTION_BACK ); break;
			case HF_UI_FRONTEND_RESULTS:	PerformAction( HF_UI_ACTION_MAIN_MENU ); break;
			case HF_UI_FRONTEND_SAVES:		PerformAction( HF_UI_ACTION_BACK ); break;
			default: break;
		}
	}

	// --- input ------------------------------------------------------------

	void SetMouse( float x, float y ) {
		m_mouseX = Clamp( x, 0.0f, m_width );
		m_mouseY = Clamp( y, 0.0f, m_height );
		m_input.SetMouse( m_mouseX, m_mouseY );
	}
	void SetButton( int button, bool down ) {
		m_input.SetButton( button, down );
	}

	// Absolute and relative mouse events, mouse buttons, Enter/Space to
	// activate the focused control, Escape to go back.  Returns true when the
	// event was consumed by the UI.
	bool HandleSysEvent( const sysEvent_t &ev ) {
		switch ( ev.evType ) {
			case SE_MOUSE_ABSOLUTE:
				SetMouse( ( float )ev.evValue, ( float )ev.evValue2 );
				return true;
			case SE_MOUSE:
				SetMouse( m_mouseX + ( float )ev.evValue, m_mouseY + ( float )ev.evValue2 );
				return true;
			case SE_KEY: {
				const int key = ev.evValue;
				if ( key >= K_MOUSE1 && key <= K_MOUSE8 ) {
					const int button = key - K_MOUSE1;
					if ( button < HF_UI_MOUSE_COUNT ) {
						SetButton( button, ev.evValue2 != 0 );
					}
					return true;
				}
				// A pending rebind captures the next key press.  Escape cancels
				// so the options screen never becomes a keyboard trap.
				if ( m_pendingBindRow >= 0 ) {
					if ( ev.evValue2 == 0 ) { return true; }
					FinishKeybind( key );
					return true;
				}
				if ( ev.evValue2 != 0 ) {
					if ( key == K_ENTER || key == K_SPACE ) { ActivateFocused(); return true; }
					if ( key == K_ESCAPE ) { Back(); return true; }
					if ( m_bus != NULL ) {
						if ( key == K_UPARROW || key == K_JOY_DPAD_UP ) { m_input.MoveFocus( m_registry, m_ui, *m_bus, 0, -1 ); return true; }
						if ( key == K_DOWNARROW || key == K_JOY_DPAD_DOWN || key == K_TAB ) { m_input.MoveFocus( m_registry, m_ui, *m_bus, 0, 1 ); return true; }
						if ( key == K_LEFTARROW || key == K_JOY_DPAD_LEFT ) { m_input.MoveFocus( m_registry, m_ui, *m_bus, -1, 0 ); return true; }
						if ( key == K_RIGHTARROW || key == K_JOY_DPAD_RIGHT ) { m_input.MoveFocus( m_registry, m_ui, *m_bus, 1, 0 ); return true; }
					}
				}
				return false;
			}
			case SE_JOYSTICK: {
				// Win32 queues SE_JOYSTICK with a zero-based axis number and a
				// signed percent, not a J_* key code.  Only the left stick
				// navigates the UI; the right stick stays with the game.
				const int axis = ( ev.evValue == AXIS_LEFT_X ) ? 0
							   : ( ev.evValue == AXIS_LEFT_Y ) ? 1 : -1;
				if ( axis < 0 ) { return false; }
				if ( m_joyAxis[ axis ] != ev.evValue2 ) {
					m_joyRepeatCooldown = 0;
				}
				m_joyAxis[ axis ] = ev.evValue2;
				return ApplyJoystickFocus();
			}
			default:
				return false;
		}
	}

	void ActivateFocused() {
		const int action = FindBoundAction( m_input.GetFocused() );
		if ( action != HF_UI_ACTION_NONE ) {
			PerformAction( action );
			return;
		}
		const int row = FindKeybindRow( m_input.GetFocused() );
		if ( row >= 0 ) {
			BeginKeybind( row );
		}
	}

	// --- per-frame --------------------------------------------------------

	void Update( float dt ) {
		m_ui.Update( m_registry, dt );

		if ( m_joyRepeatCooldown > 0 ) {
			m_joyRepeatCooldown--;
		} else if ( m_state != HF_UI_FRONTEND_PLAYING ) {
			ApplyJoystickFocus();	// repeat a held stick at a fixed cadence
		}

		if ( m_bus != NULL ) {
			m_input.Update( m_registry, m_ui, *m_bus );
			m_input.EndFrame();
		}
		m_input.ApplyFocus( m_registry );

		// Mouse handlers only record the intent; apply it after input
		// dispatch so a transition never destroys entities mid-iteration.
		if ( m_pendingAction != HF_UI_ACTION_NONE ) {
			const int action = m_pendingAction;
			m_pendingAction = HF_UI_ACTION_NONE;
			PerformAction( action );
		}

		if ( m_state == HF_UI_FRONTEND_PLAYING && m_arena != NULL ) {
			static const float kTick = 1.0f / 60.0f;
			m_arenaAccum += ( dt > 0.25f ? 0.25f : dt );
			int steps = 0;
			while ( m_arenaAccum >= kTick && steps < 8 ) {
				m_arena->Step();
				m_arenaAccum -= kTick;
				steps++;
			}
			SyncHud();
			m_ui.Update( m_registry, 0.0f );
			if ( !m_arena->Running() ) {
				SetState( HF_UI_FRONTEND_RESULTS );
			}
		}
	}

	static const char *StateName( int state ) {
		switch ( state ) {
			case HF_UI_FRONTEND_BOOT:		return "boot";
			case HF_UI_FRONTEND_TITLE:		return "title";
			case HF_UI_FRONTEND_MENU:		return "menu";
			case HF_UI_FRONTEND_LOADING:	return "loading";
			case HF_UI_FRONTEND_PLAYING:	return "playing";
			case HF_UI_FRONTEND_PAUSED:		return "paused";
			case HF_UI_FRONTEND_RESULTS:	return "results";
			case HF_UI_FRONTEND_SAVES:		return "saves";
			default:						return "?";
		}
	}

private:
	struct BoundAction {
		HF_Ecs::Entity	entity;
		int				action;
	};

	static float Clamp( float v, float lo, float hi ) {
		return v < lo ? lo : ( v > hi ? hi : v );
	}

	static void OnUiEvent( const char *name, const void *data, int size, void *user ) {
		( ( hfUiFrontend * )user )->HandleEvent( name, data, size );
	}

	void HandleEvent( const char *name, const void *data, int size ) {
		if ( data == NULL ) {
			return;
		}
		if ( hfStr::Icmp( name, "ui.click" ) == 0 ) {
			if ( size < ( int )sizeof( hfUiEventClick ) ) { return; }
			const hfUiEventClick *ev = ( const hfUiEventClick * )data;
			const int action = FindBoundAction( ev->entity );
			if ( action != HF_UI_ACTION_NONE ) {
				m_pendingAction = action;
				return;
			}
			const int row = FindKeybindRow( ev->entity );
			if ( row >= 0 ) {
				BeginKeybind( row );
				return;
			}
			if ( m_state == HF_UI_FRONTEND_SAVES ) {
				int slot = -1;
				int kind = -1;
				if ( m_saveSlots.Resolve( ev->entity, slot, kind ) ) {
					m_lastSaveSlot = slot;
					m_lastSaveKind = kind;
					if ( kind == hfUiSaveSlots::HF_UI_SAVE_KIND_LOAD ) {
						m_pendingAction = HF_UI_ACTION_SAVE_SLOT_LOAD;
					} else if ( kind == hfUiSaveSlots::HF_UI_SAVE_KIND_SAVE ) {
						m_pendingAction = HF_UI_ACTION_SAVE_SLOT_WRITE;
					} else if ( kind == hfUiSaveSlots::HF_UI_SAVE_KIND_DELETE ) {
						m_pendingAction = HF_UI_ACTION_SAVE_SLOT_DELETE;
					}
				}
			}
			return;
		}
		if ( hfStr::Icmp( name, "ui.change" ) == 0 ) {
			if ( size < ( int )sizeof( hfUiEventChange ) ) { return; }
			const hfUiEventChange *ev = ( const hfUiEventChange * )data;
			if ( ev->entity == m_musicSlider ) {
				m_musicVolume = ev->valueF;
			} else if ( ev->entity == m_sfxSlider ) {
				m_sfxVolume = ev->valueF;
			} else if ( ev->entity == m_fullscreenToggle ) {
				m_fullscreen = ev->valueI != 0;
			}
		}
	}

	// Held-stick navigation.  The cooldown is frame-counted rather than
	// wall-clock based so a fixed input sequence produces the same focus path
	// in a headless test as it does in the windowed engine.
	bool ApplyJoystickFocus() {
		if ( m_bus == NULL || m_joyRepeatCooldown > 0 ) {
			return true;
		}
		static const int DEADZONE = 40;
		const int ax = m_joyAxis[ 0 ] < 0 ? -m_joyAxis[ 0 ] : m_joyAxis[ 0 ];
		const int ay = m_joyAxis[ 1 ] < 0 ? -m_joyAxis[ 1 ] : m_joyAxis[ 1 ];
		int dx = 0;
		int dy = 0;
		if ( ax >= ay && ax > DEADZONE ) {
			dx = ( m_joyAxis[ 0 ] > 0 ) ? 1 : -1;
		} else if ( ay > DEADZONE ) {
			dy = ( m_joyAxis[ 1 ] > 0 ) ? 1 : -1;
		}
		if ( dx != 0 || dy != 0 ) {
			if ( m_input.MoveFocus( m_registry, m_ui, *m_bus, dx, dy ) ) {
				m_joyRepeatCooldown = 12;
			}
		}
		return true;
	}

	int FindBoundAction( HF_Ecs::Entity entity ) const {
		if ( entity == HF_Ecs::ENTITY_NULL ) { return HF_UI_ACTION_NONE; }
		for ( int i = 0; i < m_bindCount; i++ ) {
			if ( m_binds[ i ].entity == entity ) { return m_binds[ i ].action; }
		}
		return HF_UI_ACTION_NONE;
	}

	// --- keybind row helpers ----------------------------------------------

	int FindKeybindRow( HF_Ecs::Entity entity ) const {
		if ( entity == HF_Ecs::ENTITY_NULL ) { return -1; }
		for ( int i = 0; i < m_keybindRowCount; i++ ) {
			if ( m_keybindRows[ i ].entity == entity ) { return i; }
		}
		return -1;
	}

	// Local names keep the UI header self-contained (the harness does not
	// link the engine's key-input translation unit).
	static const char * KeyName( int key ) {
		if ( key <= 0 ) { return "UNBOUND"; }
		struct NameEntry { int key; const char *name; };
		static const NameEntry names[] = {
			{ K_ESCAPE, "ESC" }, { K_ENTER, "ENTER" }, { K_SPACE, "SPACE" },
			{ K_TAB, "TAB" }, { K_BACKSPACE, "BKSP" },
			{ K_UPARROW, "UP" }, { K_DOWNARROW, "DOWN" },
			{ K_LEFTARROW, "LEFT" }, { K_RIGHTARROW, "RIGHT" },
			{ K_JOY_DPAD_UP, "PAD UP" }, { K_JOY_DPAD_DOWN, "PAD DOWN" },
			{ K_JOY_DPAD_LEFT, "PAD LEFT" }, { K_JOY_DPAD_RIGHT, "PAD RIGHT" },
			{ K_1, "1" }, { K_2, "2" }, { K_3, "3" }, { K_4, "4" }, { K_5, "5" },
			{ K_6, "6" }, { K_7, "7" }, { K_8, "8" }, { K_9, "9" }, { K_0, "0" },
			{ K_Q, "Q" }, { K_W, "W" }, { K_E, "E" }, { K_R, "R" }, { K_T, "T" },
			{ K_Y, "Y" }, { K_U, "U" }, { K_I, "I" }, { K_O, "O" }, { K_P, "P" },
			{ K_A, "A" }, { K_S, "S" }, { K_D, "D" }, { K_F, "F" }, { K_G, "G" },
			{ K_H, "H" }, { K_J, "J" }, { K_K, "K" }, { K_L, "L" },
			{ K_Z, "Z" }, { K_X, "X" }, { K_C, "C" }, { K_V, "V" },
			{ K_B, "B" }, { K_N, "N" }, { K_M, "M" }
		};
		for ( int i = 0; i < ( int )( sizeof( names ) / sizeof( names[ 0 ] ) ); i++ ) {
			if ( names[ i ].key == key ) { return names[ i ].name; }
		}
		static char fallback[ 32 ];
		snprintf( fallback, sizeof( fallback ), "KEY %d", key );
		return fallback;
	}

	void SetKeybindStatus( const char *text ) {
		if ( m_keybindStatus != HF_Ecs::ENTITY_NULL ) { SetLabelText( m_keybindStatus, text ); }
	}

	HF_Ecs::Entity FindChildLabel( HF_Ecs::Entity parent ) {
		if ( parent == HF_Ecs::ENTITY_NULL ) { return HF_Ecs::ENTITY_NULL; }
		HF_Ecs::Entity found = HF_Ecs::ENTITY_NULL;
		m_registry.Each< hfUiLabelComponent >( [&]( HF_Ecs::Entity ent, hfUiLabelComponent & ) {
			if ( found != HF_Ecs::ENTITY_NULL ) { return; }
			const hfUiCanvasComponent *c = m_registry.GetComponent< hfUiCanvasComponent >( ent );
			if ( c != NULL && c->parent == parent ) { found = ent; }
		} );
		return found;
	}

	void BeginKeybind( int row ) {
		if ( row < 0 || row >= m_keybindRowCount ) { return; }
		m_pendingBindRow = row;
		SetKeybindStatus( "PRESS A KEY (ESC TO CANCEL)" );
	}

	void FinishKeybind( int key ) {
		const int row = m_pendingBindRow;
		m_pendingBindRow = -1;
		if ( row < 0 || row >= m_keybindRowCount ) { return; }
		const int index = m_keybindRows[ row ].index;
		const char *action = m_keybinds.ActionAt( index );

		if ( key == K_ESCAPE ) {
			SetKeybindStatus( "BIND CANCELLED" );
			return;
		}
		if ( action == NULL || !m_keybinds.Bind( action, key ) ) {
			const char *conflict = m_keybinds.GetLastConflictAction();
			char buf[ 128 ];
			snprintf( buf, sizeof( buf ), "ALREADY BOUND TO %s", conflict != NULL ? conflict : "?" );
			SetKeybindStatus( buf );
			return;
		}

		char buf[ 128 ];
		snprintf( buf, sizeof( buf ), "%s = %s", action, KeyName( key ) );
		SetKeybindStatus( buf );
		const HF_Ecs::Entity button = m_keybindRows[ row ].entity;
		const HF_Ecs::Entity label = FindChildLabel( button );
		if ( label != HF_Ecs::ENTITY_NULL ) { SetLabelText( label, KeyName( key ) ); }
	}

	void Bind( HF_Ecs::Entity entity, int action ) {
		if ( m_bindCount < ( int )( sizeof( m_binds ) / sizeof( m_binds[ 0 ] ) ) ) {
			m_binds[ m_bindCount ].entity = entity;
			m_binds[ m_bindCount ].action = action;
			m_bindCount++;
		}
	}

	void LogAction( int action ) {
		if ( m_logCount < ( int )( sizeof( m_log ) / sizeof( m_log[ 0 ] ) ) ) {
			m_log[ m_logCount++ ] = action;
		}
	}

	// --- construction -----------------------------------------------------

	HF_Ecs::Entity MakeRoot( float r, float g, float b, float a ) {
		HF_Ecs::Entity root = hfUiBuilder::CreateRoot( m_registry, m_width, m_height );
		hfUiPanelComponent &p = m_registry.AddComponent< hfUiPanelComponent >( root );
		p.r = r; p.g = g; p.b = b; p.a = a;
		m_root = root;
		return root;
	}

	HF_Ecs::Entity AddLabel( HF_Ecs::Entity parent, float x, float y, float w, float h,
							 const char *text, float scale, int align,
							 float r = 1.0f, float g = 1.0f, float b = 1.0f ) {
		HF_Ecs::Entity e = hfUiBuilder::CreateLabel( m_registry, parent, x, y, w, h, text, scale, r, g, b, 1.0f );
		hfUiLabelComponent *lab = m_registry.GetComponent< hfUiLabelComponent >( e );
		if ( lab != NULL ) { lab->align = align; }
		return e;
	}

	HF_Ecs::Entity AddButton( HF_Ecs::Entity parent, float x, float y, float w, float h,
							  const char *text, int action ) {
		HF_Ecs::Entity e = hfUiBuilder::CreateButton( m_registry, parent, x, y, w, h, text );
		Bind( e, action );
		return e;
	}

	void BuildState( hfUiFrontendState state ) {
		switch ( state ) {
			case HF_UI_FRONTEND_BOOT:
				break;
			case HF_UI_FRONTEND_TITLE:
				BuildTitle();
				break;
			case HF_UI_FRONTEND_MENU:
				BuildMenu();
				break;
			case HF_UI_FRONTEND_LOADING:
				BuildLoading();
				break;
			case HF_UI_FRONTEND_PLAYING:
				BuildPlaying();
				break;
			case HF_UI_FRONTEND_PAUSED:
				BuildPaused();
				break;
			case HF_UI_FRONTEND_RESULTS:
				BuildResults();
				break;
			case HF_UI_FRONTEND_SAVES:
				BuildSaves();
				break;
			default:
				break;
		}
	}

	void BuildTitle() {
		HF_Ecs::Entity root = MakeRoot( 0.06f, 0.07f, 0.10f, 1.0f );
		const float bx = ( m_width - 300.0f ) * 0.5f;

		AddLabel( root, 0.0f, 110.0f, m_width, 64.0f, "HELLFORGE ARENA", 3.0f,
				  HF_UI_TEXT_CENTER, 0.95f, 0.55f, 0.15f );
		AddLabel( root, 0.0f, 180.0f, m_width, 24.0f, "A DETERMINISTIC HFSCRIPT ARENA", 1.0f,
				  HF_UI_TEXT_CENTER, 0.55f, 0.58f, 0.65f );

		AddButton( root, bx, 280.0f, 300.0f, 46.0f, "PLAY", HF_UI_ACTION_START );
		AddButton( root, bx, 340.0f, 300.0f, 46.0f, "OPTIONS", HF_UI_ACTION_OPTIONS );
		AddButton( root, bx, 400.0f, 300.0f, 46.0f, "SAVES", HF_UI_ACTION_SAVES );
		AddButton( root, bx, 460.0f, 300.0f, 46.0f, "QUIT", HF_UI_ACTION_QUIT );

		AddLabel( root, 0.0f, m_height - 40.0f, m_width, 20.0f,
				  "PHASE 16 FRONTEND", 1.0f, HF_UI_TEXT_CENTER, 0.35f, 0.38f, 0.45f );
	}

	void BuildMenu() {
		HF_Ecs::Entity root = MakeRoot( 0.06f, 0.07f, 0.10f, 1.0f );
		const float labelX = m_width * 0.24f;
		const float controlX = m_width * 0.42f;
		const float controlW = m_width * 0.32f;

		AddLabel( root, 0.0f, 70.0f, m_width, 44.0f, "OPTIONS", 2.0f,
				  HF_UI_TEXT_CENTER, 0.95f, 0.55f, 0.15f );

		AddLabel( root, labelX, 170.0f, 160.0f, 28.0f, "MUSIC", 1.5f, HF_UI_TEXT_LEFT );
		m_musicSlider = hfUiBuilder::CreateSlider( m_registry, root, controlX, 168.0f, controlW, 28.0f,
												   0.0f, 100.0f, m_musicVolume );

		AddLabel( root, labelX, 214.0f, 160.0f, 28.0f, "SOUND FX", 1.5f, HF_UI_TEXT_LEFT );
		m_sfxSlider = hfUiBuilder::CreateSlider( m_registry, root, controlX, 212.0f, controlW, 28.0f,
												 0.0f, 100.0f, m_sfxVolume );

		AddLabel( root, labelX, 258.0f, 160.0f, 28.0f, "FULLSCREEN", 1.5f, HF_UI_TEXT_LEFT );
		m_fullscreenToggle = hfUiBuilder::CreateToggle( m_registry, root, controlX, 256.0f,
														controlW, 28.0f, "ENABLED", m_fullscreen );

		AddLabel( root, 0.0f, 300.0f, m_width, 24.0f, "KEY BINDINGS", 1.25f,
				  HF_UI_TEXT_CENTER, 0.95f, 0.55f, 0.15f );

		m_keybindRowCount = 0;
		const int rows = m_keybinds.Count() < 5 ? m_keybinds.Count() : 5;
		for ( int i = 0; i < rows; i++ ) {
			const float y = 338.0f + ( float )i * 31.0f;
			const char *action = m_keybinds.ActionAt( i );
			AddLabel( root, labelX, y, 160.0f, 24.0f, action != NULL ? action : "?", 1.0f, HF_UI_TEXT_LEFT );
			char buf[ 64 ];
			snprintf( buf, sizeof( buf ), "%s", KeyName( m_keybinds.Primary( i ) ) );
			const HF_Ecs::Entity button = AddButton( root, controlX, y - 2.0f, controlW, 26.0f,
													 buf, HF_UI_ACTION_NONE );
			m_keybindRows[ m_keybindRowCount ].entity = button;
			m_keybindRows[ m_keybindRowCount ].index = i;
			m_keybindRowCount++;
		}

		m_keybindStatus = AddLabel( root, 0.0f, 510.0f, m_width, 24.0f,
									"CLICK A BINDING, THEN PRESS A KEY", 1.0f,
									HF_UI_TEXT_CENTER, 0.55f, 0.58f, 0.65f );

		AddButton( root, ( m_width - 300.0f ) * 0.5f, 560.0f, 300.0f, 46.0f, "BACK", HF_UI_ACTION_BACK );
	}

	void BuildLoading() {
		HF_Ecs::Entity root = MakeRoot( 0.02f, 0.02f, 0.03f, 1.0f );
		AddLabel( root, 0.0f, m_height * 0.45f, m_width, 30.0f, "LOADING...", 2.0f,
				  HF_UI_TEXT_CENTER, 0.9f, 0.9f, 0.9f );
	}

	void BuildPlaying() {
		HF_Ecs::Entity root = MakeRoot( 0.0f, 0.0f, 0.0f, 0.0f );

		m_hud[ HF_UI_HUD_SCORE ]  = AddLabel( root, 16.0f, 14.0f, 300.0f, 24.0f, "SCORE 0", 1.5f,
											  HF_UI_TEXT_LEFT, 1.0f, 0.85f, 0.4f );
		m_hud[ HF_UI_HUD_HEALTH ] = AddLabel( root, m_width * 0.5f - 100.0f, 14.0f, 200.0f, 24.0f, "HP 100", 1.5f,
											  HF_UI_TEXT_CENTER, 0.95f, 0.35f, 0.35f );
		m_hud[ HF_UI_HUD_WAVE ]   = AddLabel( root, 16.0f, m_height - 42.0f, 300.0f, 24.0f, "WAVE 1/5", 1.25f,
											  HF_UI_TEXT_LEFT, 0.5f, 0.75f, 1.0f );

		AddButton( root, m_width - 150.0f, 14.0f, 134.0f, 34.0f, "PAUSE", HF_UI_ACTION_PAUSE );
	}

	void BuildPaused() {
		HF_Ecs::Entity root = MakeRoot( 0.0f, 0.0f, 0.0f, 0.65f );
		const float bx = ( m_width - 300.0f ) * 0.5f;

		AddLabel( root, 0.0f, 120.0f, m_width, 44.0f, "PAUSED", 2.5f,
				  HF_UI_TEXT_CENTER, 0.95f, 0.55f, 0.15f );

		AddButton( root, bx, 240.0f, 300.0f, 46.0f, "RESUME", HF_UI_ACTION_RESUME );
		AddButton( root, bx, 300.0f, 300.0f, 46.0f, "RESTART", HF_UI_ACTION_RESTART );
		AddButton( root, bx, 360.0f, 300.0f, 46.0f, "MAIN MENU", HF_UI_ACTION_MAIN_MENU );
		AddButton( root, bx, 420.0f, 300.0f, 46.0f, "QUIT", HF_UI_ACTION_QUIT );
	}

	void BuildResults() {
		HF_Ecs::Entity root = MakeRoot( 0.05f, 0.06f, 0.09f, 1.0f );
		const float bx = ( m_width - 300.0f ) * 0.5f;

		const bool won = ( m_arena != NULL ) ? m_arena->won : true;
		m_resultLabel = AddLabel( root, 0.0f, 120.0f, m_width, 52.0f,
								  won ? "VICTORY" : "GAME OVER", 3.0f,
								  HF_UI_TEXT_CENTER,
								  won ? 0.3f : 0.95f, won ? 0.9f : 0.3f, won ? 0.4f : 0.3f );

		char score[ 64 ];
		snprintf( score, sizeof( score ), "SCORE %d", m_arena != NULL ? m_arena->score : 0 );
		AddLabel( root, 0.0f, 190.0f, m_width, 28.0f, score, 1.75f,
				  HF_UI_TEXT_CENTER, 0.8f, 0.8f, 0.85f );

		AddButton( root, bx, 290.0f, 300.0f, 46.0f, "PLAY AGAIN", HF_UI_ACTION_START );
		AddButton( root, bx, 350.0f, 300.0f, 46.0f, "MAIN MENU", HF_UI_ACTION_MAIN_MENU );
		AddButton( root, bx, 410.0f, 300.0f, 46.0f, "QUIT", HF_UI_ACTION_QUIT );
	}

	void BuildSaves() {
		HF_Ecs::Entity root = MakeRoot( 0.06f, 0.07f, 0.10f, 1.0f );
		AddLabel( root, 0.0f, 90.0f, m_width, 44.0f, "SAVES", 2.0f,
				  HF_UI_TEXT_CENTER, 0.95f, 0.55f, 0.15f );

		hfconsole::hfConsoleServices *svc = hfconsole::HfConsole_GetServices();
		hfconsole::hfConsoleUserId user = svc ? svc->GetUserId( 0 ) : hfconsole::hfConsoleUserId( 0 );
		m_saveSlots.Build( m_registry, m_ui, root, svc, user,
						   m_width * 0.5f - 250.0f, 160.0f, 500.0f, 320.0f, 3 );

		AddButton( root, ( m_width - 300.0f ) * 0.5f, 500.0f, 300.0f, 46.0f, "BACK", HF_UI_ACTION_BACK );
	}

	void SyncHud() {
		if ( m_hud[ HF_UI_HUD_SCORE ] == HF_Ecs::ENTITY_NULL ) {
			return;
		}
		char buf[ 64 ];
		const int score = ( m_arena != NULL ) ? m_arena->score : 0;
		const int hp = ( m_arena != NULL ) ? m_arena->hitPoints : HfArena::PLAYER_HITPOINTS;
		const int wave = ( m_arena != NULL ) ? m_arena->wave : 1;

		snprintf( buf, sizeof( buf ), "SCORE %d", score );
		SetLabelText( m_hud[ HF_UI_HUD_SCORE ], buf );
		snprintf( buf, sizeof( buf ), "HP %d", hp );
		SetLabelText( m_hud[ HF_UI_HUD_HEALTH ], buf );
		snprintf( buf, sizeof( buf ), "WAVE %d/%d", wave, HfArena::TOTAL_WAVES );
		SetLabelText( m_hud[ HF_UI_HUD_WAVE ], buf );
	}

	void SetLabelText( HF_Ecs::Entity entity, const char *text ) {
		hfUiLabelComponent *lab = m_registry.GetComponent< hfUiLabelComponent >( entity );
		if ( lab != NULL ) { lab->SetText( text ); }
	}

	// --- state ------------------------------------------------------------
	HF_Ecs::Registry	m_registry;
	hfUiSystem			m_ui;
	hfUiInput			m_input;
	HF_Ecs::EventBus *	m_bus;
	HfArena *			m_arena;
	hfUiFrontendState	m_state;
	int					m_pendingAction;
	float				m_width;
	float				m_height;
	int					m_seed;
	float				m_arenaAccum;
	bool				m_quitRequested;
	bool				m_hostHandled;
	int					m_joyRepeatCooldown;
	int					m_joyAxis[ 2 ];
	float				m_mouseX = 0.0f;
	float				m_mouseY = 0.0f;

	// Options screen state (persisted by the host in Phase 17).
	float				m_musicVolume;
	float				m_sfxVolume;
	bool				m_fullscreen;

	hfUiTheme			m_theme;

	HF_Ecs::Entity		m_root;
	HF_Ecs::Entity		m_hud[ HF_UI_HUD_COUNT ];
	HF_Ecs::Entity		m_musicSlider;
	HF_Ecs::Entity		m_sfxSlider;
	HF_Ecs::Entity		m_fullscreenToggle;
	HF_Ecs::Entity		m_resultLabel;
	hfUiSaveSlots		m_saveSlots;
	int					m_lastSaveSlot;
	int					m_lastSaveKind;

	hfUiKeybinds		m_keybinds;
	struct KeybindRow {
		HF_Ecs::Entity	entity;
		int				index;
	};
	KeybindRow			m_keybindRows[ 8 ];
	int					m_keybindRowCount;
	int					m_pendingBindRow;
	HF_Ecs::Entity		m_keybindStatus;

	BoundAction			m_binds[ 32 ];
	int					m_bindCount;
	int					m_log[ 128 ];
	int					m_logCount;

	hfUiActionHandler	m_actionHandler;
	void *				m_actionUser;
};

} // namespace HF_Ui

#endif
