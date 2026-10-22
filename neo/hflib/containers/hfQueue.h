#ifndef __QUEUE_H__
#define __QUEUE_H__

/*
===============================================================================

	Queue template

===============================================================================
*/
template< class type, int nextOffset >
class hfQueueTemplate {
public:
							hfQueueTemplate();

	void					Add( type *element );
	type *					Get();

private:
	type *					first;
	type *					last;
};

#define QUEUE_NEXT_PTR( element )		(*((type**)(((byte*)element)+nextOffset)))

template< class type, int nextOffset >
hfQueueTemplate<type,nextOffset>::hfQueueTemplate() {
	first = last = NULL;
}

template< class type, int nextOffset >
void hfQueueTemplate<type,nextOffset>::Add( type *element ) {
	QUEUE_NEXT_PTR(element) = NULL;
	if ( last ) {
		QUEUE_NEXT_PTR(last) = element;
	} else {
		first = element;
	}
	last = element;
}

template< class type, int nextOffset >
type *hfQueueTemplate<type,nextOffset>::Get() {
	type *element;

	element = first;
	if ( element ) {
		first = QUEUE_NEXT_PTR(first);
		if ( last == element ) {
			last = NULL;
		}
		QUEUE_NEXT_PTR(element) = NULL;
	}
	return element;
}

/*
================================================
A node of a Queue
================================================
*/
template< typename type >
class hfQueueNode {
public:
				hfQueueNode() { next = NULL; }

	type *		GetNext() const { return next; }
	void		SetNext( type *next ) { this->next = next; }

private:
	type *		next;
};

/*
================================================
A Queue, hfQueue, is a template Container class implementing the Queue abstract data 
type.
================================================
*/
template< typename type, hfQueueNode<type> type::*nodePtr >
class hfQueue {
public:
				hfQueue();

	void		Add( type *element );
	type *		RemoveFirst();
	type *		Peek() const;
	bool		IsEmpty();

	static void	Test();

private:
	type *		first;
	type *		last;
};

/*
========================
hfQueue<type,nodePtr>::hfQueue
========================
*/
template< typename type, hfQueueNode<type> type::*nodePtr >
hfQueue<type,nodePtr>::hfQueue() {
	first = last = NULL;
}

/*
========================
hfQueue<type,nodePtr>::Add
========================
*/
template< typename type, hfQueueNode<type> type::*nodePtr >
void hfQueue<type,nodePtr>::Add( type *element ) {
	(element->*nodePtr).SetNext( NULL );
	if ( last ) {
		(last->*nodePtr).SetNext( element );
	} else {
		first = element;
	}
	last = element;
}

/*
========================
hfQueue<type,nodePtr>::RemoveFirst
========================
*/
template< typename type, hfQueueNode<type> type::*nodePtr >
type *hfQueue<type,nodePtr>::RemoveFirst() {
	type *element;

	element = first;
	if ( element ) {
		first = (first->*nodePtr).GetNext();
		if ( last == element ) {
			last = NULL;
		}
		(element->*nodePtr).SetNext( NULL );
	}
	return element;
}

/*
========================
hfQueue<type,nodePtr>::Peek
========================
*/
template< typename type, hfQueueNode<type> type::*nodePtr >
type *hfQueue<type,nodePtr>::Peek() const {
	return first;
}

/*
========================
hfQueue<type,nodePtr>::IsEmpty
========================
*/
template< typename type, hfQueueNode<type> type::*nodePtr >
bool hfQueue<type,nodePtr>::IsEmpty() {
	return ( first == NULL );
}

/*
========================
hfQueue<type,nodePtr>::Test
========================
*/
template< typename type, hfQueueNode<type> type::*nodePtr >
void hfQueue<type,nodePtr>::Test() {

	class hfMyType {
	public:
		hfQueueNode<hfMyType> queueNode;
	};

	hfQueue<hfMyType,&hfMyType::queueNode> myQueue;

	hfMyType *element = new (TAG_IDLIB) hfMyType;
	myQueue.Add( element );
	element = myQueue.RemoveFirst();
	delete element;
}

#endif // !__QUEUE_H__

