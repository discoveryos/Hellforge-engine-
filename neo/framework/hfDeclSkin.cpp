#include "../hflib/hfprecompiled.h"
#pragma hdrstop


/*
=================
hfDeclSkin::Size
=================
*/
size_t hfDeclSkin::Size() const {
	return sizeof( hfDeclSkin );
}

/*
================
hfDeclSkin::FreeData
================
*/
void hfDeclSkin::FreeData() {
	mappings.Clear();
}

/*
================
hfDeclSkin::Parse
================
*/
bool hfDeclSkin::Parse( const char *text, const int textLength, bool allowBinaryVersion ) {
	hfLexer src;
	hfToken	token, token2;

	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( DECL_LEXER_FLAGS );
	src.SkipUntilString( "{" );

	associatedModels.Clear();

	while (1) {
		if ( !src.ReadToken( &token ) ) {
			break;
		}

		if ( !token.Icmp( "}" ) ) {
			break;
		}
		if ( !src.ReadToken( &token2 ) ) {
			src.Warning( "Unexpected end of file" );
			MakeDefault();
			return false;
		}

		if ( !token.Icmp( "model" ) ) {
			associatedModels.Append( token2 );
			continue;
		}

		skinMapping_t	map;

		if ( !token.Icmp( "*" ) ) {
			// wildcard
			map.from = NULL;
		} else {
			map.from = declManager->FindMaterial( token );
		}

		map.to = declManager->FindMaterial( token2 );

		mappings.Append( map );
	}

	return false;
}

/*
================
hfDeclSkin::SetDefaultText
================
*/
bool hfDeclSkin::SetDefaultText() {
	// if there exists a material with the same name
	if ( declManager->FindType( DECL_MATERIAL, GetName(), false ) ) {
		char generated[2048];

		hfStr::snPrintf( generated, sizeof( generated ),
						"skin %s // IMPLICITLY GENERATED\n"
						"{\n"
						"_default %s\n"
						"}\n", GetName(), GetName() );
		SetText( generated );
		return true;
	} else {
		return false;
	}
}

/*
================
hfDeclSkin::DefaultDefinition
================
*/
const char *hfDeclSkin::DefaultDefinition() const {
	return
		"{\n"
	"\t"	"\"*\"\t\"_default\"\n"
		"}";
}

/*
================
hfDeclSkin::GetNumModelAssociations
================
*/
const int hfDeclSkin::GetNumModelAssociations(void ) const {
	return associatedModels.Num(); 
}

/*
================
hfDeclSkin::GetAssociatedModel
================
*/
const char *hfDeclSkin::GetAssociatedModel( int index ) const {
	if ( index >= 0 && index < associatedModels.Num() ) {
		return associatedModels[ index ];
	}
	return "";
}

/*
===============
RemapShaderBySkin
===============
*/
const hfMaterial *hfDeclSkin::RemapShaderBySkin( const hfMaterial *shader ) const {
	int		i;

	if ( !shader ) {
		return NULL;
	}

	// never remap surfaces that were originally nodraw, like collision hulls
	if ( !shader->IsDrawn() ) {
		return shader;
	}

	for ( i = 0; i < mappings.Num() ; i++ ) {
		const skinMapping_t	*map = &mappings[i];

		// NULL = wildcard match
		if ( !map->from || map->from == shader ) {
			return map->to;
		}
	}

	// didn't find a match or wildcard, so stay the same
	return shader;
}

