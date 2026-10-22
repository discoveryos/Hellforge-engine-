#ifndef __MATH_ODE_H__
#define __MATH_ODE_H__

/*
===============================================================================

	Numerical solvers for ordinary differential equations.

===============================================================================
*/


//===============================================================
//
//	hfODE
//
//===============================================================

typedef void (*deriveFunction_t)( const float t, const void *userData, const float *state, float *derivatives );

class hfODE {

public:
	virtual				~hfODE() {}

	virtual float		Evaluate( const float *state, float *newState, float t0, float t1 ) = 0;

protected:
	int					dimension;		// dimension in floats allocated for
	deriveFunction_t	derive;			// derive function
	const void *		userData;		// client data
};

//===============================================================
//
//	hfODE_Euler
//
//===============================================================

class hfODE_Euler : public hfODE {

public:
						hfODE_Euler( const int dim, const deriveFunction_t dr, const void *ud );
	virtual				~hfODE_Euler();

	virtual float		Evaluate( const float *state, float *newState, float t0, float t1 );

protected:
	float *				derivatives;	// space to store derivatives
};

//===============================================================
//
//	hfODE_Midpoint
//
//===============================================================

class hfODE_Midpoint : public hfODE {

public:
						hfODE_Midpoint( const int dim, const deriveFunction_t dr, const void *ud );
	virtual				~hfODE_Midpoint();

	virtual float		Evaluate( const float *state, float *newState, float t0, float t1 );

protected:
	float *				tmpState;
	float *				derivatives;	// space to store derivatives
};

//===============================================================
//
//	hfODE_RK4
//
//===============================================================

class hfODE_RK4 : public hfODE {

public:
						hfODE_RK4( const int dim, const deriveFunction_t dr, const void *ud );
	virtual				~hfODE_RK4();

	virtual float		Evaluate( const float *state, float *newState, float t0, float t1 );

protected:
	float *				tmpState;
	float *				d1;				// derivatives
	float *				d2;
	float *				d3;
	float *				d4;
};

//===============================================================
//
//	hfODE_RK4Adaptive
//
//===============================================================

class hfODE_RK4Adaptive : public hfODE {

public:
						hfODE_RK4Adaptive( const int dim, const deriveFunction_t dr, const void *ud );
	virtual				~hfODE_RK4Adaptive();

	virtual float		Evaluate( const float *state, float *newState, float t0, float t1 );
	void				SetMaxError( const float err );

protected:
	float				maxError;		// maximum allowed error
	float *				tmpState;
	float *				d1;				// derivatives
	float *				d1half;
	float *				d2;
	float *				d3;
	float *				d4;
};

#endif /* !__MATH_ODE_H__ */

