#ifndef __MATH_LCP_H__
#define __MATH_LCP_H__

/*
================================================
The *LCP* class, hfLCP, is a Box-Constrained Mixed Linear Complementarity Problem solver.

'A' is a matrix of dimension n*n and 'x', 'b', 'lo', 'hi' are vectors of dimension n.

Solve: Ax = b + t, where t is a vector of dimension n, with complementarity condition:

	(x[i] - lo[i]) * (x[i] - hi[i]) * t[i] = 0

such that for each 0 <= i < n one of the following holds:

	lo[i] < x[i] < hi[i], t[i] == 0
	x[i] == lo[i], t[i] >= 0
	x[i] == hi[i], t[i] <= 0

Partly-bounded or unbounded variables can have lo[i] and/or hi[i] set to negative/positive 
hfMath::INFITITY, respectively.

If boxIndex != NULL and boxIndex[i] != -1, then

	lo[i] = - fabs( lo[i] * x[boxIndex[i]] )
	hi[i] = fabs( hi[i] * x[boxIndex[i]] )
	boxIndex[boxIndex[i]] must be -1

Before calculating any of the bounded x[i] with boxIndex[i] != -1, the solver calculates all 
unbounded x[i] and all x[i] with boxIndex[i] == -1.
================================================
*/
class hfLCP {
public:
	static hfLCP *	AllocSquare();		// 'A' must be a square matrix
	static hfLCP *	AllocSymmetric();	// 'A' must be a symmetric matrix

	virtual			~hfLCP();

	virtual bool	Solve( const hfMatX &A, hfVecX &x, const hfVecX &b, const hfVecX &lo, 
						   const hfVecX &hi, const int *boxIndex = NULL ) = 0;

	virtual void	SetMaxIterations( int max );
	virtual int		GetMaxIterations();

	static void		Test_f( const hfCmdArgs &args );

protected:
	int				maxIterations;
};

#endif // !__MATH_LCP_H__

