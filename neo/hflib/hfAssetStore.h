#ifndef __HF_ASSET_STORE_H__
#define __HF_ASSET_STORE_H__

#include "hfStr.h"

// Versioned binary asset store + lightweight .mtr material parser used by
// the offline asset tool (neo/tools/hfAssetTool.cpp), the engine (optional
// streaming front end) and the headless harness. No filesystem or GL
// dependency: everything goes through byte buffers so it is fully testable
// offline. All integer fields are stored little-endian.

namespace HF_Asset {

static const unsigned int	ASSET_MAGIC		= 0x32334e58;	// 'XN32'
static const unsigned int	ASSET_VERSION	= 2;			// v2 adds an embedded name table

struct AssetEntry {
	unsigned int		nameHash;	// Adler32 of the lower-cased name
	unsigned int		offset;		// byte offset of data within the store
	unsigned int		size;		// raw data bytes
	unsigned int		adler;		// Adler32 of the data
	unsigned short		flags;		// bit0 = streaming mips present, bit1 = compressed
	unsigned short		mipCount;	// texture stores: number of mips
};

struct AssetSource {
	const char *		name;
	const void *		data;
	unsigned int		size;
};

unsigned int	Adler32( const void *data, int length );
unsigned int	LowerCaseHash( const char *name );

// Pack produces a binary store (NUL bytes make it unsuited for hfStr);
// callers must FreeStore() the result.
struct PackedStore {
	byte *			data;
	int				size;
	bool			ok;
};

PackedStore		Pack( const AssetSource *items, int count );
void			FreeStore( PackedStore & store );

// Returns false when the store blob is malformed (magic/version/size checks).
bool			Validate( const byte *store, int storeSize, hfStr *error = NULL );

// Extract one asset by name. Returns false when missing.
bool			Extract( const byte *store, int storeSize, const char *name, hfStr &out, AssetEntry *meta = NULL );

// Append every asset name to 'out' (one per line).
void			List( const byte *store, int storeSize, hfStr &out );

// --- .mtr material parsing (shared by the tool and the engine preview) ----
enum mtrBlend_t {
	MBLEND_NONE = 0,
	MBLEND_MODULATE,
	MBLEND_ADD,
	MBLEND_ALPHA,
	MBLEND_BLEND
};

struct mtrStage_t {
	hfStr			textureName;	// map / cubeMap argument
	hfStr			blendFunc;		// raw blendfunc argument
	hfStr			texGen;			// raw vertexTexGen argument ('' = none)
	int				blend;			// mtrBlend_t
	bool			hasColor;

	void			Reset() { textureName = ""; blendFunc = ""; texGen = ""; blend = MBLEND_NONE; hasColor = false; }
};

struct mtrDef_t {
	hfStr			name;
	mtrStage_t		stages[8];
	int				numStages;
	bool			noShadows;
	bool			polygonOffset;
	const char *	unk;
	bool			parseOk;

	void			Reset() { name = ""; numStages = 0; noShadows = false; polygonOffset = false; parseOk = false; }
};

// Parse a .mtr text buffer into declarations. Returns the number of defs
// parsed (bounded by maxDefs). Skips comments; tolerant of unknown keywords.
int				ParseMtr( const char *text, int textSize, mtrDef_t *defs, int maxDefs );

// Collect every distinct texture name referenced by the defs.
void			CollectTextures( const mtrDef_t *defs, int numDefs, hfStr &names );

// --- GLSL source sanity checker (offline replacement for glslang when a
//     validator binary is not installed) -----------------------------------
// Balance-checks braces/parens and #if/#endif; verifies 'void main'.
bool			GlslSanityCheck( const char *src, hfStr *error = NULL );

} // namespace HF_Asset

#endif