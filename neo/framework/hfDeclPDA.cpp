#include "../hflib/hfprecompiled.h"
#pragma hdrstop

hfCVar g_useOldPDAStrings( "g_useOldPDAStrings", "0", CVAR_BOOL, "Read strings from the .pda files rather than from the .lang file" );

/*
=================
hfDeclPDA::Size
=================
*/
size_t hfDeclPDA::Size() const {
	return sizeof( hfDeclPDA );
}

/*
===============
hfDeclPDA::Print
===============
*/
void hfDeclPDA::Print() const {
	common->Printf( "Implement me\n" );
}

/*
===============
hfDeclPDA::List
===============
*/
void hfDeclPDA::List() const {
	common->Printf( "Implement me\n" );
}

/*
================
hfDeclPDA::Parse
================
*/
bool hfDeclPDA::Parse( const char *text, const int textLength, bool allowBinaryVersion ) {
	hfLexer src;
	hfToken token;

	hfStr baseStrId = va( "#str_%s_pda_", GetName() );

	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( DECL_LEXER_FLAGS );
	src.SkipUntilString( "{" );

	// scan through, identifying each individual parameter
	while( 1 ) {
		
		if ( !src.ReadToken( &token ) ) {
			break;
		}

		if ( token == "}" ) {
			break;
		}

		if ( !token.Icmp( "name") ) {
			src.ReadToken( &token );

			if ( g_useOldPDAStrings.GetBool() ) {
				pdaName = token;
			} else {
				pdaName = hfLocalization::GetString( baseStrId + "name" );
			}
			continue;
		}

		if ( !token.Icmp( "fullname") ) {
			src.ReadToken( &token );

			if ( g_useOldPDAStrings.GetBool() ) {
				fullName = token;
			} else {
				fullName = hfLocalization::GetString( baseStrId + "fullname" );
			}
			continue;
		}

		if ( !token.Icmp( "icon") ) {
			src.ReadToken( &token );
			icon = token;
			continue;
		}

		if ( !token.Icmp( "id") ) {
			src.ReadToken( &token );
			if ( g_useOldPDAStrings.GetBool() ) {
				id = token;
			} else {
				id = hfLocalization::GetString( baseStrId + "id" );
			}
			continue;
		}

		if ( !token.Icmp( "post") ) {
			src.ReadToken( &token );
			if ( g_useOldPDAStrings.GetBool() ) {
				post = token;
			} else {
				post = hfLocalization::GetString( baseStrId + "post" );
			}
			continue;
		}

		if ( !token.Icmp( "title") ) {
			src.ReadToken( &token );
			if ( g_useOldPDAStrings.GetBool() ) {
				title = token;
			} else {
				title = hfLocalization::GetString( baseStrId + "title" );
			}
			continue;
		}

		if ( !token.Icmp( "security") ) {
			src.ReadToken( &token );
			if ( g_useOldPDAStrings.GetBool() ) {
				security = token;
			} else {
				security = hfLocalization::GetString( baseStrId + "security" );
			}
			continue;
		}

		if ( !token.Icmp( "pda_email") ) {
			src.ReadToken( &token );
			emails.Append( static_cast<const hfDeclEmail *>( declManager->FindType( DECL_EMAIL, token ) ) );
			continue;
		}

		if ( !token.Icmp( "pda_audio") ) {
			src.ReadToken( &token );
			audios.Append( static_cast<const hfDeclAudio *>( declManager->FindType( DECL_AUDIO, token ) ) );
			continue;
		}

		if ( !token.Icmp( "pda_video") ) {
			src.ReadToken( &token );
			videos.Append( static_cast<const hfDeclVideo *>( declManager->FindType( DECL_VIDEO, token ) ) );
			continue;
		}

	}

	if ( src.HadError() ) {
		src.Warning( "PDA decl '%s' had a parse error", GetName() );
		return false;
	}

	originalVideos = videos.Num();
	originalEmails = emails.Num();
	return true;
}

/*
===================
hfDeclPDA::DefaultDefinition
===================
*/
const char *hfDeclPDA::DefaultDefinition() const {
	return
		"{\n"
		"\t"		"name  \"default pda\"\n"
		"}"; 
}

/*
===================
hfDeclPDA::FreeData
===================
*/
void hfDeclPDA::FreeData() {
	videos.Clear();
	audios.Clear();
	emails.Clear();
	originalEmails = 0;
	originalVideos = 0;
}

