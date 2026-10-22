#pragma hdrstop
#include "../hflib/hfprecompiled.h"

#include "hfDeviceContext.h"
#include "hfWindow.h"
#include "hfUserInterfaceLocal.h"
#include "hfSliderWindow.h"
#include "hfEditWindow.h"


bool hfEditWindow::ParseInternalVar( const char *_name, hfTokenParser *src ) {
	if ( hfStr::Icmp( _name, "maxchars" ) == 0) {
		maxChars = src->ParseInt();
		return true;
	}
	if ( hfStr::Icmp( _name, "numeric" ) == 0) {
		numeric = src->ParseBool();
		return true;
	}
	if ( hfStr::Icmp( _name, "wrap" ) == 0) {
		wrap = src->ParseBool();
		return true;
	}
	if ( hfStr::Icmp( _name, "readonly" ) == 0) {
		readonly = src->ParseBool();
		return true;
	}
	if ( hfStr::Icmp( _name, "forceScroll" ) == 0) {
		forceScroll = src->ParseBool();
		return true;
	}
	if ( hfStr::Icmp( _name, "source" ) == 0) {
		ParseString( src, sourceFile );
		return true;
	}
	if ( hfStr::Icmp( _name, "password" ) == 0 ) { 
		password = src->ParseBool();
		return true;
	}
	if ( hfStr::Icmp( _name, "cvarMax" ) == 0) {
		cvarMax = src->ParseInt();
		return true;
	}

	return hfWindow::ParseInternalVar( _name, src );
}

hfWinVar *hfEditWindow::GetWinVarByName( const char *_name, bool fixup, drawWin_t** owner ) {
	if ( hfStr::Icmp( _name, "cvar" ) == 0 ) {
		return &cvarStr;
	}
	if ( hfStr::Icmp( _name, "password" ) == 0 ) {
		return &password;
	}
	if ( hfStr::Icmp( _name, "liveUpdate" ) == 0 ) {
		return &liveUpdate;
	}
	if ( hfStr::Icmp( _name, "cvarGroup" ) == 0 ) {
		return &cvarGroup;
	}
	return hfWindow::GetWinVarByName( _name, fixup, owner );
}

void hfEditWindow::CommonInit() {
	maxChars = 128;
	numeric = false;
	paintOffset = 0;
	cursorPos = 0;
	cursorLine = 0;
	cvarMax = 0;
	wrap = false;
	sourceFile = "";
	scroller = NULL;
	sizeBias = 0;
	lastTextLength = 0;
	forceScroll = false;
	password = NULL;
	cvar = NULL;
	liveUpdate = true;
	readonly = false;

	scroller = new (TAG_OLD_UI) hfSliderWindow(gui);
}


hfEditWindow::hfEditWindow( hfUserInterfaceLocal *g ) : hfWindow(g) {
	gui = g;
	CommonInit();
}

hfEditWindow::~hfEditWindow() {

}

void hfEditWindow::GainFocus() {
	cursorPos = text.Length();
	EnsureCursorVisible();
}

void hfEditWindow::Draw( int time, float x, float y ) {
	hfVec4 color = foreColor;

	UpdateCvar( true );

	int len = text.Length();
	if ( len != lastTextLength ) {
		scroller->SetValue( 0.0f );
		EnsureCursorVisible();
		lastTextLength = len;
	}
	float scale = textScale;

	hfStr		pass;
	const char* buffer;
	if ( password ) {		
		const char* temp = text;
		for ( ; *temp; temp++ )	{
			pass += "*";
		}
		buffer = pass;
	} else {
		buffer = text;
	}

	if ( cursorPos > len ) {
		cursorPos = len;
	}

	hfRectangle rect = textRect;

	rect.x -= paintOffset;
	rect.w += paintOffset;

	if ( wrap && scroller->GetHigh() > 0.0f ) {
		float lineHeight = GetMaxCharHeight( ) + 5;
		rect.y -= scroller->GetValue() * lineHeight;
		rect.w -= sizeBias;
		rect.h = ( breaks.Num() + 1 ) * lineHeight;
	}

	if ( hover && !noEvents && Contains(gui->CursorX(), gui->CursorY()) ) {
		color = hoverColor;
	} else {
		hover = false;
	}
	if ( flags & WIN_FOCUS ) {
		color = hoverColor;
	}

	dc->DrawText( buffer, scale, 0, color, rect, wrap, (flags & WIN_FOCUS) ? cursorPos : -1);
}

