#ifndef __SYS_STATS_H__
#define __SYS_STATS_H__

//------------------------
// leaderboardError_t
//------------------------
enum leaderboardError_t {
	LEADERBOARD_ERROR_NONE,
	LEADERBOARD_ERROR_FAILED,				// General error occurred
	LEADERBOARD_ERROR_NOT_ONLINE,			// Not online to request leaderboards
	LEADERBOARD_ERROR_BUSY,					// Unable to download leaderboards right now (download already in progress)
	LEADERBOARD_ERROR_INVALID_USER,			// Unable to request leaderboards as the given user
	LEADERBOARD_ERROR_INVALID_REQUEST,		// The leaderboard request was invalid
	LEADERBOARD_ERROR_DOWNLOAD,				// An error occurred while downloading the leaderboard
	LEADERBOARD_ERROR_MAX
};

/*
================================================
hfLeaderboardCallback 
================================================
*/
class hfLeaderboardCallback : public hfCallback {
public:
	struct row_t {
		bool		hasAttachment;
		int64		attachmentID;
		hfStr		name;
		int64		rank;
		hfArray< int64, MAX_LEADERBOARD_COLUMNS > 	columns;
	};

	hfLeaderboardCallback() : def( NULL ), startIndex( -1 ), localIndex( -1 ), numRowsInLeaderboard( -1 ), errorCode( LEADERBOARD_ERROR_NONE ) { }
	virtual hfLeaderboardCallback * Clone() const = 0;

	// Used by the platform handlers to set data
	void 							ResetRows() { rows.Clear(); }
	void 							AddRow( const row_t & row ) { rows.Append( row ); }
	void 							SetNumRowsInLeaderboard( int32 i ) { numRowsInLeaderboard = i; }
	void 							SetDef( const leaderboardDefinition_t * def_ ) { def = def_; }
	void 							SetStartIndex( int startIndex_ ) { startIndex = startIndex_; }
	void 							SetLocalIndex( int localIndex_ ) { localIndex = localIndex_; }
	void							SetErrorCode( leaderboardError_t errorCode ) { this->errorCode = errorCode; }

	// Used in user callback for information retrieval
	const leaderboardDefinition_t *	GetDef() const { return def; }
	int								GetStartIndex() const { return startIndex; }
	const hfList< row_t > &			GetRows() const { return rows; } 
	int								GetNumRowsInLeaderboard() const { return numRowsInLeaderboard; }
	int								GetLocalIndex() const { return localIndex; }
	leaderboardError_t				GetErrorCode() const { return this->errorCode; }

protected:
	const leaderboardDefinition_t *	def;					// leaderboard def
	int								startIndex;				// where the first row starts in the online leaderboard
	int								localIndex;				// if player is in the rows, this is the offset of him within the returned rows
	hfList< row_t >					rows;					// leaderboard entries for the request
	int								numRowsInLeaderboard;	// total number of rows in the online leaderboard
	leaderboardError_t				errorCode;				// error, if any, that occurred during last operation
};



#endif // !__SYS_STATS_H__

