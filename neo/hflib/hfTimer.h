#ifndef __TIMER_H__
#define __TIMER_H__

/*
===============================================================================

	Clock tick counter. Should only be used for profiling.

===============================================================================
*/

class hfTimer {
public:
					hfTimer();
					hfTimer( double clockTicks );
					~hfTimer();

	hfTimer			operator+( const hfTimer &t ) const;
	hfTimer			operator-( const hfTimer &t ) const;
	hfTimer &		operator+=( const hfTimer &t );
	hfTimer &		operator-=( const hfTimer &t );

	void			Start();
	void			Stop();
	void			Clear();
	double			ClockTicks() const;
	double			Milliseconds() const;

private:
	static double	base;
	enum			{
						TS_STARTED,
						TS_STOPPED
					} state;
	double			start;
	double			clockTicks;

	void			InitBaseClockTicks() const;
};

/*
=================
hfTimer::hfTimer
=================
*/
HF_INLINE hfTimer::hfTimer() {
	state = TS_STOPPED;
	clockTicks = 0.0;
}

/*
=================
hfTimer::hfTimer
=================
*/
HF_INLINE hfTimer::hfTimer( double _clockTicks ) {
	state = TS_STOPPED;
	clockTicks = _clockTicks;
}

/*
=================
hfTimer::~hfTimer
=================
*/
HF_INLINE hfTimer::~hfTimer() {
}

/*
=================
hfTimer::operator+
=================
*/
HF_INLINE hfTimer hfTimer::operator+( const hfTimer &t ) const {
	assert( state == TS_STOPPED && t.state == TS_STOPPED );
	return hfTimer( clockTicks + t.clockTicks );
}

/*
=================
hfTimer::operator-
=================
*/
HF_INLINE hfTimer hfTimer::operator-( const hfTimer &t ) const {
	assert( state == TS_STOPPED && t.state == TS_STOPPED );
	return hfTimer( clockTicks - t.clockTicks );
}

/*
=================
hfTimer::operator+=
=================
*/
HF_INLINE hfTimer &hfTimer::operator+=( const hfTimer &t ) {
	assert( state == TS_STOPPED && t.state == TS_STOPPED );
	clockTicks += t.clockTicks;
	return *this;
}

/*
=================
hfTimer::operator-=
=================
*/
HF_INLINE hfTimer &hfTimer::operator-=( const hfTimer &t ) {
	assert( state == TS_STOPPED && t.state == TS_STOPPED );
	clockTicks -= t.clockTicks;
	return *this;
}

/*
=================
hfTimer::Start
=================
*/
HF_INLINE void hfTimer::Start() {
	assert( state == TS_STOPPED );
	state = TS_STARTED;
	start = hfLib::sys->GetClockTicks();
}

/*
=================
hfTimer::Stop
=================
*/
HF_INLINE void hfTimer::Stop() {
	assert( state == TS_STARTED );
	clockTicks += hfLib::sys->GetClockTicks() - start;
	if ( base < 0.0 ) {
		InitBaseClockTicks();
	}
	if ( clockTicks > base ) {
		clockTicks -= base;
	}
	state = TS_STOPPED;
}

/*
=================
hfTimer::Clear
=================
*/
HF_INLINE void hfTimer::Clear() {
	clockTicks = 0.0;
}

/*
=================
hfTimer::ClockTicks
=================
*/
HF_INLINE double hfTimer::ClockTicks() const {
	assert( state == TS_STOPPED );
	return clockTicks;
}

/*
=================
hfTimer::Milliseconds
=================
*/
HF_INLINE double hfTimer::Milliseconds() const {
	assert( state == TS_STOPPED );
	return clockTicks / ( hfLib::sys->ClockTicksPerSecond() * 0.001 );
}


/*
===============================================================================

	Report of multiple named timers.

===============================================================================
*/

class hfTimerReport {
public:
					hfTimerReport();
					~hfTimerReport();

	void			SetReportName( const char *name );
	int				AddReport( const char *name );
	void			Clear();
	void			Reset();
	void			PrintReport();
	void			AddTime( const char *name, hfTimer *time );

private:
	hfList<hfTimer*>timers;
	hfStrList		names;
	hfStr			reportName;
};

#endif /* !__TIMER_H__ */