/*
=============
hfEditWindow::HandleEvent
=============
*/
const char *hfEditWindow::HandleEvent(const sysEvent_t *event, bool *updateVisuals) {
	static char buffer[ MAX_EDITFIELD ];

	if ( wrap ) {
		// need to call this to allow proper focus and capturing on embedded children
		const char * ret = hfWindow::HandleEvent( event, updateVisuals );
		if ( ret != NULL && *ret != NULL ) {
			return ret;
		}
	}

	if ( ( event->evType != SE_CHAR && event->evType != SE_KEY ) ) {
		return "";
	}

	hfStr::Copynz( buffer, text.c_str(), sizeof( buffer ) );
	int key = event->evValue;
	int len = text.Length();

	if ( event->evType == SE_CHAR ) {
		if ( key == '`' ) {
			return "";
		}

		if ( updateVisuals ) {
			*updateVisuals = true;
		}

		if ( maxChars && len > maxChars ) {
			len = maxChars;
		}

		if ( readonly ) {
			return "";
		}

   		//
   		// ignore any non printable chars (except enter when wrap is enabled)
   		//
		if ( wrap && (key == K_ENTER || key == K_KP_ENTER) ) {
		} else if ( !hfStr::CharIsPrintable( key ) ) {
			return "";
		}

		if ( numeric ) {
			if ( ( key < '0' || key > '9' ) && key != '.' ) {
	       		return "";
			}
		}

		if ( dc->GetOverStrike() ) {
			if ( maxChars && cursorPos >= maxChars ) {
	       		return "";
			}
		} else {
			if ( ( len == MAX_EDITFIELD - 1 ) || ( maxChars && len >= maxChars ) ) {
	       		return "";
			}
			memmove( &buffer[ cursorPos + 1 ], &buffer[ cursorPos ], len + 1 - cursorPos );
		}

		buffer[ cursorPos ] = key;

		text = buffer;
		UpdateCvar( false );
		RunScript( ON_ACTION );

		if ( cursorPos < len + 1 ) {
			cursorPos++;
		}
		EnsureCursorVisible();

	} else if ( event->evType == SE_KEY && event->evValue2 ) {

		if ( updateVisuals ) {
			*updateVisuals = true;
		}

		if ( key == K_DEL ) {
			if ( readonly ) {
				return "";
			}
			if ( cursorPos < len ) {
				memmove( &buffer[cursorPos], &buffer[cursorPos + 1], len - cursorPos);
				text = buffer;
				UpdateCvar( false );
				RunScript( ON_ACTION );
			}
			return "";
		}

		if ( key == K_BACKSPACE ) {	// ctrl-h is backspace
			if ( readonly ) {
				return "";
			}
   			if ( cursorPos > 0 ) {
				if ( cursorPos >= len ) {
					buffer[len - 1] = 0;
					cursorPos = len - 1;
				} else {
					memmove( &buffer[ cursorPos - 1 ], &buffer[ cursorPos ], len + 1 - cursorPos);
					cursorPos--;
				}

				text = buffer;
				UpdateCvar( false );
				RunScript( ON_ACTION );
			}

			return "";
   		}
		if ( key == K_RIGHTARROW )  {
			if ( cursorPos < len ) {
				if ( ( hfKeyInput::IsDown( K_LCTRL ) || hfKeyInput::IsDown( K_RCTRL ) ) ) {
					// skip to next word
					while( ( cursorPos < len ) && ( buffer[ cursorPos ] != ' ' ) ) {
						cursorPos++;
					}

					while( ( cursorPos < len ) && ( buffer[ cursorPos ] == ' ' ) ) {
						cursorPos++;
					}
				} else {
					if ( cursorPos < len ) {
						cursorPos++;
					}
				}
			} 

			EnsureCursorVisible();

			return "";
		}

		if ( key == K_LEFTARROW ) {
			if ( ( hfKeyInput::IsDown( K_LCTRL ) || hfKeyInput::IsDown( K_RCTRL ) ) ) {
				// skip to previous word
				while( ( cursorPos > 0 ) && ( buffer[ cursorPos - 1 ] == ' ' ) ) {
					cursorPos--;
				}

				while( ( cursorPos > 0 ) && ( buffer[ cursorPos - 1 ] != ' ' ) ) {
					cursorPos--;
				}
			} else {
				if ( cursorPos > 0 ) {
					cursorPos--;
				}
			}

			EnsureCursorVisible();

			return "";
		}

		if ( key == K_HOME ) {
			if ( ( hfKeyInput::IsDown( K_LCTRL ) || hfKeyInput::IsDown( K_RCTRL ) ) || cursorLine <= 0 || ( cursorLine >= breaks.Num() ) ) {
                cursorPos = 0;
			} else {
				cursorPos = breaks[cursorLine];
			}
			EnsureCursorVisible();
			return "";
		}

		if ( key == K_END )  {
			if ( ( hfKeyInput::IsDown( K_LCTRL ) || hfKeyInput::IsDown( K_RCTRL ) ) || (cursorLine < -1) || ( cursorLine >= breaks.Num() - 1 ) ) {
				cursorPos = len;
			} else {
				cursorPos = breaks[cursorLine + 1] - 1;
			}
			EnsureCursorVisible();
			return "";
		}

		if ( key == K_INS ) {
			if ( !readonly ) {
				dc->SetOverStrike( !dc->GetOverStrike() );
			}
			return "";
		}

		if ( key == K_DOWNARROW ) {
			if ( ( hfKeyInput::IsDown( K_LCTRL ) || hfKeyInput::IsDown( K_RCTRL ) ) ) {
				scroller->SetValue( scroller->GetValue() + 1.0f );
			} else {
				if ( cursorLine < breaks.Num() - 1 ) {
					int offset = cursorPos - breaks[cursorLine];
					cursorPos = breaks[cursorLine + 1] + offset;
					EnsureCursorVisible();
				}
			}
		}

		if (key == K_UPARROW ) {
			if ( ( hfKeyInput::IsDown( K_LCTRL ) || hfKeyInput::IsDown( K_RCTRL ) ) ) {
				scroller->SetValue( scroller->GetValue() - 1.0f );
			} else {
				if ( cursorLine > 0 ) {
					int offset = cursorPos - breaks[cursorLine];
					cursorPos = breaks[cursorLine - 1] + offset;
					EnsureCursorVisible();
				}
			}
		}

		if ( key == K_ENTER || key == K_KP_ENTER ) {
			RunScript( ON_ACTION );
			RunScript( ON_ENTER );
			return cmd;
		}

		if ( key == K_ESCAPE ) {
			RunScript( ON_ESC );
			return cmd;
		}

	} else if ( event->evType == SE_KEY && !event->evValue2 ) {
		if ( key == K_ENTER || key == K_KP_ENTER ) {
			RunScript( ON_ENTERRELEASE );
			return cmd;
		} else {
			RunScript( ON_ACTIONRELEASE );
		}
	}

	return "";
}

