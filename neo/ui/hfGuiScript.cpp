#pragma hdrstop
#include "../hflib/hfprecompiled.h"

#include "hfWindow.h"
#include "hfWinvar.h"
#include "hfGuiScript.h"
#include "hfUserInterfaceLocal.h"

/*
=========================
Script_Set
=========================
*/
void Script_Set(hfWindow *window, hfList<hfGSWinVar, TAG_OLD_UI> *src) {
	hfStr key, val;
	hfWinStr *dest = dynamic_cast<hfWinStr*>((*src)[0].var);
	if (dest) {
		if (hfStr::Icmp(*dest, "cmd") == 0) {
			dest = dynamic_cast<hfWinStr*>((*src)[1].var);
			int parmCount = src->Num();
			if (parmCount > 2) {
				val = dest->c_str();
				int i = 2;
				while (i < parmCount) {
					val += " \"";
					val += (*src)[i].var->c_str();
					val += "\"";
					i++;
				}
				window->AddCommand(val);
			} else {
				window->AddCommand(*dest);
			}
			return;
		} 
	}
	(*src)[0].var->Set((*src)[1].var->c_str());
	(*src)[0].var->SetEval(false);
}

/*
=========================
Script_SetFocus
=========================
*/
void Script_SetFocus(hfWindow *window, hfList<hfGSWinVar, TAG_OLD_UI> *src) {
	hfWinStr *parm = dynamic_cast<hfWinStr*>((*src)[0].var);
	if (parm) {
		drawWin_t *win = window->GetGui()->GetDesktop()->FindChildByName(*parm);
		if ( win != NULL && win->win != NULL ) {
			window->SetFocus(win->win);
		}
	}
}

/*
=========================
Script_ShowCursor
=========================
*/
void Script_ShowCursor(hfWindow *window, hfList<hfGSWinVar, TAG_OLD_UI> *src) {
	hfWinStr *parm = dynamic_cast<hfWinStr*>((*src)[0].var);
	if ( parm ) {
		if ( atoi( *parm ) ) {
			window->GetGui()->GetDesktop()->ClearFlag( WIN_NOCURSOR );
		} else {
			window->GetGui()->GetDesktop()->SetFlag( WIN_NOCURSOR );
		}
	}
}

/*
=========================
Script_RunScript

 run scripts must come after any set cmd set's in the script
=========================
*/
void Script_RunScript(hfWindow *window, hfList<hfGSWinVar, TAG_OLD_UI> *src) {
	hfWinStr *parm = dynamic_cast<hfWinStr*>((*src)[0].var);
	if (parm) {
		hfStr str = window->cmd;
		str += " ; runScript ";
		str += parm->c_str();
		window->cmd = str;
	}
}

/*
=========================
Script_LocalSound
=========================
*/
void Script_LocalSound(hfWindow *window, hfList<hfGSWinVar, TAG_OLD_UI> *src) {
	hfWinStr *parm = dynamic_cast<hfWinStr*>((*src)[0].var);
	if (parm) {
		common->SW()->PlayShaderDirectly(*parm);
	}
}

/*
=========================
Script_EvalRegs
=========================
*/
void Script_EvalRegs(hfWindow *window, hfList<hfGSWinVar, TAG_OLD_UI> *src) {
	window->EvalRegs(-1, true);
}

/*
=========================
Script_EndGame
=========================
*/
void Script_EndGame( hfWindow *window, hfList<hfGSWinVar, TAG_OLD_UI> *src ) {
	cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "disconnect\n" );
}

/*
=========================
Script_ResetTime
=========================
*/
void Script_ResetTime(hfWindow *window, hfList<hfGSWinVar, TAG_OLD_UI> *src) {
	hfWinStr *parm = dynamic_cast<hfWinStr*>((*src)[0].var);
	drawWin_t *win = NULL;
	if (parm != NULL && src->Num() > 1) {
		win = window->GetGui()->GetDesktop()->FindChildByName(*parm);
		parm = dynamic_cast<hfWinStr*>((*src)[1].var);
	}
	if ( parm == NULL ) {
		return;
	}

	if (win != NULL && win->win != NULL) {
		win->win->ResetTime(atoi(*parm));
		win->win->EvalRegs(-1, true);
	} else {
		window->ResetTime(atoi(*parm));
		window->EvalRegs(-1, true);
	}
}

/*
=========================
Script_ResetCinematics
=========================
*/
void Script_ResetCinematics(hfWindow *window, hfList<hfGSWinVar, TAG_OLD_UI> *src) {
	window->ResetCinematics();
}

