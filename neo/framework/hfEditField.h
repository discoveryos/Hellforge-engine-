#ifndef __EDITFIELD_H__
#define __EDITFIELD_H__

/*
===============================================================================

	Edit field

===============================================================================
*/

const int MAX_EDIT_LINE = 256;

typedef struct autoComplete_s {
	bool			valid;
	int				length;
	char			completionString[MAX_EDIT_LINE];
	char			currentMatch[MAX_EDIT_LINE];
	int				matchCount;
	int				matchIndex;
	int				findMatchIndex;
} autoComplete_t;

class hfEditField {
public:
					hfEditField();
					~hfEditField();

	void			Clear();
	void			SetWidthInChars( int w );
	void			SetCursor( int c );
	int				GetCursor() const;
	void			ClearAutoComplete();
	int				GetAutoCompleteLength() const;
	void			AutoComplete();
	void			CharEvent( int c );
	void			KeyDownEvent( int key );
	void			Paste();
	char *			GetBuffer();
	void			Draw( int x, int y, int width, bool showCursor );
	void			SetBuffer( const char *buffer );

private:
	int				cursor;
	int				scroll;
	int				widthInChars;
	char			buffer[MAX_EDIT_LINE];
	autoComplete_t	autoComplete;
};

#endif /* !__EDITFIELD_H__ */

