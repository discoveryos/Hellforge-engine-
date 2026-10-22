#ifndef __BINSEARCH_H__
#define __BINSEARCH_H__

/*
===============================================================================

	Binary Search templates

	The array elements have to be ordered in increasing order.

===============================================================================
*/

/*
====================
hfBinSearch_GreaterEqual

	Finds the last array element which is smaller than the given value.
====================
*/
template< class type >
HF_INLINE int hfBinSearch_Less( const type *array, const int arraySize, const type &value ) {
	int len = arraySize;
	int mid = len;
	int offset = 0;
	while( mid > 0 ) {
		mid = len >> 1;
		if ( array[offset+mid] < value ) {
			offset += mid;
		}
		len -= mid;
	}
	return offset;
}

/*
====================
hfBinSearch_GreaterEqual

	Finds the last array element which is smaller than or equal to the given value.
====================
*/
template< class type >
HF_INLINE int hfBinSearch_LessEqual( const type *array, const int arraySize, const type &value ) {
	int len = arraySize;
	int mid = len;
	int offset = 0;
	while( mid > 0 ) {
		mid = len >> 1;
		if ( array[offset+mid] <= value ) {
			offset += mid;
		}
		len -= mid;
	}
	return offset;
}

/*
====================
hfBinSearch_Greater

	Finds the first array element which is greater than the given value.
====================
*/
template< class type >
HF_INLINE int hfBinSearch_Greater( const type *array, const int arraySize, const type &value ) {
	int len = arraySize;
	int mid = len;
	int offset = 0;
	int res = 0;
	while( mid > 0 ) {
		mid = len >> 1;
		if ( array[offset+mid] > value ) {
			res = 0;
		} else {
			offset += mid;
			res = 1;
		}
		len -= mid;
	}
	return offset+res;
}

/*
====================
hfBinSearch_GreaterEqual

	Finds the first array element which is greater than or equal to the given value.
====================
*/
template< class type >
HF_INLINE int hfBinSearch_GreaterEqual( const type *array, const int arraySize, const type &value ) {
	int len = arraySize;
	int mid = len;
	int offset = 0;
	int res = 0;
	while( mid > 0 ) {
		mid = len >> 1;
		if ( array[offset+mid] >= value ) {
			res = 0;
		} else {
			offset += mid;
			res = 1;
		}
		len -= mid;
	}
	return offset+res;
}

#endif /* !__BINSEARCH_H__ */