/*
=================
hfDeclPDA::RemoveAddedEmailsAndVideos
=================
*/
void hfDeclPDA::RemoveAddedEmailsAndVideos() const {
	int num = emails.Num();
	if ( originalEmails < num ) {
		while ( num && num > originalEmails ) {
			emails.RemoveIndex( --num );
		}
	}
	num = videos.Num();
	if ( originalVideos < num ) {
		while ( num && num > originalVideos ) {
			videos.RemoveIndex( --num );
		}
	}
}

/*
=================
hfDeclPDA::SetSecurity
=================
*/
void hfDeclPDA::SetSecurity( const char *sec ) const {
	security = sec;
}

/*
=================
hfDeclEmail::Size
=================
*/
size_t hfDeclEmail::Size() const {
	return sizeof( hfDeclEmail );
}

/*
===============
hfDeclEmail::Print
===============
*/
void hfDeclEmail::Print() const {
	common->Printf( "Implement me\n" );
}

/*
===============
hfDeclEmail::List
===============
*/
void hfDeclEmail::List() const {
	common->Printf( "Implement me\n" );
}

/*
================
hfDeclEmail::Parse
================
*/
bool hfDeclEmail::Parse( const char *_text, const int textLength, bool allowBinaryVersion ) {
	hfLexer src;
	hfToken token;

	hfStr baseStrId = va( "#str_%s_email_", GetName() );

	src.LoadMemory( _text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWPATHNAMES |	LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT | LEXFL_NOFATALERRORS );
	src.SkipUntilString( "{" );

	text = "";
	// scan through, identifying each individual parameter
	while( 1 ) {

		if ( !src.ReadToken( &token ) ) {
			break;
		}

		if ( token == "}" ) {
			break;
		}

		if ( !token.Icmp( "subject") ) {
			src.ReadToken( &token );
			if ( g_useOldPDAStrings.GetBool() ) {
				subject = token;
			} else {
				subject = hfLocalization::GetString( baseStrId + "subject" );
			}
			continue;
		}

		if ( !token.Icmp( "to") ) {
			src.ReadToken( &token );
			if ( g_useOldPDAStrings.GetBool() ) {
				to = token;
			} else {
				to = hfLocalization::GetString( baseStrId + "to" );
			}
			continue;
		}

		if ( !token.Icmp( "from") ) {
			src.ReadToken( &token );
			if ( g_useOldPDAStrings.GetBool() ) {
				from = token;
			} else {
				from = hfLocalization::GetString( baseStrId + "from" );
			}
			continue;
		}

		if ( !token.Icmp( "date") ) {
			src.ReadToken( &token );
			if ( g_useOldPDAStrings.GetBool() ) {
				date = token;
			} else {
				date = hfLocalization::GetString( baseStrId + "date" );
			}
			continue;
		}

		if ( !token.Icmp( "text") ) {
			src.ReadToken( &token );
			if ( token != "{" ) {
				src.Warning( "Email decl '%s' had a parse error", GetName() );
				return false;
			}
			while ( src.ReadToken( &token ) && token != "}" ) {
				text += token;
			}
			if ( !g_useOldPDAStrings.GetBool() ) {
				text = hfLocalization::GetString( baseStrId + "text" );
			}
			continue;
		}
	}

	if ( src.HadError() ) {
		src.Warning( "Email decl '%s' had a parse error", GetName() );
		return false;
	}
	return true;
}

/*
===================
hfDeclEmail::DefaultDefinition
===================
*/
const char *hfDeclEmail::DefaultDefinition() const {
	return
		"{\n"
		"\t"	"{\n"
		"\t\t"		"to\t5Mail recipient\n"
		"\t\t"		"subject\t5Nothing\n"
		"\t\t"		"from\t5No one\n"
		"\t"	"}\n"
		"}"; 
}

/*
===================
hfDeclEmail::FreeData
===================
*/
void hfDeclEmail::FreeData() {
}

/*
=================
hfDeclVideo::Size
=================
*/
size_t hfDeclVideo::Size() const {
	return sizeof( hfDeclVideo );
}

/*
===============
hfDeclVideo::Print
===============
*/
void hfDeclVideo::Print() const {
	common->Printf( "Implement me\n" );
}

/*
===============
hfDeclVideo::List
===============
*/
void hfDeclVideo::List() const {
	common->Printf( "Implement me\n" );
}

