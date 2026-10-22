#ifndef __LINKLIST_H__
#define __LINKLIST_H__

/*
==============================================================================

hfLinkList

Circular linked list template

==============================================================================
*/

template< class type >
class hfLinkList {
public:
						hfLinkList();
						~hfLinkList();

	bool				IsListEmpty() const;
	bool				InList() const;
	int					Num() const;
	void				Clear();

	void				InsertBefore( hfLinkList &node );
	void				InsertAfter( hfLinkList &node );
	void				AddToEnd( hfLinkList &node );
	void				AddToFront( hfLinkList &node );

	void				Remove();

	type *				Next() const;
	type *				Prev() const;

	type *				Owner() const;
	void				SetOwner( type *object );

	hfLinkList *		ListHead() const;
	hfLinkList *		NextNode() const;
	hfLinkList *		PrevNode() const;

private:
	hfLinkList *		head;
	hfLinkList *		next;
	hfLinkList *		prev;
	type *				owner;
};

/*
================
hfLinkList<type>::hfLinkList

Node is initialized to be the head of an empty list
================
*/
template< class type >
hfLinkList<type>::hfLinkList() {
	owner	= NULL;
	head	= this;	
	next	= this;
	prev	= this;
}

/*
================
hfLinkList<type>::~hfLinkList

Removes the node from the list, or if it's the head of a list, removes
all the nodes from the list.
================
*/
template< class type >
hfLinkList<type>::~hfLinkList() {
	Clear();
}

/*
================
hfLinkList<type>::IsListEmpty

Returns true if the list is empty.
================
*/
template< class type >
bool hfLinkList<type>::IsListEmpty() const {
	return head->next == head;
}

/*
================
hfLinkList<type>::InList

Returns true if the node is in a list.  If called on the head of a list, will always return false.
================
*/
template< class type >
bool hfLinkList<type>::InList() const {
	return head != this;
}

/*
================
hfLinkList<type>::Num

Returns the number of nodes in the list.
================
*/
template< class type >
int hfLinkList<type>::Num() const {
	hfLinkList<type>	*node;
	int					num;

	num = 0;
	for( node = head->next; node != head; node = node->next ) {
		num++;
	}

	return num;
}

/*
================
hfLinkList<type>::Clear

If node is the head of the list, clears the list.  Otherwise it just removes the node from the list.
================
*/
template< class type >
void hfLinkList<type>::Clear() {
	if ( head == this ) {
		while( next != this ) {
			next->Remove();
		}
	} else {
		Remove();
	}
}

/*
================
hfLinkList<type>::Remove

Removes node from list
================
*/
template< class type >
void hfLinkList<type>::Remove() {
	prev->next = next;
	next->prev = prev;

	next = this;
	prev = this;
	head = this;
}

/*
================
hfLinkList<type>::InsertBefore

Places the node before the existing node in the list.  If the existing node is the head,
then the new node is placed at the end of the list.
================
*/
template< class type >
void hfLinkList<type>::InsertBefore( hfLinkList &node ) {
	Remove();

	next		= &node;
	prev		= node.prev;
	node.prev	= this;
	prev->next	= this;
	head		= node.head;
}

/*
================
hfLinkList<type>::InsertAfter

Places the node after the existing node in the list.  If the existing node is the head,
then the new node is placed at the beginning of the list.
================
*/
template< class type >
void hfLinkList<type>::InsertAfter( hfLinkList &node ) {
	Remove();

	prev		= &node;
	next		= node.next;
	node.next	= this;
	next->prev	= this;
	head		= node.head;
}

/*
================
hfLinkList<type>::AddToEnd

Adds node at the end of the list
================
*/
template< class type >
void hfLinkList<type>::AddToEnd( hfLinkList &node ) {
	InsertBefore( *node.head );
}

/*
================
hfLinkList<type>::AddToFront

Adds node at the beginning of the list
================
*/
template< class type >
void hfLinkList<type>::AddToFront( hfLinkList &node ) {
	InsertAfter( *node.head );
}

/*
================
hfLinkList<type>::ListHead

Returns the head of the list.  If the node isn't in a list, it returns
a pointer to itself.
================
*/
template< class type >
hfLinkList<type> *hfLinkList<type>::ListHead() const {
	return head;
}

/*
================
hfLinkList<type>::Next

Returns the next object in the list, or NULL if at the end.
================
*/
template< class type >
type *hfLinkList<type>::Next() const {
	if ( !next || ( next == head ) ) {
		return NULL;
	}
	return next->owner;
}

/*
================
hfLinkList<type>::Prev

Returns the previous object in the list, or NULL if at the beginning.
================
*/
template< class type >
type *hfLinkList<type>::Prev() const {
	if ( !prev || ( prev == head ) ) {
		return NULL;
	}
	return prev->owner;
}

/*
================
hfLinkList<type>::NextNode

Returns the next node in the list, or NULL if at the end.
================
*/
template< class type >
hfLinkList<type> *hfLinkList<type>::NextNode() const {
	if ( next == head ) {
		return NULL;
	}
	return next;
}

/*
================
hfLinkList<type>::PrevNode

Returns the previous node in the list, or NULL if at the beginning.
================
*/
template< class type >
hfLinkList<type> *hfLinkList<type>::PrevNode() const {
	if ( prev == head ) {
		return NULL;
	}
	return prev;
}

/*
================
hfLinkList<type>::Owner

Gets the object that is associated with this node.
================
*/
template< class type >
type *hfLinkList<type>::Owner() const {
	return owner;
}

/*
================
hfLinkList<type>::SetOwner

Sets the object that this node is associated with.
================
*/
template< class type >
void hfLinkList<type>::SetOwner( type *object ) {
	owner = object;
}

#endif /* !__LINKLIST_H__ */

