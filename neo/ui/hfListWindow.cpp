#pragma hdrstop
#include "../hflib/hfprecompiled.h"

#include "hfDeviceContext.h"
#include "hfWindow.h"
#include "hfUserInterfaceLocal.h"
#include "hfSliderWindow.h"
#include "hfListWindow.h"

// Number of pixels above the text that the rect starts
static const int pixelOffset = 3;

// number of pixels between columns
static const int tabBorder = 4;

// Time in milliseconds between clicks to register as a double-click
static const int doubleClickSpeed = 300;

void hfListWindow::CommonInit() {
	typed = "";
	typedTime = 0;
	clickTime = 0;
	currentSel.Clear();
	top = 0;
	sizeBias = 0;
	horizontal = false;
	scroller = new (TAG_OLD_UI) hfSliderWindow(gui);
	multipleSel = false;
}

hfListWindow::hfListWindow(hfUserInterfaceLocal *g) : hfWindow(g) {
	gui = g;
	CommonInit();
}

void hfListWindow::SetCurrentSel( int sel ) {
	currentSel.Clear();
	currentSel.Append( sel );
}

void hfListWindow::ClearSelection( int sel ) {
	int cur = currentSel.FindIndex( sel );
	if ( cur >= 0 ) {
		currentSel.RemoveIndex( cur );
	}
}

void hfListWindow::AddCurrentSel( int sel ) {
	currentSel.Append( sel );
}

int hfListWindow::GetCurrentSel() {
	return ( currentSel.Num() ) ? currentSel[0] : 0;
}

bool hfListWindow::IsSelected( int index ) {
	return ( currentSel.FindIndex( index ) >= 0 );
}

const char *hfListWindow::HandleEvent(const sysEvent_t *event, bool *updateVisuals) {
	// need to call this to allow proper focus and capturing on embedded children
	const char *ret = hfWindow::HandleEvent(event, updateVisuals);

	float vert = GetMaxCharHeight();
	int numVisibleLines = textRect.h / vert;

	int key = event->evValue;

	if ( event->evType == SE_KEY ) {
		if ( !event->evValue2 ) {
			// We only care about key down, not up
			return ret;
		}

		if ( key == K_MOUSE1 || key == K_MOUSE2 ) {
			// If the user clicked in the scroller, then ignore it
			if ( scroller->Contains(gui->CursorX(), gui->CursorY()) ) {
				return ret;
			}
		}

		if ( ( key == K_ENTER || key == K_KP_ENTER ) ) {
			RunScript( ON_ENTER );
			return cmd;
		}

		if ( key == K_MWHEELUP ) {
			key = K_UPARROW;
		} else if ( key == K_MWHEELDOWN ) {
			key = K_DOWNARROW;
		}

		if ( key == K_MOUSE1) {
			if (Contains(gui->CursorX(), gui->CursorY())) {
				int cur = ( int )( ( gui->CursorY() - actualY - pixelOffset ) / vert ) + top;
				if ( cur >= 0 && cur < listItems.Num() ) {
					if ( multipleSel && ( hfKeyInput::IsDown( K_LCTRL ) || hfKeyInput::IsDown( K_RCTRL ) ) ) {
						if ( IsSelected( cur ) ) {
							ClearSelection( cur );
						} else {
							AddCurrentSel( cur );
						}
					} else {
						if ( IsSelected( cur ) && ( gui->GetTime() < clickTime + doubleClickSpeed ) ) {
							// Double-click causes ON_ENTER to get run
							RunScript(ON_ENTER);
							return cmd;
						}
						SetCurrentSel( cur );

						clickTime = gui->GetTime();
					}
				} else {
					SetCurrentSel( listItems.Num() - 1 );
				}
			}
		} else if ( key == K_UPARROW || key == K_PGUP || key == K_DOWNARROW || key == K_PGDN ) {
			int numLines = 1;

			if ( key == K_PGUP || key == K_PGDN ) {
				numLines = numVisibleLines / 2;
			}

			if ( key == K_UPARROW || key == K_PGUP ) {
				numLines = -numLines;
			}

			if ( hfKeyInput::IsDown( K_LCTRL ) || hfKeyInput::IsDown( K_RCTRL ) ) {
				top += numLines;
			} else {
				SetCurrentSel( GetCurrentSel() + numLines );
			}
		} else {
			return ret;
		}
	} else if ( event->evType == SE_CHAR ) {
		if ( !hfStr::CharIsPrintable(key) ) {
			return ret;
		}

		if ( gui->GetTime() > typedTime + 1000 ) {
			typed = "";
		}
		typedTime = gui->GetTime();
		typed.Append( key );

		for ( int i=0; i<listItems.Num(); i++ ) {
			if ( hfStr::Icmpn( typed, listItems[i], typed.Length() ) == 0 ) {
				SetCurrentSel( i );
				break;
			}
		}

	} else {
		return ret;
	}

	if ( GetCurrentSel() < 0 ) {
		SetCurrentSel( 0 );
	}

	if ( GetCurrentSel() >= listItems.Num() ) {
		SetCurrentSel( listItems.Num() - 1 );
	}

	if ( scroller->GetHigh() > 0.0f ) {
		if ( !hfKeyInput::IsDown( K_LCTRL ) && !hfKeyInput::IsDown( K_RCTRL ) ) {
			if ( top > GetCurrentSel() - 1 ) {
				top = GetCurrentSel() - 1;
			}
			if ( top < GetCurrentSel() - numVisibleLines + 2 ) {
				top = GetCurrentSel() - numVisibleLines + 2;
			}
		}

		if ( top > listItems.Num() - 2 ) {
			top = listItems.Num() - 2;
		}
		if ( top < 0 ) {
			top = 0;
		}
		scroller->SetValue(top);
	} else {
		top = 0;
		scroller->SetValue(0.0f);
	}

	if ( key != K_MOUSE1 ) {
		// Send a fake mouse click event so onAction gets run in our parents
		const sysEvent_t ev = sys->GenerateMouseButtonEvent( 1, true );
		hfWindow::HandleEvent(&ev, updateVisuals);
	}

	if ( currentSel.Num() > 0 ) {
		for ( int i = 0; i < currentSel.Num(); i++ ) {
			gui->SetStateInt( va( "%s_sel_%i", listName.c_str(), i ), currentSel[i] );
		}
	} else {
		gui->SetStateInt( va( "%s_sel_0", listName.c_str() ), 0 );
	}
	gui->SetStateInt( va( "%s_numsel", listName.c_str() ), currentSel.Num() );

	return ret;
}


