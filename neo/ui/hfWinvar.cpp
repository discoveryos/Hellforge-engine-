#pragma hdrstop
#include "../hflib/hfprecompiled.h"

#include "hfWindow.h"
#include "hfWinvar.h"
#include "hfUserInterfaceLocal.h"

hfWinVar::hfWinVar() { 
	guiDict = NULL; 
	name = NULL; 
	eval = true;
}

hfWinVar::~hfWinVar() { 
	delete name;
	name = NULL;
}

void hfWinVar::SetGuiInfo(hfDict *gd, const char *_name) { 
	guiDict = gd; 
	SetName(_name); 
}


void hfWinVar::Init(const char *_name, hfWindow *win) {
	hfStr key = _name;
	guiDict = NULL;
	int len = key.Length();
	if (len > 5 && key[0] == 'g' && key[1] == 'u' && key[2] == 'i' && key[3] == ':') {
		key = key.Right(len - VAR_GUIPREFIX_LEN);
		SetGuiInfo( win->GetGui()->GetStateDict(), key );
		win->AddUpdateVar(this);
	} else {
		Set(_name);
	}
}

void hfMultiWinVar::Set( const char *val ) {
	for ( int i = 0; i < Num(); i++ ) {
		(*this)[i]->Set( val );
	}
}

void hfMultiWinVar::Update() {
	for ( int i = 0; i < Num(); i++ ) {
		(*this)[i]->Update();
	}
}

void hfMultiWinVar::SetGuiInfo( hfDict *dict ) {
	for ( int i = 0; i < Num(); i++ ) {
		(*this)[i]->SetGuiInfo( dict, (*this)[i]->c_str() );
	}
}


