#ifndef __GUISCRIPT_H
#define __GUISCRIPT_H

#include "hfWindow.h"
#include "hfWinvar.h"

struct hfGSWinVar {
	hfGSWinVar() {
		var = NULL;
		own = false;
	}
	hfWinVar* var;
	bool own;
};

class hfGuiScriptList;

class hfGuiScript {
	friend class hfGuiScriptList;
	friend class hfWindow;

public:
	hfGuiScript();
	~hfGuiScript();

	bool Parse(hfTokenParser *src);
	void Execute(hfWindow *win) {
		if (handler) {
			handler(win, &parms);
		}
	}
	void FixupParms(hfWindow *win);
	size_t Size() {
		int sz = sizeof(*this);
		for (int i = 0; i < parms.Num(); i++) {
			sz += parms[i].var->Size();
		}
		return sz;
	}

	void WriteToSaveGame( hfFile *savefile );
	void ReadFromSaveGame( hfFile *savefile );

protected:
	int conditionReg;
	hfGuiScriptList *ifList;
	hfGuiScriptList *elseList;
	hfList<hfGSWinVar, TAG_OLD_UI> parms;
	void (*handler) (hfWindow *window, hfList<hfGSWinVar, TAG_OLD_UI> *src);

};


class hfGuiScriptList {
	hfList<hfGuiScript*, TAG_OLD_UI> list;
public:
	hfGuiScriptList() { list.SetGranularity( 4 ); };
	~hfGuiScriptList() { list.DeleteContents(true); };
	void Execute(hfWindow *win);
	void Append(hfGuiScript* gs) {
		list.Append(gs);
	}
	size_t Size() {
		int sz = sizeof(*this);
		for (int i = 0; i < list.Num(); i++) {
			sz += list[i]->Size();
		}
		return sz;
	}
	void FixupParms(hfWindow *win);
	void ReadFromDemoFile( class hfDemoFile *f ) {};
	void WriteToDemoFile( class hfDemoFile *f ) {};

	void WriteToSaveGame( hfFile *savefile );
	void ReadFromSaveGame( hfFile *savefile );
};

#endif // __GUISCRIPT_H

