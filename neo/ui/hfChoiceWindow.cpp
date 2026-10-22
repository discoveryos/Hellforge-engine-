#pragma hdrstop
#include "../hflib/hfprecompiled.h"

#include "hfDeviceContext.h"
#include "hfWindow.h"
#include "hfUserInterfaceLocal.h"
#include "hfChoiceWindow.h"

/*
============
hfChoiceWindow::InitVars
============
*/
void hfChoiceWindow::InitVars( ) {
	if ( cvarStr.Length() ) {
		cvar = cvarSystem->Find( cvarStr );
		if ( !cvar ) {
			common->Warning( "hfChoiceWindow::InitVars: gui '%s' window '%s' references undefined cvar '%s'", gui->GetSourceFile(), name.c_str(), cvarStr.c_str() );
			return;
		}
		updateStr.Append( &cvarStr );
	}
	if ( guiStr.Length() ) {
		updateStr.Append( &guiStr );
	}
	updateStr.SetGuiInfo( gui->GetStateDict() );
	updateStr.Update();
}

/*
============
hfChoiceWindow::CommonInit
============
*/
void hfChoiceWindow::CommonInit() {
	currentChoice = 0;
	choiceType = 0;
	cvar = NULL;
	liveUpdate = true;
	choices.Clear();
}

hfChoiceWindow::hfChoiceWindow(hfUserInterfaceLocal *g) : hfWindow(g) {
	gui = g;
	CommonInit();
}

hfChoiceWindow::~hfChoiceWindow() {

}

void hfChoiceWindow::RunNamedEvent( const char* eventName ) {
	hfStr event, group;
	
	if ( !hfStr::Cmpn( eventName, "cvar read ", 10 ) ) {
		event = eventName;
		group = event.Mid( 10, event.Length() - 10 );
		if ( !group.Cmp( updateGroup ) ) {
			UpdateVars( true, true );
		}
	} else if ( !hfStr::Cmpn( eventName, "cvar write ", 11 ) ) {
		event = eventName;
		group = event.Mid( 11, event.Length() - 11 );
		if ( !group.Cmp( updateGroup ) ) {
			UpdateVars( false, true );
		}
	}
}

void hfChoiceWindow::UpdateVars( bool read, bool force ) {
	if ( force || liveUpdate ) {
		if ( cvar && cvarStr.NeedsUpdate() ) {
			if ( read ) {
				cvarStr.Set( cvar->GetString() );
			} else {
				cvar->SetString( cvarStr.c_str() );
			}	
		}
		if ( !read && guiStr.NeedsUpdate() ) {
			guiStr.Set( va( "%i", currentChoice ) );
		}
	}
}

const char *hfChoiceWindow::HandleEvent(const sysEvent_t *event, bool *updateVisuals) {
	int key;
	bool runAction = false;
	bool runAction2 = false;

	if ( event->evType == SE_KEY ) {
		key = event->evValue;

		if ( key == K_RIGHTARROW || key == K_KP_6 || key == K_MOUSE1)  {
			// never affects the state, but we want to execute script handlers anyway
			if ( !event->evValue2 ) {
				RunScript( ON_ACTIONRELEASE );
				return cmd;
			}
			currentChoice++;
			if (currentChoice >= choices.Num()) {
				currentChoice = 0;
			}
			runAction = true;
		}

		if ( key == K_LEFTARROW || key == K_KP_4 || key == K_MOUSE2) {
			// never affects the state, but we want to execute script handlers anyway
			if ( !event->evValue2 ) {
				RunScript( ON_ACTIONRELEASE );
				return cmd;
			}
			currentChoice--;
			if (currentChoice < 0) {
				currentChoice = choices.Num() - 1;
			}
			runAction = true;
		}

		if ( !event->evValue2 ) {
			// is a key release with no action catch
			return "";
		}

	} else if ( event->evType == SE_CHAR ) {

		key = event->evValue;

		int potentialChoice = -1;
		for ( int i = 0; i < choices.Num(); i++ ) {
			if ( toupper(key) == toupper(choices[i][0]) ) {
				if ( i < currentChoice && potentialChoice < 0 ) {
					potentialChoice = i;
				} else if ( i > currentChoice ) {
					potentialChoice = -1;
					currentChoice = i;
					break;
				}
			}
		}
		if ( potentialChoice >= 0 ) {
			currentChoice = potentialChoice;
		}

		runAction = true;
		runAction2 = true;

	} else {
		return "";
	}

	if ( runAction ) {
		RunScript( ON_ACTION );
	}

	if ( choiceType == 0 ) {
		cvarStr.Set( va( "%i", currentChoice ) );
	} else if ( values.Num() ) {
		cvarStr.Set( values[ currentChoice ] );
	} else {
		cvarStr.Set( choices[ currentChoice ] );
	}

	UpdateVars( false );

	if ( runAction2 ) {
		RunScript( ON_ACTIONRELEASE );
	}
	
	return cmd;
}

void hfChoiceWindow::ValidateChoice() {
	if ( currentChoice < 0 || currentChoice >= choices.Num() ) {
		currentChoice = 0;
	}
	if ( choices.Num() == 0 ) {
		choices.Append( "No Choices Defined" );
	}
}

