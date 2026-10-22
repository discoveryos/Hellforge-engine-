#ifndef __HF_UI_BITMAP_FONT_H__
#define __HF_UI_BITMAP_FONT_H__

/*
===============================================================================

	hfBitmapFont — Phase 9, the CPU text rasterizer.

	A tiny fixed-cell 5x7 bitmap font. Every printable ASCII character gets a
	glyph; lowercase folds to uppercase (the usual small-font convention) and
	anything undefined falls back to a hollow box, so metrics never depend on
	the glyph table being complete.

	This is the "first" text implementation the roadmap calls for: pure CPU,
	no font file, no GPU, so the harness can measure and rasterize text
	headlessly. A real glyph atlas can replace Glyph() later without moving
	the metrics or the wrapping, which are what layout actually depends on.

	Metrics: advance 6px (5px glyph + 1px gap), line height 9px. Measurement
	and wrapping are done on those numbers, not on the pixels.

===============================================================================
*/

#include <string.h>

namespace HF_Ui {

static const int HF_UI_GLYPH_W		= 5;
static const int HF_UI_GLYPH_H		= 7;
static const int HF_UI_ADVANCE		= 6;
static const int HF_UI_LINE_HEIGHT	= 9;

// one wrapped line: [start, start+length) of the source string, and its pixel width
struct hfUiTextLine {
	int		start;
	int		length;
	int		width;
};

// a caller-owned 8-bit coverage buffer (0 = empty, 255 = solid)
struct hfUiCoverage {
	int					width;
	int					height;
	unsigned char *		pixels;		// width * height, row-major

