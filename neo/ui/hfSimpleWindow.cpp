#pragma hdrstop
#include "../hflib/hfprecompiled.h"

#include "hfDeviceContext.h"
#include "hfWindow.h"
#include "hfUserInterfaceLocal.h"
#include "hfSimpleWindow.h"


hfSimpleWindow::hfSimpleWindow(hfWindow *win) {
	gui = win->GetGui();
	drawRect = win->drawRect;
	clientRect = win->clientRect;
	textRect = win->textRect;
	origin = win->origin;
	font = win->font;
	name = win->name;
	matScalex = win->matScalex;
	matScaley = win->matScaley;
	borderSize = win->borderSize;
	textAlign = win->textAlign;
	textAlignx = win->textAlignx;
	textAligny = win->textAligny;
	background = win->background;
	flags = win->flags;
	textShadow = win->textShadow;

	visible = win->visible;
	text = win->text;
	rect = win->rect;
	backColor = win->backColor;
	matColor = win->matColor;
	foreColor = win->foreColor;
	borderColor = win->borderColor;
	textScale = win->textScale;
	rotate = win->rotate;
	shear = win->shear;
	backGroundName = win->backGroundName;
	if (backGroundName.Length()) {
		background = declManager->FindMaterial(backGroundName);
		background->SetSort( SS_GUI );
	}
	backGroundName.SetMaterialPtr(&background);

// 
//  added parent
	mParent = win->GetParent();
// 

	hideCursor = win->hideCursor;

	hfWindow *parent = win->GetParent();
	if (parent) {
		if (text.NeedsUpdate()) {
			parent->AddUpdateVar(&text);
		}
		if (visible.NeedsUpdate()) {
			parent->AddUpdateVar(&visible);
		}
		if (rect.NeedsUpdate()) {
			parent->AddUpdateVar(&rect);
		}
		if (backColor.NeedsUpdate()) {
			parent->AddUpdateVar(&backColor);
		}
		if (matColor.NeedsUpdate()) {
			parent->AddUpdateVar(&matColor);
		}
		if (foreColor.NeedsUpdate()) {
			parent->AddUpdateVar(&foreColor);
		}
		if (borderColor.NeedsUpdate()) {
			parent->AddUpdateVar(&borderColor);
		}
		if (textScale.NeedsUpdate()) {
			parent->AddUpdateVar(&textScale);
		}
		if (rotate.NeedsUpdate()) {
			parent->AddUpdateVar(&rotate);
		}
		if (shear.NeedsUpdate()) {
			parent->AddUpdateVar(&shear);
		}
		if (backGroundName.NeedsUpdate()) {
			parent->AddUpdateVar(&backGroundName);
		}
	}
}

hfSimpleWindow::~hfSimpleWindow() {

}

void hfSimpleWindow::StateChanged( bool redraw ) {
}

void hfSimpleWindow::SetupTransforms(float x, float y) {
	static hfMat3 trans;
	static hfVec3 org;

	trans.Identity();
	org.Set( origin.x + x, origin.y + y, 0 );
	if ( rotate ) {
		static hfRotation rot;
		static hfVec3 vec( 0, 0, 1 );
		rot.Set( org, vec, rotate );
		trans = rot.ToMat3();
	}

	static hfMat3 smat;
	smat.Identity();
	if (shear.x() || shear.y()) {
		smat[0][1] = shear.x();
		smat[1][0] = shear.y();
		trans *= smat;
	}

	if ( !trans.IsIdentity() ) {
		dc->SetTransformInfo( org, trans );
	}
}

void hfSimpleWindow::DrawBackground(const hfRectangle &drawRect) {
	if (backColor.w() > 0) {
		dc->DrawFilledRect(drawRect.x, drawRect.y, drawRect.w, drawRect.h, backColor);
	}

	if (background) {
		if (matColor.w() > 0) {
			float scalex, scaley;
			if ( flags & WIN_NATURALMAT ) {
				scalex = drawRect.w / background->GetImageWidth();
				scaley = drawRect.h / background->GetImageHeight();
			} else {
				scalex = matScalex;
				scaley = matScaley;
			}
			dc->DrawMaterial(drawRect.x, drawRect.y, drawRect.w, drawRect.h, background, matColor, scalex, scaley);
		}
	}
}

void hfSimpleWindow::DrawBorderAndCaption(const hfRectangle &drawRect) {
	if (flags & WIN_BORDER) {
		if (borderSize) {
			dc->DrawRect(drawRect.x, drawRect.y, drawRect.w, drawRect.h, borderSize, borderColor);
		}
	}
}

