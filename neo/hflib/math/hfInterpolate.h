#ifndef __MATH_INTERPOLATE_H__
#define __MATH_INTERPOLATE_H__

/*
==============================================================================================

	Linear interpolation.

==============================================================================================
*/

template< class type >
class hfInterpolate {
public:
						hfInterpolate();

	void				Init( const int startTime, const int duration, const type &startValue, const type &endValue );
	void				SetStartTime( int time ) { this->startTime = time; }
	void				SetDuration( int duration ) { this->duration = duration; }
	void				SetStartValue( const type &startValue ) { this->startValue = startValue; }
	void				SetEndValue( const type &endValue ) { this->endValue = endValue; }

	type				GetCurrentValue( int time ) const;
	bool				IsDone( int time ) const { return ( time >= startTime + duration ); }

	int					GetStartTime() const { return startTime; }
	int					GetEndTime() const { return startTime + duration; }
	int					GetDuration() const { return duration; }
	const type &		GetStartValue() const { return startValue; }
	const type &		GetEndValue() const { return endValue; }

private:
	int					startTime;
	int					duration;
	type				startValue;
	type				endValue;
};

/*
====================
hfInterpolate::hfInterpolate
====================
*/
template< class type >
HF_INLINE hfInterpolate<type>::hfInterpolate() {
	startTime = duration = 0;
	memset( &startValue, 0, sizeof( startValue ) );
	memset( &endValue, 0, sizeof( endValue ) );
}

/*
====================
hfInterpolate::Init
====================
*/
template< class type >
HF_INLINE void hfInterpolate<type>::Init( const int startTime, const int duration, const type &startValue, const type &endValue ) {
	this->startTime = startTime;
	this->duration = duration;
	this->startValue = startValue;
	this->endValue = endValue;
}

/*
====================
hfInterpolate::GetCurrentValue
====================
*/
template< class type >
HF_INLINE type hfInterpolate<type>::GetCurrentValue( int time ) const {
	if ( time <= startTime ) {
		return startValue;
	} else if ( time >= startTime + duration ) {
		return endValue;
	} else {
		const float deltaTime = time - startTime;
		const float f = deltaTime / (float)duration;
		const type range = ( endValue - startValue );
		return startValue + ( range * f );
	}
}

/*
==============================================================================================

	Continuous interpolation with linear acceleration and deceleration phase.
	The velocity is continuous but the acceleration is not.

==============================================================================================
*/

template< class type >
class hfInterpolateAccelDecelLinear  {
public:
						hfInterpolateAccelDecelLinear();

	void				Init( const int startTime, const int accelTime, const int decelTime, const int duration, const type &startValue, const type &endValue );
	void				SetStartTime( int time ) { startTime = time; Invalidate(); }
	void				SetStartValue( const type &startValue ) { this->startValue = startValue; Invalidate(); }
	void				SetEndValue( const type &endValue ) { this->endValue = endValue; Invalidate(); }

	type				GetCurrentValue( int time ) const;
	type				GetCurrentSpeed( int time ) const;
	bool				IsDone( int time ) const { return ( time >= startTime + accelTime + linearTime + decelTime ); }

	int					GetStartTime() const { return startTime; }
	int					GetEndTime() const { return startTime + accelTime + linearTime + decelTime; }
	int					GetDuration() const { return accelTime + linearTime + decelTime; }
	int					GetAcceleration() const { return accelTime; }
	int					GetDeceleration() const { return decelTime; }
	const type &		GetStartValue() const { return startValue; }
	const type &		GetEndValue() const { return endValue; }

private:
	int					startTime;
	int					accelTime;
	int					linearTime;
	int					decelTime;
	type				startValue;
	type				endValue;
	mutable hfExtrapolate<type> extrapolate;

	void				Invalidate();
	void				SetPhase( int time ) const;
};

/*
====================
hfInterpolateAccelDecelLinear::hfInterpolateAccelDecelLinear
====================
*/
template< class type >
HF_INLINE hfInterpolateAccelDecelLinear<type>::hfInterpolateAccelDecelLinear() {
	startTime = accelTime = linearTime = decelTime = 0;
	memset( &startValue, 0, sizeof( startValue ) );
	endValue = startValue;
}

