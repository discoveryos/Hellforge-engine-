#ifndef __BOUNDSTRACK_H__
#define __BOUNDSTRACK_H__


struct shortBounds_t;

class hfBoundsTrack {
public:
	hfBoundsTrack();
	~hfBoundsTrack();

	void ClearAll();

	// more than this will thrash a 32k L1 data cache
	static const int MAX_BOUNDS_TRACK_INDEXES = 2048;

	// the bounds will be clamped and rounded to short integers
	void	SetIndex( const int index, const hfBounds & bounds );

	// an index that has been cleared will never be returned by FindIntersections()
	void	ClearIndex( const int index );

	// returns the number of indexes filled in intersectedIndexes[]
	//
	// The intersections may include some bounds that are not truly overlapping
	// due to the rounding from float to short integers.
	int		FindIntersections( const hfBounds & testBounds, int intersectedIndexes[ MAX_BOUNDS_TRACK_INDEXES ] ) const;

	// validate implementation
	void	Test();

private:
	// All elements that haven't had SetIndex() called since ClearAll() will be
	// in the cleared state, so they can safely be compared against by an
	// unwound loop.
	shortBounds_t *boundsList;	// [MAX_BOUNDS_TRACK_INDEXES]

	// set to 0 at ClearAll(), maintained greater than the highest index passed
	// to SetIndex() since ClearAll().
	int				maxIndex;
};


#endif // __BOUNDSTRACK_H__

