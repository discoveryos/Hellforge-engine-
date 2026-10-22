#ifndef __STACK_H__
#define __STACK_H__

/*
===============================================================================

	Stack template

===============================================================================
*/

#define hfStack( type, next )		hfStackTemplate<type, (int)&(((type*)NULL)->next)>

template< class type, int nextOffset >
class hfStackTemplate {
public:
							hfStackTemplate();

	void					Add( type *element );
	type *					Get();

private:
	type *					top;
	type *					bottom;
};

#define STACK_NEXT_PTR( element )		(*(type**)(((byte*)element)+nextOffset))

template< class type, int nextOffset >
hfStackTemplate<type,nextOffset>::hfStackTemplate() {
	top = bottom = NULL;
}

template< class type, int nextOffset >
void hfStackTemplate<type,nextOffset>::Add( type *element ) {
	STACK_NEXT_PTR(element) = top;
	top = element;
	if ( !bottom ) {
		bottom = element;
	}
}

template< class type, int nextOffset >
type *hfStackTemplate<type,nextOffset>::Get() {
	type *element;

	element = top;
	if ( element ) {
		top = STACK_NEXT_PTR(top);
		if ( bottom == element ) {
			bottom = NULL;
		}
		STACK_NEXT_PTR(element) = NULL;
	}
	return element;
}

#endif /* !__STACK_H__ */

