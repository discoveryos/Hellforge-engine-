#ifndef __CONSOLEHISTORY_H__
#define __CONSOLEHISTORY_H__

/*

This should behave like the windows command prompt, with the addition
of a persistant history file.

Note that commands bound to keys do not go through the console history.

*/

class hfConsoleHistory {
public:
			hfConsoleHistory() :
					upPoint( 0 ),
					downPoint( 0 ),
					returnLine( 0 ),
					numHistory( 0 )
				{ ClearHistory(); }

	void	LoadHistoryFile();

	// the line should not have a \n
	// Empty lines are never added to the history.
	// If the command is the same as the last returned history line, nothing is changed.
	void	AddToHistory( const char *line, bool writeHistoryFile = true );

	// the string will not have a \n
	// Returns an empty string if there is nothing to retrieve in that
	// direction.
	hfStr	RetrieveFromHistory( bool backward );

	// console commands
	void	PrintHistory();
	void	ClearHistory();

private:
	int		upPoint;	// command to be retrieved with next up-arrow
	int		downPoint;	// command to be retrieved with next down-arrow
	int		returnLine;	// last returned by RetrieveFromHistory()
	int		numHistory;

	static const int COMMAND_HISTORY = 64;
	hfArray<hfStr,COMMAND_HISTORY>	historyLines;

	compile_time_assert( CONST_ISPOWEROFTWO( COMMAND_HISTORY ) );	// we use the binary 'and' operator for wrapping
};

extern hfConsoleHistory consoleHistory;

#endif // !__CONSOLEHISTORY_H__