bool hfListWindow::ParseInternalVar(const char *_name, hfTokenParser *src) {
	if (hfStr::Icmp(_name, "horizontal") == 0) {
		horizontal = src->ParseBool();
		return true;
	}
	if (hfStr::Icmp(_name, "listname") == 0) {
		ParseString(src, listName);
		return true;
	}
	if (hfStr::Icmp(_name, "tabstops") == 0) {
		ParseString(src, tabStopStr);
		return true;
	}
	if (hfStr::Icmp(_name, "tabaligns") == 0) {
		ParseString(src, tabAlignStr);
		return true;
	}
	if (hfStr::Icmp(_name, "multipleSel") == 0) {
		multipleSel = src->ParseBool();
		return true;
	}
	if(hfStr::Icmp(_name, "tabvaligns") == 0) {
		ParseString(src, tabVAlignStr);
		return true;
	}
	if(hfStr::Icmp(_name, "tabTypes") == 0) {
		ParseString(src, tabTypeStr);
		return true;
	}
	if(hfStr::Icmp(_name, "tabIconSizes") == 0) {
		ParseString(src, tabIconSizeStr);
		return true;
	}
	if(hfStr::Icmp(_name, "tabIconVOffset") == 0) {
		ParseString(src, tabIconVOffsetStr);
		return true;
	}
	
	hfStr strName = _name;
	if(hfStr::Icmp(strName.Left(4), "mtr_") == 0) {
		hfStr matName;
		const hfMaterial* mat;

		ParseString(src, matName);
		mat = declManager->FindMaterial(matName);
		if ( mat != NULL && !mat->TestMaterialFlag( MF_DEFAULTED ) ) {
			mat->SetSort(SS_GUI );
		}
		iconMaterials.Set(_name, mat);
		return true;
	}

	return hfWindow::ParseInternalVar(_name, src);
}

hfWinVar *hfListWindow::GetWinVarByName(const char *_name, bool fixup, drawWin_t** owner) {
	return hfWindow::GetWinVarByName(_name, fixup, owner);
}