/*
====================
hfInterpolateAccelDecelLinear::Init
====================
*/
template< class type >
HF_INLINE void hfInterpolateAccelDecelLinear<type>::Init( const int startTime, const int accelTime, const int decelTime, const int duration, const type &startValue, const type &endValue ) {
	this->startTime = startTime;
	this->accelTime = accelTime;
	this->decelTime = decelTime;
	this->startValue = startValue;
	this->endValue = endValue;

	if ( duration <= 0 ) {
		return;
	}

	if ( this->accelTime + this->decelTime > duration ) {
		this->accelTime = this->accelTime * duration / ( this->accelTime + this->decelTime );
		this->decelTime = duration - this->accelTime;
	}
	this->linearTime = duration - this->accelTime - this->decelTime;
	const type speed = ( endValue - startValue ) * ( 1000.0f / ( (float) this->linearTime + ( this->accelTime + this->decelTime ) * 0.5f ) );

	if ( this->accelTime ) {
		extrapolate.Init( startTime, this->accelTime, startValue, ( startValue - startValue ), speed, EXTRAPOLATION_ACCELLINEAR ); //-V501
	} else if ( this->linearTime ) {
		extrapolate.Init( startTime, this->linearTime, startValue, ( startValue - startValue ), speed, EXTRAPOLATION_LINEAR ); //-V501
	} else {
		extrapolate.Init( startTime, this->decelTime, startValue, ( startValue - startValue ), speed, EXTRAPOLATION_DECELLINEAR ); //-V501
	}
}

/*
====================
hfInterpolateAccelDecelLinear::Invalidate
====================
*/
template< class type >
HF_INLINE void hfInterpolateAccelDecelLinear<type>::Invalidate() {
	extrapolate.Init( 0, 0, extrapolate.GetStartValue(), extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_NONE );
}

/*
====================
hfInterpolateAccelDecelLinear::SetPhase
====================
*/
template< class type >
HF_INLINE void hfInterpolateAccelDecelLinear<type>::SetPhase( int time ) const {
	const float deltaTime = time - startTime;
	if ( deltaTime < accelTime ) {
		if ( extrapolate.GetExtrapolationType() != EXTRAPOLATION_ACCELLINEAR ) {
			extrapolate.Init( startTime, accelTime, startValue, extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_ACCELLINEAR );
		}
	} else if ( deltaTime < accelTime + linearTime ) {
		if ( extrapolate.GetExtrapolationType() != EXTRAPOLATION_LINEAR ) {
			extrapolate.Init( startTime + accelTime, linearTime, startValue + extrapolate.GetSpeed() * ( accelTime * 0.001f * 0.5f ), extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_LINEAR );
		}
	} else {
		if ( extrapolate.GetExtrapolationType() != EXTRAPOLATION_DECELLINEAR ) {
			extrapolate.Init( startTime + accelTime + linearTime, decelTime, endValue - ( extrapolate.GetSpeed() * ( decelTime * 0.001f * 0.5f ) ), extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_DECELLINEAR );
		}
	}
}

/*
====================
hfInterpolateAccelDecelLinear::GetCurrentValue
====================
*/
template< class type >
HF_INLINE type hfInterpolateAccelDecelLinear<type>::GetCurrentValue( int time ) const {
	SetPhase( time );
	return extrapolate.GetCurrentValue( time );
}

/*
====================
hfInterpolateAccelDecelLinear::GetCurrentSpeed
====================
*/
template< class type >
HF_INLINE type hfInterpolateAccelDecelLinear<type>::GetCurrentSpeed( int time ) const {
	SetPhase( time );
	return extrapolate.GetCurrentSpeed( time );
}


/*
==============================================================================================

	Continuous interpolation with sinusoidal acceleration and deceleration phase.
	Both the velocity and acceleration are continuous.

==============================================================================================
*/

template< class type >
class hfInterpolateAccelDecelSine  {
public:
						hfInterpolateAccelDecelSine();

	void				Init( const int startTime, const int accelTime, const int decelTime, const int duration, const type &startValue, const type &endValue );
	void				SetStartTime( int time ) { startTime = time; Invalidate(); }
	void				SetStartValue( const type &startValue ) { this->startValue = startValue; Invalidate(); }
	void				SetEndValue( const type &endValue ) { this->endValue = endValue; Invalidate(); }

	type				GetCurrentValue( int time ) const;
	type				GetCurrentSpeed( int time ) const;
	bool				IsDone( int time ) const { return ( time >= startTime + accelTime + linearTime + decelTime ); }

	int					GetStartTime() const { return startTime; }
	int					GetEndTime() const { return startTime + accelTime + linearTime + decelTime; }
	int					GetDuration() const { return accelTime + linearTime + decelTime; }
	int					GetAcceleration() const { return accelTime; }
	int					GetDeceleration() const { return decelTime; }
	const type &		GetStartValue() const { return startValue; }
	const type &		GetEndValue() const { return endValue; }

private:
	int					startTime;
	int					accelTime;
	int					linearTime;
	int					decelTime;
	type				startValue;
	type				endValue;
	mutable hfExtrapolate<type> extrapolate;

