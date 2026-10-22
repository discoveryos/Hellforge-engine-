#ifndef __STRLIST_H__
#define __STRLIST_H__

/*
===============================================================================

	hfStrList

===============================================================================
*/

typedef hfList<hfStr> hfStrList;
typedef hfList<hfStr*> hfStrPtrList;
typedef hfStr *hfStrPtr;

///*
//================
//hfListSortCompare<hfStrPtr>
//
//Compares two pointers to strings. Used to sort a list of string pointers alphabetically in hfList<hfStr>::Sort.
//================
//*/
//template<>
//HF_INLINE int hfListSortCompare<hfStrPtr, memTag_t _tag_ >( const hfStrPtr *a, const hfStrPtr *b ) {
//	return ( *a )->Icmp( **b );
//}

///*
//================
//hfStrList::Sort
//
//Sorts the list of strings alphabetically. Creates a list of pointers to the actual strings and sorts the
//pointer list. Then copies the strings into another list using the ordered list of pointers.
//================
//*/
//template<>
//HF_INLINE void hfStrList::Sort( cmp_t *compare ) {
//	int i;
//
//	if ( !num ) {
//		return;
//	}
//
//	hfList<hfStr>		other;
//	hfList<hfStrPtr>	pointerList;
//
//	pointerList.SetNum( num );
//	for( i = 0; i < num; i++ ) {
//		pointerList[ i ] = &( *this )[ i ];
//	}
//
//	pointerList.Sort();
//
//	other.SetNum( num );
//	other.SetGranularity( granularity );
//	for( i = 0; i < other.Num(); i++ ) {
//		other[ i ] = *pointerList[ i ];
//	}
//
//	this->Swap( other );
//}

///*
//================
//hfStrList::SortSubSection
//
//Sorts a subsection of the list of strings alphabetically.
//================
//*/
//template<>
//HF_INLINE void hfStrList::SortSubSection( int startIndex, int endIndex, cmp_t *compare ) {
//	int i, s;
//
//	if ( !num ) {
//		return;
//	}
//	if ( startIndex < 0 ) {
//		startIndex = 0;
//	}
//	if ( endIndex >= num ) {
//		endIndex = num - 1;
//	}
//	if ( startIndex >= endIndex ) {
//		return;
//	}
//
//	hfList<hfStr>		other;
//	hfList<hfStrPtr>	pointerList;
//
//	s = endIndex - startIndex + 1;
//	other.SetNum( s );
//	pointerList.SetNum( s );
//	for( i = 0; i < s; i++ ) {
//		other[ i ] = ( *this )[ startIndex + i ];
//		pointerList[ i ] = &other[ i ];
//	}
//
//	pointerList.Sort();
//
//	for( i = 0; i < s; i++ ) {
//		(*this)[ startIndex + i ] = *pointerList[ i ];
//	}
//}

/*
================
hfStrList::Size
================
*/
template<>
HF_INLINE size_t hfStrList::Size() const {
	size_t s;
	int i;

	s = sizeof( *this );
	for( i = 0; i < Num(); i++ ) {
		s += ( *this )[ i ].Size();
	}

	return s;
}

/*
===============================================================================

	hfStrList path sorting

===============================================================================
*/
//
///*
//================
//hfListSortComparePaths
//
//Compares two pointers to strings. Used to sort a list of string pointers alphabetically in hfList<hfStr>::Sort.
//================
//*/
//template<class hfStrPtr>
//HF_INLINE int hfListSortComparePaths( const hfStrPtr *a, const hfStrPtr *b ) {
//	return ( *a )->IcmpPath( **b );
//}

///*
//================
//hfStrListSortPaths
//
//Sorts the list of path strings alphabetically and makes sure folders come first.
//================
//*/
//HF_INLINE void hfStrListSortPaths( hfStrList &list ) {
//	int i;
//
//	if ( !list.Num() ) {
//		return;
//	}
//
//	hfList<hfStr>		other;
//	hfList<hfStrPtr>	pointerList;
//
//	pointerList.SetNum( list.Num() );
//	for( i = 0; i < list.Num(); i++ ) {
//		pointerList[ i ] = &list[ i ];
//	}
//
//	pointerList.Sort( hfListSortComparePaths<hfStrPtr> );
//
//	other.SetNum( list.Num() );
//	other.SetGranularity( list.GetGranularity() );
//	for( i = 0; i < other.Num(); i++ ) {
//		other[ i ] = *pointerList[ i ];
//	}
//
//	list.Swap( other );
//}

#endif /* !__STRLIST_H__ */

