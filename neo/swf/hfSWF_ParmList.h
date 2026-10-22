#ifndef	__SWF_PARMLIST_H__
#define	__SWF_PARMLIST_H__

// static list for script parameters
static const int SWF_MAX_PARMS = 16;

/*
================================================
hfSWFParmList 

A static list for script parameters that reduces the number of SWF allocations dramatically.
================================================
*/
class hfSWFParmList : public hfStaticList< hfSWFScriptVar, SWF_MAX_PARMS > {
public:
					hfSWFParmList() {
					}
	explicit		hfSWFParmList( const int num_ ) {
						SetNum( num_ );
					}

	void	Append( const hfSWFScriptVar & other );
	void	Append( hfSWFScriptObject * o );
	void	Append( hfSWFScriptFunction * f );
	void	Append( const char * s );
	void	Append( const hfStr & s );
	void	Append( hfSWFScriptString * s );
	void	Append( const float f );
	void	Append( const int32 i );
	void	Append( const bool b );
};

#endif	// __SWF_PARMLIST_H__ 