	void				Invalidate();
	void				SetPhase( int time ) const;
};

/*
====================
hfInterpolateAccelDecelSine::hfInterpolateAccelDecelSine
====================
*/
template< class type >
HF_INLINE hfInterpolateAccelDecelSine<type>::hfInterpolateAccelDecelSine() {
	startTime = accelTime = linearTime = decelTime = 0;
	memset( &startValue, 0, sizeof( startValue ) );
	memset( &endValue, 0, sizeof( endValue ) );
}

/*
====================
hfInterpolateAccelDecelSine::Init
====================
*/
template< class type >
HF_INLINE void hfInterpolateAccelDecelSine<type>::Init( const int startTime, const int accelTime, const int decelTime, const int duration, const type &startValue, const type &endValue ) {
	this->startTime = startTime;
	this->accelTime = accelTime;
	this->decelTime = decelTime;
	this->startValue = startValue;
	this->endValue = endValue;

	if ( duration <= 0 ) {
		return;
	}

	if ( this->accelTime + this->decelTime > duration ) {
		this->accelTime = this->accelTime * duration / ( this->accelTime + this->decelTime );
		this->decelTime = duration - this->accelTime;
	}
	this->linearTime = duration - this->accelTime - this->decelTime;
	const type speed = ( endValue - startValue ) * ( 1000.0f / ( (float) this->linearTime + ( this->accelTime + this->decelTime ) * hfMath::SQRT_1OVER2 ) );

	if ( this->accelTime ) {
		extrapolate.Init( startTime, this->accelTime, startValue, ( startValue - startValue ), speed, EXTRAPOLATION_ACCELSINE ); //-V501
	} else if ( this->linearTime ) {
		extrapolate.Init( startTime, this->linearTime, startValue, ( startValue - startValue ), speed, EXTRAPOLATION_LINEAR ); //-V501
	} else {
		extrapolate.Init( startTime, this->decelTime, startValue, ( startValue - startValue ), speed, EXTRAPOLATION_DECELSINE ); //-V501
	}
}

/*
====================
hfInterpolateAccelDecelSine::Invalidate
====================
*/
template< class type >
HF_INLINE void hfInterpolateAccelDecelSine<type>::Invalidate() {
	extrapolate.Init( 0, 0, extrapolate.GetStartValue(), extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_NONE );
}

/*
====================
hfInterpolateAccelDecelSine::SetPhase
====================
*/
template< class type >
HF_INLINE void hfInterpolateAccelDecelSine<type>::SetPhase( int time ) const {
	const float deltaTime = time - startTime;
	if ( deltaTime < accelTime ) {
		if ( extrapolate.GetExtrapolationType() != EXTRAPOLATION_ACCELSINE ) {
			extrapolate.Init( startTime, accelTime, startValue, extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_ACCELSINE );
		}
	} else if ( deltaTime < accelTime + linearTime ) {
		if ( extrapolate.GetExtrapolationType() != EXTRAPOLATION_LINEAR ) {
			extrapolate.Init( startTime + accelTime, linearTime, startValue + extrapolate.GetSpeed() * ( accelTime * 0.001f * hfMath::SQRT_1OVER2 ), extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_LINEAR );
		}
	} else {
		if ( extrapolate.GetExtrapolationType() != EXTRAPOLATION_DECELSINE ) {
			extrapolate.Init( startTime + accelTime + linearTime, decelTime, endValue - ( extrapolate.GetSpeed() * ( decelTime * 0.001f * hfMath::SQRT_1OVER2 ) ), extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_DECELSINE );
		}
	}
}

/*
====================
hfInterpolateAccelDecelSine::GetCurrentValue
====================
*/
template< class type >
HF_INLINE type hfInterpolateAccelDecelSine<type>::GetCurrentValue( int time ) const {
	SetPhase( time );
	return extrapolate.GetCurrentValue( time );
}

/*
====================
hfInterpolateAccelDecelSine::GetCurrentSpeed
====================
*/
template< class type >
HF_INLINE type hfInterpolateAccelDecelSine<type>::GetCurrentSpeed( int time ) const {
	SetPhase( time );
	return extrapolate.GetCurrentSpeed( time );
}

#endif /* !__MATH_INTERPOLATE_H__ */

