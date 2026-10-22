#include "hfprecompiled.h"
#include "hfAssetStore.h"

namespace HF_Asset {

/*
====================
Adler32
====================
*/
unsigned int Adler32( const void *data, int length ) {
	const byte *p = ( const byte * )data;
	unsigned int a = 1, b = 0;
	for ( int i = 0; i < length; i++ ) {
		a = ( a + p[i] ) % 65521;
		b = ( b + a ) % 65521;
	}
	return ( b << 16 ) | a;
}

/*
====================
LowerCaseHash
====================
*/
unsigned int LowerCaseHash( const char *name ) {
	hfStr lower = name;
	lower.ToLower();
	return Adler32( lower.c_str(), lower.Length() );
}

/*
====================
Pack
====================
*/
PackedStore Pack( const AssetSource *items, int count ) {
	PackedStore result;
	result.data = NULL;
	result.size = 0;
	result.ok = false;
	if ( !items || count <= 0 ) { return result; }
	// Reject duplicate names and compute offsets.
	unsigned int dataSize = 0;
	for ( int i = 0; i < count; i++ ) {
		unsigned int hash = LowerCaseHash( items[i].name );
		for ( int j = 0; j < i; j++ ) {
			if ( LowerCaseHash( items[j].name ) == hash ) { return result; }
		}
		dataSize += ( items[i].size + 3 ) & ~3u;
	}
	int namesSize = 0;
	for ( int i = 0; i < count; i++ ) {
		namesSize += 2 + ( int )strlen( items[i].name ) + 1;
	}

	int headerSize = 12 + count * ( int )sizeof( AssetEntry );
	int totalBytes = headerSize + namesSize + dataSize;
	byte *buf = new byte[totalBytes];

	// header
	memcpy( buf, &ASSET_MAGIC, 4 );
	memcpy( buf + 4, &ASSET_VERSION, 4 );
	memcpy( buf + 8, &count, 4 );

	// names block: [u16 len][name bytes (len incl null)] per entry, sequential
	byte * nameBase = buf + headerSize;
	int nameCursor = 0;
	for ( int i = 0; i < count; i++ ) {
		int len = ( int )strlen( items[i].name ) + 1;
		unsigned short slen = ( unsigned short )len;
		memcpy( nameBase + nameCursor, &slen, 2 );
		memcpy( nameBase + nameCursor + 2, items[i].name, len );
		nameCursor += 2 + len;
	}

	unsigned int offset = ( unsigned int )( headerSize + namesSize );
	for ( int i = 0; i < count; i++ ) {
		AssetEntry e;
		e.nameHash = LowerCaseHash( items[i].name );
		e.offset = offset;
		e.size = items[i].size;
		e.adler = Adler32( items[i].data, items[i].size );
		e.flags = 0;
		e.mipCount = 1;
		memcpy( buf + 12 + i * ( int )sizeof( AssetEntry ), &e, sizeof( e ) );
		memcpy( buf + offset, items[i].data, items[i].size );
		offset += ( items[i].size + 3 ) & ~3u;
	}
	result.data = buf;
	result.size = totalBytes;
	result.ok = true;
	return result;
}

void FreeStore( PackedStore & store ) {
	delete[] store.data;
	store.data = NULL;
	store.size = 0;
	store.ok = false;
}

/*
====================
Validate
====================
*/
bool Validate( const byte *store, int storeSize, hfStr *error ) {
	if ( !store || storeSize < 12 ) {
		if ( error ) { *error = "store too small"; }
		return false;
	}
	unsigned int magic, version, entryCount;
	memcpy( &magic, store, 4 );
	memcpy( &version, store + 4, 4 );
	memcpy( &entryCount, store + 8, 4 );
	if ( magic != ASSET_MAGIC ) {
		if ( error ) { *error = "bad magic"; }
		return false;
	}
	if ( version != ASSET_VERSION ) {
		if ( error ) { *error = "unsupported version"; }
		return false;
	}
	if ( entryCount > 100000 ) {
		if ( error ) { *error = "suspicious entry count"; }
		return false;
	}
	if ( storeSize < 12 + ( int )entryCount * ( int )sizeof( AssetEntry ) ) {
		if ( error ) { *error = "table overruns buffer"; }
		return false;
	}
	// names block lives between the table and the lowest data offset
	AssetEntry first;
	memcpy( &first, store + 12, sizeof( first ) );
	int nameRegion = ( int )first.offset - ( 12 + ( int )entryCount * ( int )sizeof( AssetEntry ) );
	if ( nameRegion < 2 * ( int )entryCount ) {
		if ( error ) { *error = "names region too small"; }
		return false;
	}
	// verify every entry lies inside the blob and hashes to itself; also
	// validate the name table
	unsigned int cursor = 0;
	for ( unsigned int i = 0; i < entryCount; i++ ) {
		AssetEntry e;
		memcpy( &e, store + 12 + i * ( int )sizeof( AssetEntry ), sizeof( e ) );
		if ( e.offset + e.size > ( unsigned int )storeSize ) {
			if ( error ) { *error = va( "entry %u overruns buffer", i ); }
			return false;
		}
		if ( e.adler != Adler32( store + e.offset, e.size ) ) {
			if ( error ) { *error = va( "entry %u adler mismatch", i ); }
			return false;
		}
		// name table (interleaved [u16 len][bytes] per entry)
		const byte *np = store + 12 + ( int )entryCount * ( int )sizeof( AssetEntry );
		// find this entry's payload by walking
		unsigned int c0 = 0;
		for ( unsigned int j = 0; j <= i; j++ ) {
			unsigned short nlen;
			memcpy( &nlen, np + c0, 2 );
			if ( nlen < 1 ) { if ( error ) { *error = va( "entry %u bad name len", j ); } return false; }
			c0 += 2 + nlen;
		}
		cursor = c0;
	}
	if ( cursor > ( unsigned int )nameRegion ) {
		if ( error ) { *error = "names region overruns data"; }
		return false;
	}
	return true;
}

// returns the length-prefixed stored name for entry index i
static const char * StoredName( const byte *store, unsigned int entryCount, unsigned int i ) {
	const byte *np = store + 12 + entryCount * ( unsigned int )sizeof( AssetEntry );
	unsigned int c = 0;
	for ( unsigned int j = 0; j < i; j++ ) {
		unsigned short nlen;
		memcpy( &nlen, np + c, 2 );
		c += 2 + nlen;
	}
	return ( const char * )( np + c + 2 );
}

/*
====================
Extract
====================
*/
bool Extract( const byte *store, int storeSize, const char *name, hfStr &out, AssetEntry *meta ) {
	if ( !Validate( store, storeSize ) ) { return false; }
	unsigned int entryCount;
	memcpy( &entryCount, store + 8, 4 );
	unsigned int want = LowerCaseHash( name );
	for ( unsigned int i = 0; i < entryCount; i++ ) {
		AssetEntry e;
		memcpy( &e, store + 12 + i * ( int )sizeof( AssetEntry ), sizeof( e ) );
		const char * stored = StoredName( store, entryCount, i );
		if ( e.nameHash == want && hfStr::Icmp( stored, name ) == 0 ) {
			out.Clear();
			out.Append( ( const char * )( store + e.offset ), e.size );
			if ( meta ) { *meta = e; }
			return true;
		}
	}
	return false;
}

/*
====================
List
====================
*/
void List( const byte *store, int storeSize, hfStr &out ) {
	if ( !Validate( store, storeSize ) ) { return; }
	unsigned int entryCount;
	memcpy( &entryCount, store + 8, 4 );
	out.Clear();
	for ( unsigned int i = 0; i < entryCount; i++ ) {
		AssetEntry e;
		memcpy( &e, store + 12 + i * ( int )sizeof( AssetEntry ), sizeof( e ) );
		const char * stored = StoredName( store, entryCount, i );
		out += va( "%8u  %s\n", e.size, stored );
	}
}

/*
====================
ParseMtr

Grammar handled:  comments (#... and /*...* /), then
  materialName
  {
    keyword args...
    stageKeyword
    {
      map texturePath
      blendFunc ...
    }
  }
Any unknown top-level keyword is skipped until the next line so forward
compat with engine additions does not break the tool.
====================
*/
int ParseMtr( const char *text, int textSize, mtrDef_t *defs, int maxDefs ) {
	if ( !text || !defs || maxDefs <= 0 ) { return 0; }
	for ( int i = 0; i < maxDefs; i++ ) { defs[i].Reset(); }

	int count = 0;
	int pos = 0;
	hfStr line;

	while ( pos < textSize && count < maxDefs ) {
		// skip whitespace, newlines and comments to the next token
		for ( ; pos < textSize; pos++ ) {
			char c = text[pos];
			if ( c == '#' ) {
				while ( pos < textSize && text[pos] != '\n' ) { pos++; }
			} else if ( c == '/' && pos + 1 < textSize && text[pos + 1] == '*' ) {
				pos += 2;
				while ( pos + 1 < textSize && !( text[pos] == '*' && text[pos + 1] == '/' ) ) { pos++; }
				pos++;	// past the closing '/' (loop ++ handles the rest)
				if ( pos >= textSize ) { break; }
			} else if ( c != ' ' && c != '\t' && c != '\r' && c != '\n' ) {
				break;
			}
		}
		if ( pos >= textSize ) { break; }

		// read the declaration name up to '{' or whitespace
		hfStr name;
		while ( pos < textSize ) {
			char c = text[pos];
			if ( c == '{' || c == ' ' || c == '\t' || c == '\r' || c == '\n' ) { break; }
			name += c;
			pos++;
		}
		while ( pos < textSize && text[pos] != '{' && text[pos] != '\n' ) { pos++; }
		if ( pos >= textSize || text[pos] != '{' ) { pos++; continue; }	// malformed, skip
		pos++;	// past '{'

		mtrDef_t &def = defs[count];
		def.name = name;
		def.parseOk = true;

		// consume until matching '}'
		int depth = 1;
		while ( pos < textSize && depth > 0 ) {
			// skip comment noise inside the body
			if ( text[pos] == '#' ) {
				while ( pos < textSize && text[pos] != '\n' ) { pos++; }
				continue;
			}
			if ( text[pos] == '{' ) { depth++; pos++; continue; }
			if ( text[pos] == '}' ) { depth--; pos++; continue; }

			// read a keyword
			hfStr keyword;
			while ( pos < textSize && text[pos] > ' ' && text[pos] != '{' ) { keyword += text[pos++]; }
			if ( keyword.Length() == 0 ) { pos++; continue; }
			keyword.ToLower();

if ( keyword == "stages" || keyword == "stage" ) {
			while ( pos < textSize && text[pos] != '{' && text[pos] != '}' ) { pos++; }
			if ( pos >= textSize ) { break; }
			if ( text[pos] == '}' ) { continue; }
			pos++;
			// shift down so 'stages' becomes a *new* stage block
			int si = def.numStages < 7 ? def.numStages : 0;
			if ( def.numStages < 8 ) { def.numStages++; }
			mtrStage_t &s = def.stages[si];
			s.Reset();
			s.blend = MBLEND_MODULATE;	// engine default
			while ( pos < textSize && text[pos] != '}' ) {
				hfStr k;
				while ( pos < textSize && text[pos] <= ' ' ) { pos++; }
				while ( pos < textSize && text[pos] > ' ' && text[pos] != '}' ) { k += text[pos++]; }
				if ( k.Length() == 0 ) { continue; }
				k.ToLower();
				if ( k == "map" || k == "cubeMap" ) {
					hfStr v;
					while ( pos < textSize && text[pos] <= ' ' ) { pos++; }
					while ( pos < textSize && text[pos] > ' ' && text[pos] != '}' ) { v += text[pos++]; }
					s.textureName = v;
				} else if ( k == "blendfunc" ) {
					hfStr v;
					while ( pos < textSize && text[pos] <= ' ' ) { pos++; }
					while ( pos < textSize && text[pos] > ' ' && text[pos] != '}' ) { v += text[pos++]; }
						s.blendFunc = v;
						if ( v.Icmp( "add" ) == 0 ) { s.blend = MBLEND_ADD; }
						else if ( v.Icmp( "modulate" ) == 0 ) { s.blend = MBLEND_MODULATE; }
						else if ( v.Icmp( "blend" ) == 0 || v.Icmp( "alpha" ) == 0 ) { s.blend = MBLEND_ALPHA; }
						else { s.blend = MBLEND_BLEND; }
				} else if ( k == "vertextexgen" ) {
					hfStr v;
					while ( pos < textSize && text[pos] <= ' ' ) { pos++; }
					while ( pos < textSize && text[pos] > ' ' && text[pos] != '}' ) { v += text[pos++]; }
					s.texGen = v;
					} else if ( k == "colored" ) {
						s.hasColor = true;
					} else {
						// consume single argument and ignore
						while ( pos < textSize && text[pos] > ' ' && text[pos] != '}' ) { pos++; }
					}
				}
				if ( pos < textSize ) { pos++; }	// past '}'
			} else if ( keyword == "noshadows" ) {
				def.noShadows = true;
			} else if ( keyword == "polygonoffset" ) {
				def.polygonOffset = true;
			} else {
				// unknown keyword: consume the rest of the line
				while ( pos < textSize && text[pos] != '\n' ) { pos++; }
			}
		}
		count++;
	}
	return count;
}

/*
====================
CollectTextures
====================
*/
void CollectTextures( const mtrDef_t *defs, int numDefs, hfStr &names ) {
	names.Clear();
	for ( int d = 0; d < numDefs; d++ ) {
		for ( int s = 0; s < defs[d].numStages; s++ ) {
			const hfStr &tex = defs[d].stages[s].textureName;
			if ( tex.Length() == 0 ) { continue; }
			if ( names.Find( tex.c_str() ) < 0 ) { names += tex + "\n"; }
		}
	}
}

/*
====================
GlslSanityCheck
====================
*/
bool GlslSanityCheck( const char *src, hfStr *error ) {
	if ( !src || src[0] == 0 ) {
		if ( error ) { *error = "empty source"; }
		return false;
	}
	int brace = 0, paren = 0, bracket = 0, define = 0;
	bool sawMain = false;
	const char *p = src;
	for ( ; *p; p++ ) {
		switch ( *p ) {
			case '{': brace++; break;
			case '}': brace--; break;
			case '(': paren++; break;
			case ')': paren--; break;
			case '[': bracket++; break;
			case ']': bracket--; break;
			default: break;
		}
		if ( brace < 0 || paren < 0 || bracket < 0 ) {
			if ( error ) { *error = "unbalanced closing delimiter"; }
			return false;
		}
	}
	if ( error && ( brace != 0 || paren != 0 || bracket != 0 ) ) {
		*error = va( "'{' %d  '(' %d  '[' %d", brace, paren, bracket );
	}
	if ( brace != 0 || paren != 0 || bracket != 0 ) { return false; }

	// naive '#if'/'#endif' balance
	int ifdef = 0;
	const char *q = src;
	while ( ( q = strstr( q, "#if" ) ) != NULL ) { ifdef++; q += 3; }
	// count #endif strictly after #ifs could allow stray endif; do a dirty check:
	int endif = 0;
	q = src;
	while ( ( q = strstr( q, "#endif" ) ) != NULL ) { endif++; q += 6; }
	if ( error && ifdef != endif ) { *error = va( "#if %d vs #endif %d", ifdef, endif ); }
	if ( ifdef != endif ) { return false; }

	if ( strstr( src, "void main" ) == NULL ) {
		if ( error ) { *error = "missing 'void main'"; }
		return false;
	}
	if ( error ) { *error = ""; }
	return true;
}

} // namespace HF_Asset