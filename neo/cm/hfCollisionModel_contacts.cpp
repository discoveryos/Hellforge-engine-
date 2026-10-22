/*
===============================================================================

	Trace model vs. polygonal model collision detection.

===============================================================================
*/

#pragma hdrstop
#include "../hflib/hfprecompiled.h"


#include "hfCollisionModel_local.h"

/*
===============================================================================

Retrieving contacts

===============================================================================
*/

/*
==================
hfCollisionModelManagerLocal::Contacts
==================
*/
int hfCollisionModelManagerLocal::Contacts( contactInfo_t *contacts, const int maxContacts, const hfVec3 &start, const hfVec6 &dir, const float depth,
								const hfTraceModel *trm, const hfMat3 &trmAxis, int contentMask,
								cmHandle_t model, const hfVec3 &origin, const hfMat3 &modelAxis ) {
	trace_t results;
	hfVec3 end;

	// same as Translation but instead of storing the first collision we store all collisions as contacts
	hfCollisionModelManagerLocal::getContacts = true;
	hfCollisionModelManagerLocal::contacts = contacts;
	hfCollisionModelManagerLocal::maxContacts = maxContacts;
	hfCollisionModelManagerLocal::numContacts = 0;
	end = start + dir.SubVec3(0) * depth;
	hfCollisionModelManagerLocal::Translation( &results, start, end, trm, trmAxis, contentMask, model, origin, modelAxis );
	if ( dir.SubVec3(1).LengthSqr() != 0.0f ) {
		// FIXME: rotational contacts
	}
	hfCollisionModelManagerLocal::getContacts = false;
	hfCollisionModelManagerLocal::maxContacts = 0;

	return hfCollisionModelManagerLocal::numContacts;
}