void hfSimpleWindow::CalcClientRect(float xofs, float yofs) {

	drawRect = rect;

	if ( flags & WIN_INVERTRECT ) {
		drawRect.x = rect.x() - rect.w();
		drawRect.y = rect.y() - rect.h();
	}
	
	drawRect.x += xofs;
	drawRect.y += yofs;

	clientRect = drawRect;
	if (rect.h() > 0.0 && rect.w() > 0.0) {

		if (flags & WIN_BORDER && borderSize != 0.0) {
			clientRect.x += borderSize;
			clientRect.y += borderSize;
			clientRect.w -= borderSize;
			clientRect.h -= borderSize;
		}

		textRect = clientRect;
		textRect.x += 2.0;
	 	textRect.w -= 2.0;
		textRect.y += 2.0;
		textRect.h -= 2.0;
		textRect.x += textAlignx;
		textRect.y += textAligny;

	}
	origin.Set( rect.x() + ( rect.w() / 2 ), rect.y() + ( rect.h() / 2 ) );

}


void hfSimpleWindow::Redraw(float x, float y) {
	
	if (!visible) {
		return;
	}

	CalcClientRect(0, 0);
	dc->SetFont( font );
	drawRect.Offset(x, y);
	clientRect.Offset(x, y);
	textRect.Offset(x, y);
	SetupTransforms(x, y);
	if ( flags & WIN_NOCLIP ) {
		dc->EnableClipping( false );
	}
	DrawBackground(drawRect);
	DrawBorderAndCaption(drawRect);
	if ( textShadow ) {
		hfStr shadowText = text;
		hfRectangle shadowRect = textRect;

		shadowText.RemoveColors();
		shadowRect.x += textShadow;
		shadowRect.y += textShadow;

		dc->DrawText( shadowText, textScale, textAlign, colorBlack, shadowRect, !( flags & WIN_NOWRAP ), -1 );
	}
	dc->DrawText(text, textScale, textAlign, foreColor, textRect, !( flags & WIN_NOWRAP ), -1);
	dc->SetTransformInfo(vec3_origin, mat3_identity);
	if ( flags & WIN_NOCLIP ) {
		dc->EnableClipping( true );
	}
	drawRect.Offset(-x, -y);
	clientRect.Offset(-x, -y);
	textRect.Offset(-x, -y);
}

int hfSimpleWindow::GetWinVarOffset( hfWinVar *wv, drawWin_t* owner) {
	int ret = -1;

	if ( wv == &rect ) {
		ret = (int)(intptr_t)&( ( hfSimpleWindow * ) 0 )->rect;
	}

	if ( wv == &backColor ) {
		ret = (int)(intptr_t)&( ( hfSimpleWindow * ) 0 )->backColor;
	}

	if ( wv == &matColor ) {
		ret = (int)(intptr_t)&( ( hfSimpleWindow * ) 0 )->matColor;
	}

	if ( wv == &foreColor ) {
		ret = (int)(intptr_t)&( ( hfSimpleWindow * ) 0 )->foreColor;
	}

	if ( wv == &borderColor ) {
		ret = (int)(intptr_t)&( ( hfSimpleWindow * ) 0 )->borderColor;
	}

	if ( wv == &textScale ) {
		ret = (int)(intptr_t)&( ( hfSimpleWindow * ) 0 )->textScale;
	}

	if ( wv == &rotate ) {
		ret = (int)(intptr_t)&( ( hfSimpleWindow * ) 0 )->rotate;
	}

	if ( ret != -1 ) {
		owner->simp = this;
	}
	return ret;
}

hfWinVar *hfSimpleWindow::GetWinVarByName(const char *_name) {
	hfWinVar *retVar = NULL;
	if (hfStr::Icmp(_name, "background") == 0) {
		retVar = &backGroundName;
	}
	if (hfStr::Icmp(_name, "visible") == 0) {
		retVar = &visible;
	}
	if (hfStr::Icmp(_name, "rect") == 0) {
		retVar = &rect;
	}
	if (hfStr::Icmp(_name, "backColor") == 0) {
		retVar = &backColor;
	}
	if (hfStr::Icmp(_name, "matColor") == 0) {
		retVar = &matColor;
	}
	if (hfStr::Icmp(_name, "foreColor") == 0) {
		retVar = &foreColor;
	}
	if (hfStr::Icmp(_name, "borderColor") == 0) {
		retVar = &borderColor;
	}
	if (hfStr::Icmp(_name, "textScale") == 0) {
		retVar = &textScale;
	}
	if (hfStr::Icmp(_name, "rotate") == 0) {
		retVar = &rotate;
	}
	if (hfStr::Icmp(_name, "shear") == 0) {
		retVar = &shear;
	}
	if (hfStr::Icmp(_name, "text") == 0) {
		retVar = &text;
	}
	return retVar;
}