/*
================
hfDeclVideo::Parse
================
*/
bool hfDeclVideo::Parse( const char *text, const int textLength, bool allowBinaryVersion ) {
	hfLexer src;
	hfToken token;

	hfStr baseStrId = va( "#str_%s_video_", GetName() );

	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWPATHNAMES |	LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT | LEXFL_NOFATALERRORS );
	src.SkipUntilString( "{" );

	// scan through, identifying each individual parameter
	while( 1 ) {

		if ( !src.ReadToken( &token ) ) {
			break;
		}

		if ( token == "}" ) {
			break;
		}

		if ( !token.Icmp( "name") ) {
			src.ReadToken( &token );
			if ( g_useOldPDAStrings.GetBool() ) {
				videoName = token;
			} else {
				videoName = hfLocalization::GetString( baseStrId + "name" );
			}
			continue;
		}

		if ( !token.Icmp( "preview") ) {
			src.ReadToken( &token );
			preview = declManager->FindMaterial( token );
			continue;
		}

		if ( !token.Icmp( "video") ) {
			src.ReadToken( &token );
			video = declManager->FindMaterial( token );
			continue;
		}

		if ( !token.Icmp( "info") ) {
			src.ReadToken( &token );
			if ( g_useOldPDAStrings.GetBool() ) {
				info = token;
			} else {
				info = hfLocalization::GetString( baseStrId + "info" );
			}
			continue;
		}

		if ( !token.Icmp( "audio") ) {
			src.ReadToken( &token );
			audio = declManager->FindSound( token );
			continue;
		}

	}

	if ( src.HadError() ) {
		src.Warning( "Video decl '%s' had a parse error", GetName() );
		return false;
	}
	return true;
}

/*
===================
hfDeclVideo::DefaultDefinition
===================
*/
const char *hfDeclVideo::DefaultDefinition() const {
	return
		"{\n"
		"\t"	"{\n"
		"\t\t"		"name\t5Default Video\n"
		"\t"	"}\n"
		"}"; 
}

/*
===================
hfDeclVideo::FreeData
===================
*/
void hfDeclVideo::FreeData() {
}

/*
=================
hfDeclAudio::Size
=================
*/
size_t hfDeclAudio::Size() const {
	return sizeof( hfDeclAudio );
}

/*
===============
hfDeclAudio::Print
===============
*/
void hfDeclAudio::Print() const {
	common->Printf( "Implement me\n" );
}

/*
===============
hfDeclAudio::List
===============
*/
void hfDeclAudio::List() const {
	common->Printf( "Implement me\n" );
}

/*
================
hfDeclAudio::Parse
================
*/
bool hfDeclAudio::Parse( const char *text, const int textLength, bool allowBinaryVersion ) {
	hfLexer src;
	hfToken token;

	hfStr baseStrId = va( "#str_%s_audio_", GetName() );

	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWPATHNAMES |	LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT | LEXFL_NOFATALERRORS );
	src.SkipUntilString( "{" );

	// scan through, identifying each individual parameter
	while( 1 ) {

		if ( !src.ReadToken( &token ) ) {
			break;
		}

		if ( token == "}" ) {
			break;
		}

		if ( !token.Icmp( "name") ) {
			src.ReadToken( &token );
			if ( g_useOldPDAStrings.GetBool() ) {
				audioName = token;
			} else {
				audioName = hfLocalization::GetString( baseStrId + "name" );
			}
			continue;
		}

		if ( !token.Icmp( "audio") ) {
			src.ReadToken( &token );
			audio = declManager->FindSound( token );
			continue;
		}

		if ( !token.Icmp( "info") ) {
			src.ReadToken( &token );
			if ( g_useOldPDAStrings.GetBool() ) {
				info = token;
			} else {
				info = hfLocalization::GetString( baseStrId + "info" );
			}
			continue;
		}
	}

	if ( src.HadError() ) {
		src.Warning( "Audio decl '%s' had a parse error", GetName() );
		return false;
	}
	return true;
}

/*
===================
hfDeclAudio::DefaultDefinition
===================
*/
const char *hfDeclAudio::DefaultDefinition() const {
	return
		"{\n"
		"\t"	"{\n"
		"\t\t"		"name\t5Default Audio\n"
		"\t"	"}\n"
		"}"; 
}

/*
===================
hfDeclAudio::FreeData
===================
*/
void hfDeclAudio::FreeData() {
}