/*
=========================
Script_Transition
=========================
*/
void Script_Transition(hfWindow *window, hfList<hfGSWinVar, TAG_OLD_UI> *src) {
	// transitions always affect rect or vec4 vars
	if (src->Num() >= 4) {
		hfWinRectangle *rect = NULL;
		hfWinVec4 *vec4 = dynamic_cast<hfWinVec4*>((*src)[0].var);
		// 
		//  added float variable
		hfWinFloat* val = NULL;
		// 
		if (vec4 == NULL) {
			rect = dynamic_cast<hfWinRectangle*>((*src)[0].var);
			// 
			//  added float variable					
			if ( NULL == rect ) {
				val = dynamic_cast<hfWinFloat*>((*src)[0].var);
			}
			// 
		}
		hfWinVec4 *from = dynamic_cast<hfWinVec4*>((*src)[1].var);
		hfWinVec4 *to = dynamic_cast<hfWinVec4*>((*src)[2].var);
		hfWinStr *timeStr = dynamic_cast<hfWinStr*>((*src)[3].var);
		// 
		//  added float variable					
		if (!((vec4 || rect || val) && from && to && timeStr)) {
			// 
			common->Warning("Bad transition in gui %s in window %s\n", window->GetGui()->GetSourceFile(), window->GetName());
			return;
		}
		int time = atoi(*timeStr);
		float ac = 0.0f;
		float dc = 0.0f;
		if (src->Num() > 4) {
			hfWinStr *acv = dynamic_cast<hfWinStr*>((*src)[4].var);
			hfWinStr *dcv = dynamic_cast<hfWinStr*>((*src)[5].var);
			assert(acv && dcv);
			ac = atof(*acv);
			dc = atof(*dcv);
		}
				
		if (vec4) {
			vec4->SetEval(false);
			window->AddTransition(vec4, *from, *to, time, ac, dc);
			// 
			//  added float variable					
		} else if ( val ) {
			val->SetEval ( false );
			window->AddTransition(val, *from, *to, time, ac, dc);
			// 
		} else {
			rect->SetEval(false);
			window->AddTransition(rect, *from, *to, time, ac, dc);
		}
		window->StartTransition();
	}
}

typedef struct {
	const char *name;
	void (*handler) (hfWindow *window, hfList<hfGSWinVar, TAG_OLD_UI> *src);
	int mMinParms;
	int mMaxParms;
} guiCommandDef_t;

guiCommandDef_t commandList[] = {
	{ "set", Script_Set, 2, 999 },
	{ "setFocus", Script_SetFocus, 1, 1 },
	{ "endGame", Script_EndGame, 0, 0 },
	{ "resetTime", Script_ResetTime, 0, 2 },
	{ "showCursor", Script_ShowCursor, 1, 1 },
	{ "resetCinematics", Script_ResetCinematics, 0, 2 },
	{ "transition", Script_Transition, 4, 6 },
	{ "localSound", Script_LocalSound, 1, 1 },
	{ "runScript", Script_RunScript, 1, 1 },
	{ "evalRegs", Script_EvalRegs, 0, 0 }
};

int	scriptCommandCount = sizeof(commandList) / sizeof(guiCommandDef_t);


/*
=========================
hfGuiScript::hfGuiScript
=========================
*/
hfGuiScript::hfGuiScript() {
	ifList = NULL;
	elseList = NULL;
	conditionReg = -1;
	handler = NULL;
	parms.SetGranularity( 2 );
}

/*
=========================
hfGuiScript::~hfGuiScript
=========================
*/
hfGuiScript::~hfGuiScript() {
	delete ifList;
	delete elseList;
	int c = parms.Num();
	for ( int i = 0; i < c; i++ ) {
		if ( parms[i].own ) {
			delete parms[i].var;
		}
	}
}

/*
=========================
hfGuiScript::WriteToSaveGame
=========================
*/
void hfGuiScript::WriteToSaveGame( hfFile *savefile ) {
	int i;

	if ( ifList ) {
		ifList->WriteToSaveGame( savefile );
	}
	if ( elseList ) {
		elseList->WriteToSaveGame( savefile );
	}

	savefile->Write( &conditionReg, sizeof( conditionReg ) );

	for ( i = 0; i < parms.Num(); i++ ) {
		if ( parms[i].own ) {
			parms[i].var->WriteToSaveGame( savefile );
		}
	}
}