void hfChoiceWindow::UpdateChoice() {
	if ( !updateStr.Num() ) {
		return;
	}
	UpdateVars( true );	
	updateStr.Update();
	if ( choiceType == 0 ) {
		// ChoiceType 0 stores current as an integer in either cvar or gui
		// If both cvar and gui are defined then cvar wins, but they are both updated
		if ( updateStr[ 0 ]->NeedsUpdate() ) {
			currentChoice = atoi( updateStr[ 0 ]->c_str() );
		}
		ValidateChoice();
	} else {
		// ChoiceType 1 stores current as a cvar string
		int c = ( values.Num() ) ? values.Num() : choices.Num();
		int i;
		for ( i = 0; i < c; i++ ) {
			if ( hfStr::Icmp( cvarStr.c_str(), ( values.Num() ) ? values[i] : choices[i] ) == 0 ) {
				break;
			}
		}
		if (i == c) {
			i = 0;
		}
		currentChoice = i;
		ValidateChoice();
	}
}

bool hfChoiceWindow::ParseInternalVar(const char *_name, hfTokenParser *src) {
	if (hfStr::Icmp(_name, "choicetype") == 0) {
		choiceType = src->ParseInt();
		return true;
	}
	if (hfStr::Icmp(_name, "currentchoice") == 0) {
		currentChoice = src->ParseInt();
		return true;
	}
	return hfWindow::ParseInternalVar(_name, src);
}


hfWinVar *hfChoiceWindow::GetWinVarByName(const char *_name, bool fixup, drawWin_t** owner) {
	if ( hfStr::Icmp( _name, "choices" ) == 0 ) {
		return &choicesStr;
	}
	if ( hfStr::Icmp( _name, "values" ) == 0 ) {
		return &choiceVals;
	}
	if ( hfStr::Icmp( _name, "cvar" ) == 0 ) {
		return &cvarStr;
	}
	if ( hfStr::Icmp( _name, "gui" ) == 0 ) {
		return &guiStr;
	}
	if ( hfStr::Icmp( _name, "liveUpdate" ) == 0 ) {
		return &liveUpdate;
	}
	if ( hfStr::Icmp( _name, "updateGroup" ) == 0 ) {
		return &updateGroup;
	}
	
	return hfWindow::GetWinVarByName(_name, fixup, owner);
}

// update the lists whenever the WinVar have changed
void hfChoiceWindow::UpdateChoicesAndVals() {
	hfToken token;
	hfStr str2, str3;
	hfLexer src;

	if ( latchedChoices.Icmp( choicesStr ) ) {
		choices.Clear();
		src.FreeSource();
		src.SetFlags( LEXFL_NOFATALERRORS | LEXFL_ALLOWPATHNAMES | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT );
		src.LoadMemory( choicesStr, choicesStr.Length(), "<ChoiceList>" );
		if ( src.IsLoaded() ) {
			while( src.ReadToken( &token ) ) {
				if ( token == ";" ) {
					if ( str2.Length() ) {
						str2.StripTrailingWhitespace();
						str2 = hfLocalization::GetString( str2 );
						choices.Append(str2);
						str2 = "";
					}
					continue;
				}
				str2 += token;
				str2 += " ";
			}
			if ( str2.Length() ) {
				str2.StripTrailingWhitespace();
				choices.Append( str2 );
			}
		}
		latchedChoices = choicesStr.c_str();
	}
	if ( choiceVals.Length() && latchedVals.Icmp( choiceVals ) ) {
		values.Clear();
		src.FreeSource();
		src.SetFlags( LEXFL_ALLOWPATHNAMES | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT );
		src.LoadMemory( choiceVals, choiceVals.Length(), "<ChoiceVals>" );
		str2 = "";
		bool negNum = false;
		if ( src.IsLoaded() ) {
			while( src.ReadToken( &token ) ) {
				if (token == "-") {
					negNum = true;
					continue;
				} 
				if (token == ";") {
					if (str2.Length()) {
						str2.StripTrailingWhitespace();
						values.Append( str2 );
						str2 = "";
					}
					continue;
				}
				if ( negNum ) {
					str2 += "-";
					negNum = false;
				}
				str2 += token;
				str2 += " ";
			}
			if ( str2.Length() ) {
				str2.StripTrailingWhitespace();
				values.Append( str2 );
			}
		}
		if ( choices.Num() != values.Num() ) {
			common->Warning( "hfChoiceWindow:: gui '%s' window '%s' has value count unequal to choices count", gui->GetSourceFile(), name.c_str());
		}
		latchedVals = choiceVals.c_str();
	}
}

void hfChoiceWindow::PostParse() {
	hfWindow::PostParse();
	UpdateChoicesAndVals();

	InitVars();
	UpdateChoice();
	UpdateVars(false);

	flags |= WIN_CANFOCUS;
}

void hfChoiceWindow::Draw(int time, float x, float y) {
	hfVec4 color = foreColor;

	UpdateChoicesAndVals();
	UpdateChoice();

	// FIXME: It'd be really cool if textAlign worked, but a lot of the guis have it set wrong because it used to not work
	textAlign = 0;

	if ( textShadow ) {
		hfStr shadowText = choices[currentChoice];
		hfRectangle shadowRect = textRect;

		shadowText.RemoveColors();
		shadowRect.x += textShadow;
		shadowRect.y += textShadow;

		dc->DrawText( shadowText, textScale, textAlign, colorBlack, shadowRect, false, -1 );
	}

	if ( hover && !noEvents && Contains(gui->CursorX(), gui->CursorY()) ) {
		color = hoverColor;
	} else {
		hover = false;
	}
	if ( flags & WIN_FOCUS ) {
		color = hoverColor;
	}

	dc->DrawText( choices[currentChoice], textScale, textAlign, color, textRect, false, -1 );
}

void hfChoiceWindow::Activate( bool activate, hfStr &act ) {
	hfWindow::Activate( activate, act );
	if ( activate ) {
		// sets the gui state based on the current choice the window contains
		UpdateChoice();
	}
}