	HF_INLINE void Clear() {
		if ( pixels ) { memset( pixels, 0, ( size_t )width * ( size_t )height ); }
	}
	HF_INLINE unsigned char At( int x, int y ) const {
		if ( x < 0 || y < 0 || x >= width || y >= height || !pixels ) { return 0; }
		return pixels[ y * width + x ];
	}
	HF_INLINE void Set( int x, int y, unsigned char v ) {
		if ( x < 0 || y < 0 || x >= width || y >= height || !pixels ) { return; }
		unsigned char &p = pixels[ y * width + x ];
		if ( v > p ) { p = v; }
	}
};

class hfBitmapFont {
public:
	// 7 rows, 5 bits per row (bit 4 = leftmost pixel)
	HF_INLINE static const unsigned char * Glyph( char c ) {
		static const unsigned char kBox[ 7 ]    = { 0x1F,0x11,0x11,0x11,0x11,0x11,0x1F };
		static const unsigned char kSpace[ 7 ]  = { 0,0,0,0,0,0,0 };
		static const unsigned char k0[ 7 ]      = { 0x0E,0x11,0x13,0x15,0x19,0x11,0x0E };
		static const unsigned char k1[ 7 ]      = { 0x04,0x0C,0x04,0x04,0x04,0x04,0x0E };
		static const unsigned char k2[ 7 ]      = { 0x0E,0x11,0x01,0x02,0x04,0x08,0x1F };
		static const unsigned char k3[ 7 ]      = { 0x1F,0x02,0x04,0x02,0x01,0x11,0x0E };
		static const unsigned char k4[ 7 ]      = { 0x02,0x06,0x0A,0x12,0x1F,0x02,0x02 };
		static const unsigned char k5[ 7 ]      = { 0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E };
		static const unsigned char k6[ 7 ]      = { 0x06,0x08,0x10,0x1E,0x11,0x11,0x0E };
		static const unsigned char k7[ 7 ]      = { 0x1F,0x01,0x02,0x04,0x08,0x08,0x08 };
		static const unsigned char k8[ 7 ]      = { 0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E };
		static const unsigned char k9[ 7 ]      = { 0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C };
		static const unsigned char kDot[ 7 ]    = { 0,0,0,0,0,0x0C,0x0C };
		static const unsigned char kComma[ 7 ]  = { 0,0,0,0,0x0C,0x04,0x08 };
		static const unsigned char kBang[ 7 ]   = { 0x04,0x04,0x04,0x04,0x04,0x00,0x04 };
		static const unsigned char kQuery[ 7 ]  = { 0x0E,0x11,0x01,0x02,0x04,0x00,0x04 };
		static const unsigned char kDash[ 7 ]   = { 0,0,0,0x1F,0,0,0 };
		static const unsigned char kColon[ 7 ]  = { 0,0x0C,0x0C,0,0x0C,0x0C,0 };
		static const unsigned char kA[ 7 ]      = { 0x0E,0x11,0x11,0x1F,0x11,0x11,0x11 };
		static const unsigned char kB[ 7 ]      = { 0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E };
		static const unsigned char kC[ 7 ]      = { 0x0E,0x11,0x10,0x10,0x10,0x11,0x0E };
		static const unsigned char kD[ 7 ]      = { 0x1C,0x12,0x11,0x11,0x11,0x12,0x1C };
		static const unsigned char kE[ 7 ]      = { 0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F };
		static const unsigned char kF[ 7 ]      = { 0x1F,0x10,0x10,0x1E,0x10,0x10,0x10 };
		static const unsigned char kG[ 7 ]      = { 0x0E,0x11,0x10,0x17,0x11,0x11,0x0F };
		static const unsigned char kH[ 7 ]      = { 0x11,0x11,0x11,0x1F,0x11,0x11,0x11 };
		static const unsigned char kI[ 7 ]      = { 0x0E,0x04,0x04,0x04,0x04,0x04,0x0E };
		static const unsigned char kJ[ 7 ]      = { 0x07,0x02,0x02,0x02,0x02,0x12,0x0C };
		static const unsigned char kK[ 7 ]      = { 0x11,0x12,0x14,0x18,0x14,0x12,0x11 };
		static const unsigned char kL[ 7 ]      = { 0x10,0x10,0x10,0x10,0x10,0x10,0x1F };
		static const unsigned char kM[ 7 ]      = { 0x11,0x1B,0x15,0x15,0x11,0x11,0x11 };
		static const unsigned char kN[ 7 ]      = { 0x11,0x11,0x19,0x15,0x13,0x11,0x11 };
		static const unsigned char kO[ 7 ]      = { 0x0E,0x11,0x11,0x11,0x11,0x11,0x0E };
		static const unsigned char kP[ 7 ]      = { 0x1E,0x11,0x11,0x1E,0x10,0x10,0x10 };
		static const unsigned char kQ[ 7 ]      = { 0x0E,0x11,0x11,0x11,0x15,0x12,0x0D };
		static const unsigned char kR[ 7 ]      = { 0x1E,0x11,0x11,0x1E,0x14,0x12,0x11 };
		static const unsigned char kS[ 7 ]      = { 0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E };
		static const unsigned char kT[ 7 ]      = { 0x1F,0x04,0x04,0x04,0x04,0x04,0x04 };
		static const unsigned char kU[ 7 ]      = { 0x11,0x11,0x11,0x11,0x11,0x11,0x0E };
		static const unsigned char kV[ 7 ]      = { 0x11,0x11,0x11,0x11,0x11,0x0A,0x04 };
		static const unsigned char kW[ 7 ]      = { 0x11,0x11,0x11,0x15,0x15,0x1B,0x11 };
		static const unsigned char kX[ 7 ]      = { 0x11,0x11,0x0A,0x04,0x0A,0x11,0x11 };
		static const unsigned char kY[ 7 ]      = { 0x11,0x11,0x0A,0x04,0x04,0x04,0x04 };
		static const unsigned char kZ[ 7 ]      = { 0x1F,0x01,0x02,0x04,0x08,0x10,0x1F };

		unsigned char c2 = ( unsigned char )c;
		if ( c2 >= 'a' && c2 <= 'z' ) { c2 = ( unsigned char )( c2 - 'a' + 'A' ); }
		switch ( c2 ) {
			case ' ':	return kSpace;
			case '0':	return k0; case '1': return k1; case '2': return k2; case '3': return k3;
			case '4':	return k4; case '5': return k5; case '6': return k6; case '7': return k7;
			case '8':	return k8; case '9': return k9;
			case '.':	return kDot;
			case ',':	return kComma;
			case '!':	return kBang;
			case '?':	return kQuery;
			case ':':	return kColon;
			case '-':	return kDash;
			case 'A':	return kA; case 'B': return kB; case 'C': return kC; case 'D': return kD;
			case 'E':	return kE; case 'F': return kF; case 'G': return kG; case 'H': return kH;
			case 'I':	return kI; case 'J': return kJ; case 'K': return kK; case 'L': return kL;
			case 'M':	return kM; case 'N': return kN; case 'O': return kO; case 'P': return kP;
			case 'Q':	return kQ; case 'R': return kR; case 'S': return kS; case 'T': return kT;
			case 'U':	return kU; case 'V': return kV; case 'W': return kW; case 'X': return kX;
			case 'Y':	return kY; case 'Z': return kZ;
			default:	return kBox;
		}
	}