/*
=========================
hfGuiScript::ReadFromSaveGame
=========================
*/
void hfGuiScript::ReadFromSaveGame( hfFile *savefile ) {
	int i;

	if ( ifList ) {
		ifList->ReadFromSaveGame( savefile );
	}
	if ( elseList ) {
		elseList->ReadFromSaveGame( savefile );
	}

	savefile->Read( &conditionReg, sizeof( conditionReg ) );

	for ( i = 0; i < parms.Num(); i++ ) {
		if ( parms[i].own ) {
			parms[i].var->ReadFromSaveGame( savefile );
		}
	}
}

/*
=========================
hfGuiScript::Parse
=========================
*/
bool hfGuiScript::Parse(hfTokenParser *src) {
	int i;
	
	// first token should be function call
	// then a potentially variable set of parms
	// ended with a ;
	hfToken token;
	if ( !src->ReadToken(&token) ) {
		src->Error( "Unexpected end of file" );
		return false;
	}

	handler	= NULL;
	
	for ( i = 0; i < scriptCommandCount ; i++ ) {
		if ( hfStr::Icmp(token, commandList[i].name) == 0 ) {
			handler = commandList[i].handler;
			break;
		}
	}

	if (handler == NULL) {
		src->Error("Uknown script call %s", token.c_str());
	}
	// now read parms til ;
	// all parms are read as hfWinStr's but will be fixed up later 
	// to be proper types
	while (1) {
		if ( !src->ReadToken(&token) ) {
			src->Error( "Unexpected end of file" );
			return false;
		}
		
		if (hfStr::Icmp(token, ";") == 0) {
			break;
		}

		if (hfStr::Icmp(token, "}") == 0) {
			src->UnreadToken(&token);
			break;
		}

		hfWinStr *str = new (TAG_OLD_UI) hfWinStr();
		*str = token;
		hfGSWinVar wv;
		wv.own = true;
		wv.var = str;
		parms.Append( wv );
	}

	// 
	//  verify min/max params
	if ( handler && (parms.Num() < commandList[i].mMinParms || parms.Num() > commandList[i].mMaxParms ) ) {
		src->Error("incorrect number of parameters for script %s", commandList[i].name );
	}
	// 

	return true;
}

/*
=========================
hfGuiScriptList::Execute
=========================
*/
void hfGuiScriptList::Execute(hfWindow *win) {
	int c = list.Num();
	for (int i = 0; i < c; i++) {
		hfGuiScript *gs = list[i];
		assert(gs);
		if (gs->conditionReg >= 0) {
			if (win->HasOps()) {
				float f = win->EvalRegs(gs->conditionReg);
				if (f) {
					if (gs->ifList) {
						win->RunScriptList(gs->ifList);
					}
				} else if (gs->elseList) {
					win->RunScriptList(gs->elseList);
				}
			}
		}
		gs->Execute(win);
	}
}

