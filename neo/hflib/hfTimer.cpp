#include "hfprecompiled.h"
#pragma hdrstop

double hfTimer::base = -1.0;


/*
=================
hfTimer::InitBaseClockTicks
=================
*/
void hfTimer::InitBaseClockTicks() const {
	hfTimer timer;
	double ct, b;
	int i;

	base = 0.0;
	b = -1.0;
	for ( i = 0; i < 1000; i++ ) {
		timer.Clear();
		timer.Start();
		timer.Stop();
		ct = timer.ClockTicks();
		if ( b < 0.0 || ct < b ) {
			b = ct;
		}
	}
	base = b;
}


/*
=================
hfTimerReport::hfTimerReport
=================
*/
hfTimerReport::hfTimerReport() {
}

/*
=================
hfTimerReport::SetReportName
=================
*/
void hfTimerReport::SetReportName( const char *name ) {
	reportName = ( name ) ? name : "Timer Report";
}

/*
=================
hfTimerReport::~hfTimerReport
=================
*/
hfTimerReport::~hfTimerReport() {
	Clear();
}

/*
=================
hfTimerReport::AddReport
=================
*/
int hfTimerReport::AddReport( const char *name ) {
	if ( name && *name ) {
		names.Append( name );
		return timers.Append( new (TAG_IDLIB) hfTimer() );
	}
	return -1;
}

/*
=================
hfTimerReport::Clear
=================
*/
void hfTimerReport::Clear() {
	timers.DeleteContents( true );
	names.Clear();
	reportName.Clear();
}

/*
=================
hfTimerReport::Reset
=================
*/
void hfTimerReport::Reset() {
	assert ( timers.Num() == names.Num() );
	for ( int i = 0; i < timers.Num(); i++ ) {
		timers[i]->Clear();
	}
}

/*
=================
hfTimerReport::AddTime
=================
*/
void hfTimerReport::AddTime( const char *name, hfTimer *time ) {
	assert ( timers.Num() == names.Num() );
	int i;
	for ( i = 0; i < names.Num(); i++ ) {
		if ( names[i].Icmp( name ) == 0 ) {
			*timers[i] += *time;
			break;
		}
	}
	if ( i == names.Num() ) {
		int index = AddReport( name );
		if ( index >= 0 ) {
			timers[index]->Clear();
			*timers[index] += *time;
		}
	}
}

/*
=================
hfTimerReport::PrintReport
=================
*/
void hfTimerReport::PrintReport() {
	assert( timers.Num() == names.Num() );
	hfLib::common->Printf( "Timing Report for %s\n", reportName.c_str() );
	hfLib::common->Printf( "-------------------------------\n" );
	float total = 0.0f;
	for ( int i = 0; i < names.Num(); i++ ) {
		hfLib::common->Printf( "%s consumed %5.2f seconds\n", names[i].c_str(), timers[i]->Milliseconds() * 0.001f );
		total += timers[i]->Milliseconds();
	}
	hfLib::common->Printf( "Total time for report %s was %5.2f\n\n", reportName.c_str(), total * 0.001f );
}