	// width of [text, text+len) on one line, in pixels
	HF_INLINE static int MeasureLine( const char *text, int len ) {
		( void )text;
		return len > 0 ? len * HF_UI_ADVANCE : 0;
	}

	// width of the longest line in text (text may contain '\n'), in pixels
	HF_INLINE static int MeasureText( const char *text, int len = -1 ) {
		if ( !text ) { return 0; }
		if ( len < 0 ) { len = ( int )strlen( text ); }
		int longest = 0;
		int lineStart = 0;
		for ( int i = 0; i <= len; i++ ) {
			if ( i == len || text[ i ] == '\n' ) {
				int w = MeasureLine( text + lineStart, i - lineStart );
				if ( w > longest ) { longest = w; }
				lineStart = i + 1;
			}
		}
		return longest;
	}

	// Greedy word wrap. Words are runs of non-space characters; a run longer
	// than maxWidth is allowed to overflow its line rather than be split (a
	// real implementation would hyphenate). Explicit '\n' forces a break.
	// Returns the number of lines; the first min(count,maxLines) are written
	// when lines != NULL. Empty text yields 0 lines.
	HF_INLINE static int WrapText( const char *text, int maxWidth,
								   hfUiTextLine *lines, int maxLines ) {
		if ( !text ) { return 0; }
		int len = ( int )strlen( text );
		if ( len == 0 ) { return 0; }

		int count = 0;
		int p = 0;
		while ( p <= len ) {
			int paraEnd = p;
			while ( paraEnd < len && text[ paraEnd ] != '\n' ) { paraEnd++; }

			int lineStart = p;
			int lineLen = 0;
			int i = p;
			while ( i < paraEnd ) {
				while ( i < paraEnd && text[ i ] == ' ' ) { i++; }
				if ( i >= paraEnd ) { break; }
				int wordStart = i;
				while ( i < paraEnd && text[ i ] != ' ' ) { i++; }
				int wordEnd = i;
				if ( lineLen == 0 ) {
					lineStart = wordStart;
					lineLen = wordEnd - wordStart;
				} else if ( ( wordEnd - lineStart ) * HF_UI_ADVANCE <= maxWidth ) {
					lineLen = wordEnd - lineStart;
				} else {
					if ( lines && count < maxLines ) {
						lines[ count ].start = lineStart;
						lines[ count ].length = lineLen;
						lines[ count ].width = lineLen * HF_UI_ADVANCE;
					}
					count++;
					lineStart = wordStart;
					lineLen = wordEnd - wordStart;
				}
			}
			if ( lines && count < maxLines ) {
				lines[ count ].start = lineStart;
				lines[ count ].length = lineLen;
				lines[ count ].width = lineLen * HF_UI_ADVANCE;
			}
			count++;

			if ( paraEnd >= len ) { break; }
			p = paraEnd + 1;
		}
		return count;
	}

	// Rasterize [text, text+len) with its top-left at (x, y) into out. Each
	// glyph is stamped as its 5x7 bitmap (1x scale); solid pixels take
	// 'color'. Newlines advance to the next line.
	HF_INLINE static void Rasterize( const char *text, int len, int x, int y,
									 hfUiCoverage &out, unsigned char color = 255 ) {
		if ( !text ) { return; }
		if ( len < 0 ) { len = ( int )strlen( text ); }
		int cx = x;
		int cy = y;
		for ( int i = 0; i < len; i++ ) {
			char c = text[ i ];
			if ( c == '\n' ) { cx = x; cy += HF_UI_LINE_HEIGHT; continue; }
			if ( c != ' ' ) {
				const unsigned char *g = Glyph( c );
				for ( int row = 0; row < HF_UI_GLYPH_H; row++ ) {
					unsigned char bits = g[ row ];
					for ( int col = 0; col < HF_UI_GLYPH_W; col++ ) {
						if ( bits & ( 1 << ( HF_UI_GLYPH_W - 1 - col ) ) ) {
							out.Set( cx + col, cy + row, color );
						}
					}
				}
			}
			cx += HF_UI_ADVANCE;
		}
	}
};

} // namespace HF_Ui

#endif