/*
=========================
hfGuiScriptList::FixupParms
=========================
*/
void hfGuiScript::FixupParms(hfWindow *win) {
	if (handler == &Script_Set) {
		bool precacheBackground = false;
		bool precacheSounds = false;
		hfWinStr *str = dynamic_cast<hfWinStr*>(parms[0].var);
		assert(str);
		hfWinVar *dest = win->GetWinVarByName(*str, true);
		if (dest) {
			delete parms[0].var;
			parms[0].var = dest;
			parms[0].own = false;

			if ( dynamic_cast<hfWinBackground *>(dest) != NULL ) {
				precacheBackground = true;
			}
		} else if ( hfStr::Icmp( str->c_str(), "cmd" ) == 0 ) {
			precacheSounds = true;
		}
		int parmCount = parms.Num();
		for (int i = 1; i < parmCount; i++) {
			hfWinStr *str = dynamic_cast<hfWinStr*>(parms[i].var);		
			if (hfStr::Icmpn(*str, "gui::", 5) == 0) {

				//  always use a string here, no point using a float if it is one
				//  FIXME: This creates duplicate variables, while not technically a problem since they
				//  are all bound to the same guiDict, it does consume extra memory and is generally a bad thing
				hfWinStr* defvar = new (TAG_OLD_UI) hfWinStr();
				defvar->Init ( *str, win );
				win->AddDefinedVar ( defvar );
				delete parms[i].var;
				parms[i].var = defvar;
				parms[i].own = false;

				//dest = win->GetWinVarByName(*str, true);
				//if (dest) {
				//	delete parms[i].var;
				//	parms[i].var = dest;
				//	parms[i].own = false;
				//}
				// 
			} else if ((*str[0]) == '$') {
				// 
				//  dont include the $ when asking for variable
				dest = win->GetGui()->GetDesktop()->GetWinVarByName((const char*)(*str) + 1, true);
				// 					
				if (dest) {
					delete parms[i].var;
					parms[i].var = dest;
					parms[i].own = false;
				}
			} else if ( hfStr::Cmpn( str->c_str(), STRTABLE_ID, STRTABLE_ID_LENGTH ) == 0 ) {
				str->Set( hfLocalization::GetString( str->c_str() ) );
			} else if ( precacheBackground ) {
				const hfMaterial *mat = declManager->FindMaterial( str->c_str() );
				mat->SetSort( SS_GUI );
			} else if ( precacheSounds ) {
				// Search for "play <...>"
				hfToken token;
				hfParser parser( LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT );
				parser.LoadMemory(str->c_str(), str->Length(), "command");

				while ( parser.ReadToken(&token) ) {
					if ( token.Icmp("play") == 0 ) {
						if ( parser.ReadToken(&token) && ( token != "" ) ) {
							declManager->FindSound( token.c_str() );
						}
					}
				}
			}
		}
	} else if (handler == &Script_Transition) {
		if (parms.Num() < 4) {
			common->Warning("Window %s in gui %s has a bad transition definition", win->GetName(), win->GetGui()->GetSourceFile()); 
		}
		hfWinStr *str = dynamic_cast<hfWinStr*>(parms[0].var);
		assert(str);

		// 
		drawWin_t *destowner;
		hfWinVar *dest = win->GetWinVarByName(*str, true, &destowner );
		// 

		if (dest) {
			delete parms[0].var;
			parms[0].var = dest;
			parms[0].own = false;
		} else {
			common->Warning("Window %s in gui %s: a transition does not have a valid destination var %s", win->GetName(), win->GetGui()->GetSourceFile(),str->c_str());
		}

		// 
		//  support variables as parameters		
		int c;
		for ( c = 1; c < 3; c ++ ) {
			str = dynamic_cast<hfWinStr*>(parms[c].var);

			hfWinVec4 *v4 = new (TAG_OLD_UI) hfWinVec4;
			parms[c].var = v4;
			parms[c].own = true;

			drawWin_t* owner = NULL;

			if ( (*str[0]) == '$' ) {
				dest = win->GetWinVarByName ( (const char*)(*str) + 1, true, &owner );
			} else {
				dest = NULL;
			}

			if ( dest ) {	
				hfWindow* ownerparent;
				hfWindow* destparent;
				if ( owner != NULL ) {
					ownerparent = owner->simp?owner->simp->GetParent():owner->win->GetParent();
					destparent  = destowner->simp?destowner->simp->GetParent():destowner->win->GetParent();

					// If its the rectangle they are referencing then adjust it 
					if ( ownerparent && destparent && 
						(dest == (owner->simp?owner->simp->GetWinVarByName ( "rect" ):owner->win->GetWinVarByName ( "rect" ) ) ) )
					{
						hfRectangle rect;
						rect = *(dynamic_cast<hfWinRectangle*>(dest));
						ownerparent->ClientToScreen ( &rect );
						destparent->ScreenToClient ( &rect );
						*v4 = rect.ToVec4 ( );
					} else {
						v4->Set ( dest->c_str ( ) );
					}
				} else {
					v4->Set ( dest->c_str ( ) );
				}
			} else {
				v4->Set(*str);
			}			
			
			delete str;
		}		
		// 
	} else if (handler == &Script_LocalSound) {
		hfWinStr * str = dynamic_cast<hfWinStr*>(parms[0].var);
		if ( str ) {
			declManager->FindSound( str->c_str() );
		}
	} else {
		int c = parms.Num();
		for (int i = 0; i < c; i++) {
			parms[i].var->Init(parms[i].var->c_str(), win);
		}
	}
}

/*
=========================
hfGuiScriptList::FixupParms
=========================
*/
void hfGuiScriptList::FixupParms(hfWindow *win) {
	int c = list.Num();
	for (int i = 0; i < c; i++) {
		hfGuiScript *gs = list[i];
		gs->FixupParms(win);
		if (gs->ifList) {
			gs->ifList->FixupParms(win);
		}
		if (gs->elseList) {
			gs->elseList->FixupParms(win);
		}
	}
}

/*
=========================
hfGuiScriptList::WriteToSaveGame
=========================
*/
void hfGuiScriptList::WriteToSaveGame( hfFile *savefile ) {
	int i;

	for ( i = 0; i < list.Num(); i++ ) {
		list[i]->WriteToSaveGame( savefile );
	}
}

/*
=========================
hfGuiScriptList::ReadFromSaveGame
=========================
*/
void hfGuiScriptList::ReadFromSaveGame( hfFile *savefile ) {
	int i;

	for ( i = 0; i < list.Num(); i++ ) {
		list[i]->ReadFromSaveGame( savefile );
	}
}

