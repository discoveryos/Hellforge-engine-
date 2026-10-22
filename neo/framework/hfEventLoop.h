#ifndef __EVENTLOOP_H__
#define __EVENTLOOP_H__

/*
===============================================================================

	The event loop receives events from the system and dispatches them to
	the various parts of the engine. The event loop also handles journaling.
	The file system copies .cfg files to the journaled file.

===============================================================================
*/

const int MAX_PUSHED_EVENTS =	64;

class hfEventLoop {
public:
					hfEventLoop();
					~hfEventLoop();

	void			Init();

					// Closes the journal file if needed.
	void			Shutdown();

					// It is possible to get an event at the beginning of a frame that
					// has a time stamp lower than the last event from the previous frame.
	sysEvent_t		GetEvent();

					// Dispatches all pending events and returns the current time.
	int				RunEventLoop( bool commandExecution = true );

					// Gets the current time in a way that will be journaled properly,
					// as opposed to Sys_Milliseconds(), which always reads a real timer.
	int				Milliseconds();

					// Returns the journal level, 1 = record, 2 = play back.
	int				JournalLevel() const;

					// Journal file.
	hfFile *		com_journalFile;
	hfFile *		com_journalDataFile;

private:
					// all events will have this subtracted from their time
	int				initialTimeOffset;

	int				com_pushedEventsHead, com_pushedEventsTail;
	sysEvent_t		com_pushedEvents[MAX_PUSHED_EVENTS];

	static hfCVar	com_journal;

	sysEvent_t		GetRealEvent();
	void			ProcessEvent( sysEvent_t ev );
	void			PushEvent( sysEvent_t *event );
};

extern	hfEventLoop	*eventLoop;

#endif /* !__EVENTLOOP_H__ */

