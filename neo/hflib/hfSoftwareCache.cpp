#include "hfParallelJobList_JobHeaders.h"

/*
================================================================================================

	Software Cache

================================================================================================
*/

uint32 globalDmaTag;

bool SpursEmulationAssertFailed( const char *filename, int line, const char *expression ) {
	static bool halt = true;
	if ( halt ) {
		HF_DEBUG_BREAK();
	}
	return true;
}