void hfListWindow::PostParse() {
	hfWindow::PostParse();

	InitScroller(horizontal);

	hfList<int> tabStops;
	hfList<int> tabAligns;
	if (tabStopStr.Length()) {
		hfParser src(tabStopStr, tabStopStr.Length(), "tabstops", LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS);
		hfToken tok;
		while (src.ReadToken(&tok)) {
			if (tok == ",") {
				continue;
			}
			tabStops.Append(atoi(tok));
		}
	}
	if (tabAlignStr.Length()) {
		hfParser src(tabAlignStr, tabAlignStr.Length(), "tabaligns", LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS);
		hfToken tok;
		while (src.ReadToken(&tok)) {
			if (tok == ",") {
				continue;
			}
			tabAligns.Append(atoi(tok));
		}
	}
	hfList<int> tabVAligns;
	if (tabVAlignStr.Length()) {
		hfParser src(tabVAlignStr, tabVAlignStr.Length(), "tabvaligns", LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS);
		hfToken tok;
		while (src.ReadToken(&tok)) {
			if (tok == ",") {
				continue;
			}
			tabVAligns.Append(atoi(tok));
		}
	}

	hfList<int> tabTypes;
	if (tabTypeStr.Length()) {
		hfParser src(tabTypeStr, tabTypeStr.Length(), "tabtypes", LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS);
		hfToken tok;
		while (src.ReadToken(&tok)) {
			if (tok == ",") {
				continue;
			}
			tabTypes.Append(atoi(tok));
		}
	}
	hfList<hfVec2> tabSizes;
	if (tabIconSizeStr.Length()) {
		hfParser src(tabIconSizeStr, tabIconSizeStr.Length(), "tabiconsizes", LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS);
		hfToken tok;
		while (src.ReadToken(&tok)) {
			if (tok == ",") {
				continue;
			}
			hfVec2 size;
			size.x = atoi(tok);
			
			src.ReadToken(&tok);	//","
			src.ReadToken(&tok);

			size.y = atoi(tok);
			tabSizes.Append(size);
		}
	}

	hfList<float> tabIconVOffsets;
	if (tabIconVOffsetStr.Length()) {
		hfParser src(tabIconVOffsetStr, tabIconVOffsetStr.Length(), "tabiconvoffsets", LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS);
		hfToken tok;
		while (src.ReadToken(&tok)) {
			if (tok == ",") {
				continue;
			}
			tabIconVOffsets.Append(atof(tok));
		}
	}

	int c = tabStops.Num();
	bool doAligns = (tabAligns.Num() == tabStops.Num());
	for (int i = 0; i < c; i++) {
		hfTabRect r;
		r.x = tabStops[i];
		r.w = (i < c - 1) ? tabStops[i+1] - r.x - tabBorder : -1;
		r.align = (doAligns) ? tabAligns[i] : 0;
		if(tabVAligns.Num() > 0) {
			r.valign = tabVAligns[i];
		} else {
			r.valign = 0;
		}
		if(tabTypes.Num() > 0) {
			r.type = tabTypes[i];
		} else {
			r.type = TAB_TYPE_TEXT;
		}
		if(tabSizes.Num() > 0) {
			r.iconSize = tabSizes[i];
		} else {
			r.iconSize.Zero();
		}
		if(tabIconVOffsets.Num() > 0 ) {
			r.iconVOffset = tabIconVOffsets[i];
		} else {
			r.iconVOffset = 0;
		}
		tabInfo.Append(r);
	}
	flags |= WIN_CANFOCUS;
}

/*
================
hfListWindow::InitScroller

This is the same as in hfEditWindow
================
*/
void hfListWindow::InitScroller( bool horizontal )
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

