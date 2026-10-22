#ifndef __SCREENRECT_H__
#define __SCREENRECT_H__

/*
================================================================================================

hfScreenRect

hfScreenRect gets carried around with each drawSurf, so it makes sense
to keep it compact, instead of just using the hfBounds class
================================================================================================
*/

class hfScreenRect {
public:
	// Inclusive pixel bounds inside viewport
	short		x1;
	short		y1;
	short		x2;
	short		y2;

	// for depth bounds test
	float       zmin; 
	float		zmax;

	// clear to backwards values
	void		Clear();
	bool		IsEmpty() const;
	short		GetWidth() const { return x2 - x1 + 1; }
	short		GetHeight() const { return y2 - y1 + 1; }
	int			GetArea() const { return ( x2 - x1 + 1 ) * ( y2 - y1 + 1 ); }

	// expand by one pixel each way to fix roundoffs
	void		Expand();

	// adds a point
	void		AddPoint( float x, float y );

	void		Intersect( const hfScreenRect &rect );
	void		Union( const hfScreenRect &rect );
	bool		Equals( const hfScreenRect &rect ) const;
};

void R_ShowColoredScreenRect( const hfScreenRect &rect, int colorIndex );

#endif /* !__SCREENRECT_H__ */