void hfEditWindow::PostParse() {
	hfWindow::PostParse();

	if ( maxChars == 0 ) {
		maxChars = 10;
	}
	if ( sourceFile.Length() ) {
		void *buffer;
		fileSystem->ReadFile( sourceFile, &buffer );
		text = (char *) buffer;
		fileSystem->FreeFile( buffer );
	}

	InitCvar();
	InitScroller(false);

	EnsureCursorVisible();

	flags |= WIN_CANFOCUS;
}

/*
================
hfEditWindow::InitScroller

This is the same as in hfListWindow
================
*/
void hfEditWindow::InitScroller( bool horizontal )
{
	const char *thumbImage = "guis/assets/scrollbar_thumb.tga";
	const char *barImage = "guis/assets/scrollbarv.tga";
	const char *scrollerName = "_scrollerWinV";

	if (horizontal) {
		barImage = "guis/assets/scrollbarh.tga";
		scrollerName = "_scrollerWinH";
	}

	const hfMaterial *mat = declManager->FindMaterial( barImage );
	mat->SetSort( SS_GUI );
	sizeBias = mat->GetImageWidth();

	hfRectangle scrollRect;
	if (horizontal) {
		sizeBias = mat->GetImageHeight();
		scrollRect.x = 0;
		scrollRect.y = (clientRect.h - sizeBias);
		scrollRect.w = clientRect.w;
		scrollRect.h = sizeBias;
	} else {
		scrollRect.x = (clientRect.w - sizeBias);
		scrollRect.y = 0;
		scrollRect.w = sizeBias;
		scrollRect.h = clientRect.h;
	}

	scroller->InitWithDefaults(scrollerName, scrollRect, foreColor, matColor, mat->GetName(), thumbImage, !horizontal, true);
	InsertChild(scroller, NULL);
	scroller->SetBuddy(this);
}

void hfEditWindow::HandleBuddyUpdate( hfWindow *buddy ) {
}