/*
========================
hfSimpleWindow::WriteToSaveGame
========================
*/
void hfSimpleWindow::WriteToSaveGame( hfFile *savefile ) {

	savefile->Write( &flags, sizeof( flags ) );
	savefile->Write( &drawRect, sizeof( drawRect ) );
	savefile->Write( &clientRect, sizeof( clientRect ) );
	savefile->Write( &textRect, sizeof( textRect ) );
	savefile->Write( &origin, sizeof( origin ) );
	savefile->Write( &matScalex, sizeof( matScalex ) );
	savefile->Write( &matScaley, sizeof( matScaley ) );
	savefile->Write( &borderSize, sizeof( borderSize ) );
	savefile->Write( &textAlign, sizeof( textAlign ) );
	savefile->Write( &textAlignx, sizeof( textAlignx ) );
	savefile->Write( &textAligny, sizeof( textAligny ) );
	savefile->Write( &textShadow, sizeof( textShadow ) );
	savefile->WriteString( font->GetName() );

	text.WriteToSaveGame( savefile );
	visible.WriteToSaveGame( savefile );
	rect.WriteToSaveGame( savefile );
	backColor.WriteToSaveGame( savefile );
	matColor.WriteToSaveGame( savefile );
	foreColor.WriteToSaveGame( savefile );
	borderColor.WriteToSaveGame( savefile );
	textScale.WriteToSaveGame( savefile );
	rotate.WriteToSaveGame( savefile );
	shear.WriteToSaveGame( savefile );
	backGroundName.WriteToSaveGame( savefile );

	int stringLen;

	if ( background ) {
		stringLen = strlen( background->GetName() );
		savefile->Write( &stringLen, sizeof( stringLen ) );
		savefile->Write( background->GetName(), stringLen );
	} else {
		stringLen = 0;
		savefile->Write( &stringLen, sizeof( stringLen ) );
	}

}

/*
========================
hfSimpleWindow::ReadFromSaveGame
========================
*/
void hfSimpleWindow::ReadFromSaveGame( hfFile *savefile ) {

	savefile->Read( &flags, sizeof( flags ) );
	savefile->Read( &drawRect, sizeof( drawRect ) );
	savefile->Read( &clientRect, sizeof( clientRect ) );
	savefile->Read( &textRect, sizeof( textRect ) );
	savefile->Read( &origin, sizeof( origin ) );
/*	if ( savefile->GetFileVersion() < BUILD_NUMBER_8TH_ANNIVERSARY_1 ) {
		int fontNum;
		savefile->Read( &fontNum, sizeof( fontNum ) );
		font = renderSystem->RegisterFont( "" );
	} */
	savefile->Read( &matScalex, sizeof( matScalex ) );
	savefile->Read( &matScaley, sizeof( matScaley ) );
	savefile->Read( &borderSize, sizeof( borderSize ) );
	savefile->Read( &textAlign, sizeof( textAlign ) );
	savefile->Read( &textAlignx, sizeof( textAlignx ) );
	savefile->Read( &textAligny, sizeof( textAligny ) );
	savefile->Read( &textShadow, sizeof( textShadow ) );
//	if ( savefile->GetFileVersion() >= BUILD_NUMBER_8TH_ANNIVERSARY_1 ) {
		hfStr fontName;
		savefile->ReadString( fontName );
		font = renderSystem->RegisterFont( fontName );
//	}

	text.ReadFromSaveGame( savefile );
	visible.ReadFromSaveGame( savefile );
	rect.ReadFromSaveGame( savefile );
	backColor.ReadFromSaveGame( savefile );
	matColor.ReadFromSaveGame( savefile );
	foreColor.ReadFromSaveGame( savefile );
	borderColor.ReadFromSaveGame( savefile );
	textScale.ReadFromSaveGame( savefile );
	rotate.ReadFromSaveGame( savefile );
	shear.ReadFromSaveGame( savefile );
	backGroundName.ReadFromSaveGame( savefile );

	int stringLen;

	savefile->Read( &stringLen, sizeof( stringLen ) );
	if ( stringLen > 0 ) {
		hfStr backName;

		backName.Fill( ' ', stringLen );
		savefile->Read( &(backName)[0], stringLen );

		background = declManager->FindMaterial( backName );
		background->SetSort( SS_GUI );
	} else {
		background = NULL;
	}

}


/*
===============================
*/

size_t hfSimpleWindow::Size() {
	size_t sz = sizeof(*this);
	sz += name.Size();
	sz += text.Size();
	sz += backGroundName.Size();
	return sz;
}