void hfListWindow::Draw(int time, float x, float y) {
	hfVec4 color;
	hfStr work;
	int count = listItems.Num();
	hfRectangle rect = textRect;
	float scale = textScale;
	float lineHeight = GetMaxCharHeight();

	float bottom = textRect.Bottom();
	float width = textRect.w;

	if ( scroller->GetHigh() > 0.0f ) {
		if ( horizontal ) {
			bottom -= sizeBias;
		} else {
			width -= sizeBias;
			rect.w = width;
		}
	}

	if ( noEvents || !Contains(gui->CursorX(), gui->CursorY()) ) {
		hover = false;
	}

	for (int i = top; i < count; i++) {
		if ( IsSelected( i ) ) {
			rect.h = lineHeight;
			dc->DrawFilledRect(rect.x, rect.y + pixelOffset, rect.w, rect.h, borderColor);
			if ( flags & WIN_FOCUS ) {
				hfVec4 color = borderColor;
				color.w = 1.0f;
				dc->DrawRect(rect.x, rect.y + pixelOffset, rect.w, rect.h, 1.0f, color );
			}
		}
		rect.y ++;
		rect.h = lineHeight - 1;
		if ( hover && !noEvents && Contains(rect, gui->CursorX(), gui->CursorY()) ) {
			color = hoverColor;
		} else {
			color = foreColor;
		}
		rect.h = lineHeight + pixelOffset;
		rect.y --;

		if ( tabInfo.Num() > 0 ) {
			int start = 0;
			int tab = 0;
			int stop = listItems[i].Find('\t', 0);
			while ( start < listItems[i].Length() ) {
				if ( tab >= tabInfo.Num() ) {
					common->Warning( "hfListWindow::Draw: gui '%s' window '%s' tabInfo.Num() exceeded", gui->GetSourceFile(), name.c_str() );
					break;
				}
				listItems[i].Mid(start, stop - start, work);

				rect.x = textRect.x + tabInfo[tab].x;
				rect.w = (tabInfo[tab].w == -1) ? width - tabInfo[tab].x : tabInfo[tab].w;
				dc->PushClipRect( rect );

				if ( tabInfo[tab].type == TAB_TYPE_TEXT ) {
					dc->DrawText(work, scale, tabInfo[tab].align, color, rect, false, -1);
				} else if (tabInfo[tab].type == TAB_TYPE_ICON) {
					
					const hfMaterial	**hashMat;
					const hfMaterial	*iconMat;

					// leaving the icon name empty doesn't draw anything
					if ( work[0] != '\0' ) {

						if ( iconMaterials.Get(work, &hashMat) == false ) {
							iconMat = declManager->FindMaterial("_default");
						} else {
							iconMat = *hashMat;
						}

						hfRectangle iconRect;
						iconRect.w = tabInfo[tab].iconSize.x;
						iconRect.h = tabInfo[tab].iconSize.y;

						if(tabInfo[tab].align == hfDeviceContext::ALIGN_LEFT) {
							iconRect.x = rect.x;
						} else if (tabInfo[tab].align == hfDeviceContext::ALIGN_CENTER) {
							iconRect.x = rect.x + rect.w/2.0f - iconRect.w/2.0f;
						} else if (tabInfo[tab].align == hfDeviceContext::ALIGN_RIGHT) {
							iconRect.x  = rect.x + rect.w - iconRect.w;
						}

						if(tabInfo[tab].valign == 0) { //Top
							iconRect.y = rect.y + tabInfo[tab].iconVOffset;
						} else if(tabInfo[tab].valign == 1) { //Center
							iconRect.y = rect.y + rect.h/2.0f - iconRect.h/2.0f + tabInfo[tab].iconVOffset;
						} else if(tabInfo[tab].valign == 2) { //Bottom
							iconRect.y = rect.y + rect.h - iconRect.h + tabInfo[tab].iconVOffset;
						}

						dc->DrawMaterial(iconRect.x, iconRect.y, iconRect.w, iconRect.h, iconMat, hfVec4(1.0f,1.0f,1.0f,1.0f), 1.0f, 1.0f);

					}
				}

				dc->PopClipRect();

				start = stop + 1;
				stop = listItems[i].Find('\t', start);
				if ( stop < 0 ) {
					stop = listItems[i].Length();
				}
				tab++;
			}
			rect.x = textRect.x;
			rect.w = width;
		} else {
			dc->DrawText(listItems[i], scale, 0, color, rect, false, -1);
		}
		rect.y += lineHeight;
		if ( rect.y > bottom ) {
			break;
		}
	}
}

void hfListWindow::Activate(bool activate, hfStr &act) {
	hfWindow::Activate(activate, act);

	if ( activate ) {
		UpdateList();
	}
}

void hfListWindow::HandleBuddyUpdate(hfWindow *buddy) {
	top = scroller->GetValue();
}

void hfListWindow::UpdateList() {
	hfStr str, strName;
	listItems.Clear();
	for (int i = 0; i < MAX_LIST_ITEMS; i++) {
		if (gui->State().GetString( va("%s_item_%i", listName.c_str(), i), "", str) ) {
			if ( str.Length() ) {
				listItems.Append(str);
			}
		} else {
			break;
		}
	}
	float vert = GetMaxCharHeight();
	int fit = textRect.h / vert;
	if ( listItems.Num() < fit ) {
		scroller->SetRange(0.0f, 0.0f, 1.0f);
	} else {
		scroller->SetRange(0.0f, (listItems.Num() - fit) + 1.0f, 1.0f);
	}

	SetCurrentSel( gui->State().GetInt( va( "%s_sel_0", listName.c_str() ) ) );

	float value = scroller->GetValue();
	if ( value > listItems.Num() - 1 ) {
		value = listItems.Num() - 1;
	}
	if ( value < 0.0f ) {
		value = 0.0f;
	}
	scroller->SetValue(value);
	top = value;

	typedTime = 0;
	clickTime = 0;
	typed = "";
}

void hfListWindow::StateChanged( bool redraw ) {
	UpdateList();
}