void hfEditWindow::EnsureCursorVisible()
{
	if ( readonly ) {
		cursorPos = -1;
	} else if ( maxChars == 1 ) {
		cursorPos = 0;
	}

	if ( !dc ) {
		return;
	}

	SetFont();
	if ( !wrap ) {
		int cursorX = 0;
		if ( password ) {
			cursorX = cursorPos * dc->CharWidth( '*', textScale );
		} else {
			int i = 0;
			while ( i < text.Length() && i < cursorPos ) {
				if ( hfStr::IsColor( &text[i] ) ) {
					i += 2;
				} else {
					cursorX += dc->CharWidth( text[i], textScale );
					i++;
				}
			}
		}
		int maxWidth = GetMaxCharWidth( );
		int left = cursorX - maxWidth;
		int right = ( cursorX - textRect.w ) + maxWidth;

		if ( paintOffset > left ) {
			// When we go past the left side, we want the text to jump 6 characters
			paintOffset = left - maxWidth * 6;
		}
		if ( paintOffset <  right) {
			paintOffset = right;
		}
		if ( paintOffset < 0 ) {
			paintOffset = 0;
		}
		scroller->SetRange(0.0f, 0.0f, 1.0f);

	} else {
		// Word wrap

		breaks.Clear();
		hfRectangle rect = textRect;
		rect.w -= sizeBias;
		dc->DrawText(text, textScale, textAlign, colorWhite, rect, true, (flags & WIN_FOCUS) ? cursorPos : -1, true, &breaks );

		int fit = textRect.h / (GetMaxCharHeight() + 5);
		if ( fit < breaks.Num() + 1 ) {
			scroller->SetRange(0, breaks.Num() + 1 - fit, 1);
		} else {
			// The text fits completely in the box
			scroller->SetRange(0.0f, 0.0f, 1.0f);
		}

		if ( forceScroll ) {
			scroller->SetValue( breaks.Num() - fit );
		} else if ( readonly ) {
		} else {
			cursorLine = 0;
			for ( int i = 1; i < breaks.Num(); i++ ) {
				if ( cursorPos >= breaks[i] ) {
					cursorLine = i;
				} else {
					break;
				}
			}
			int topLine = hfMath::Ftoi( scroller->GetValue() );
			if ( cursorLine < topLine ) {
				scroller->SetValue( cursorLine );
			} else if ( cursorLine >= topLine + fit) {
				scroller->SetValue( ( cursorLine - fit ) + 1 );
			}
		}
	}
}

void hfEditWindow::Activate(bool activate, hfStr &act) {
	hfWindow::Activate(activate, act);
	if ( activate ) {
		UpdateCvar( true, true );
		EnsureCursorVisible();
	}
}

/*
============
hfEditWindow::InitCvar
============
*/
void hfEditWindow::InitCvar( ) {
	if ( cvarStr[0] == '\0' ) {
		if ( text.GetName() == NULL ) {
			common->Warning( "hfEditWindow::InitCvar: gui '%s' window '%s' has an empty cvar string", gui->GetSourceFile(), name.c_str() );
		}
		cvar = NULL;
		return;
	}

	cvar = cvarSystem->Find( cvarStr );
	if ( !cvar ) {
		common->Warning( "hfEditWindow::InitCvar: gui '%s' window '%s' references undefined cvar '%s'", gui->GetSourceFile(), name.c_str(), cvarStr.c_str() );
		return;
	}
}

/*
============
hfEditWindow::UpdateCvar
============
*/
void hfEditWindow::UpdateCvar( bool read, bool force ) {
	if ( force || liveUpdate ) {
		if ( cvar ) {
			if ( read ) {
				text = cvar->GetString();
			} else {
				cvar->SetString( text );
				if ( cvarMax && ( cvar->GetInteger() > cvarMax ) ) {
					cvar->SetInteger( cvarMax );
				}
			}
		}
	}
}

/*
============
hfEditWindow::RunNamedEvent
============
*/
void hfEditWindow::RunNamedEvent( const char* eventName ) {
	hfStr event, group;
	
	if ( !hfStr::Cmpn( eventName, "cvar read ", 10 ) ) {
		event = eventName;
		group = event.Mid( 10, event.Length() - 10 );
		if ( !group.Cmp( cvarGroup ) ) {
			UpdateCvar( true, true );
		}
	} else if ( !hfStr::Cmpn( eventName, "cvar write ", 11 ) ) {
		event = eventName;
		group = event.Mid( 11, event.Length() - 11 );
		if ( !group.Cmp( cvarGroup ) ) {
			UpdateCvar( false, true );
		}
	}
}

