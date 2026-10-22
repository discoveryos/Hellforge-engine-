#ifndef __HIERARCHY_H__
#define __HIERARCHY_H__

/*
==============================================================================

	hfHierarchy

==============================================================================
*/

template< class type >
class hfHierarchy {
public:

						hfHierarchy();
						~hfHierarchy();
	
	void				SetOwner( type *object );
	type *				Owner() const;
	void				ParentTo( hfHierarchy &node );
	void				MakeSiblingAfter( hfHierarchy &node );
	bool				ParentedBy( const hfHierarchy &node ) const;
	void				RemoveFromParent();
	void				RemoveFromHierarchy();

	type *				GetParent() const;		// parent of this node
	type *				GetChild() const;			// first child of this node
	type *				GetSibling() const;		// next node with the same parent
	type *				GetPriorSibling() const;	// previous node with the same parent
	type *				GetNext() const;			// goes through all nodes of the hierarchy
	type *				GetNextLeaf() const;		// goes through all leaf nodes of the hierarchy

private:
	hfHierarchy *		parent;
	hfHierarchy *		sibling;
	hfHierarchy *		child;
	type *				owner;

	hfHierarchy<type>	*GetPriorSiblingNode() const;	// previous node with the same parent
};

/*
================
hfHierarchy<type>::hfHierarchy
================
*/
template< class type >
hfHierarchy<type>::hfHierarchy() {
	owner	= NULL;
	parent	= NULL;	
	sibling	= NULL;
	child	= NULL;
}

/*
================
hfHierarchy<type>::~hfHierarchy
================
*/
template< class type >
hfHierarchy<type>::~hfHierarchy() {
	RemoveFromHierarchy();
}

/*
================
hfHierarchy<type>::Owner

Gets the object that is associated with this node.
================
*/
template< class type >
type *hfHierarchy<type>::Owner() const {
	return owner;
}

/*
================
hfHierarchy<type>::SetOwner

Sets the object that this node is associated with.
================
*/
template< class type >
void hfHierarchy<type>::SetOwner( type *object ) {
	owner = object;
}

/*
================
hfHierarchy<type>::ParentedBy
================
*/
template< class type >
bool hfHierarchy<type>::ParentedBy( const hfHierarchy &node ) const {
	if ( parent == &node ) {
		return true;
	} else if ( parent ) {
		return parent->ParentedBy( node );
	}
	return false;
}

/*
================
hfHierarchy<type>::ParentTo

Makes the given node the parent.
================
*/
template< class type >
void hfHierarchy<type>::ParentTo( hfHierarchy &node ) {
	RemoveFromParent();

	parent		= &node;
	sibling		= node.child;
	node.child	= this;
}

/*
================
hfHierarchy<type>::MakeSiblingAfter

Makes the given node a sibling after the passed in node.
================
*/
template< class type >
void hfHierarchy<type>::MakeSiblingAfter( hfHierarchy &node ) {
	RemoveFromParent();
	parent	= node.parent;
	sibling = node.sibling;
	node.sibling = this;
}

/*
================
hfHierarchy<type>::RemoveFromParent
================
*/
template< class type >
void hfHierarchy<type>::RemoveFromParent() {
	hfHierarchy<type> *prev;

	if ( parent ) {
		prev = GetPriorSiblingNode();
		if ( prev ) {
			prev->sibling = sibling;
		} else {
			parent->child = sibling;
		}
	}

	parent = NULL;
	sibling = NULL;
}

/*
================
hfHierarchy<type>::RemoveFromHierarchy

Removes the node from the hierarchy and adds it's children to the parent.
================
*/
template< class type >
void hfHierarchy<type>::RemoveFromHierarchy() {
	hfHierarchy<type> *parentNode;
	hfHierarchy<type> *node;

	parentNode = parent;
	RemoveFromParent();

	if ( parentNode ) {
		while( child ) {
			node = child;
			node->RemoveFromParent();
			node->ParentTo( *parentNode );
		}
	} else {
		while( child ) {
			child->RemoveFromParent();
		}
	}
}

/*
================
hfHierarchy<type>::GetParent
================
*/
template< class type >
type *hfHierarchy<type>::GetParent() const {
	if ( parent ) {
		return parent->owner;
	}
	return NULL;
}

/*
================
hfHierarchy<type>::GetChild
================
*/
template< class type >
type *hfHierarchy<type>::GetChild() const {
	if ( child ) {
		return child->owner;
	}
	return NULL;
}

/*
================
hfHierarchy<type>::GetSibling
================
*/
template< class type >
type *hfHierarchy<type>::GetSibling() const {
	if ( sibling ) {
		return sibling->owner;
	}
	return NULL;
}

/*
================
hfHierarchy<type>::GetPriorSiblingNode

Returns NULL if no parent, or if it is the first child.
================
*/
template< class type >
hfHierarchy<type> *hfHierarchy<type>::GetPriorSiblingNode() const {
	if ( !parent || ( parent->child == this ) ) {
		return NULL;
	}

	hfHierarchy<type> *prev;
	hfHierarchy<type> *node;

	node = parent->child;
	prev = NULL;
	while( ( node != this ) && ( node != NULL ) ) {
		prev = node;
		node = node->sibling;
	}

	if ( node != this ) {
		hfLib::Error( "hfHierarchy::GetPriorSibling: could not find node in parent's list of children" );
	}

	return prev;
}

/*
================
hfHierarchy<type>::GetPriorSibling

Returns NULL if no parent, or if it is the first child.
================
*/
template< class type >
type *hfHierarchy<type>::GetPriorSibling() const {
	hfHierarchy<type> *prior;

	prior = GetPriorSiblingNode();
	if ( prior ) {
		return prior->owner;
	}

	return NULL;
}

/*
================
hfHierarchy<type>::GetNext

Goes through all nodes of the hierarchy.
================
*/
template< class type >
type *hfHierarchy<type>::GetNext() const {
	const hfHierarchy<type> *node;

	if ( child ) {
		return child->owner;
	} else {
		node = this;
		while( node && node->sibling == NULL ) {
			node = node->parent;
		}
		if ( node ) {
			return node->sibling->owner;
		} else {
			return NULL;
		}
	}
}

/*
================
hfHierarchy<type>::GetNextLeaf

Goes through all leaf nodes of the hierarchy.
================
*/
template< class type >
type *hfHierarchy<type>::GetNextLeaf() const {
	const hfHierarchy<type> *node;

	if ( child ) {
		node = child;
		while ( node->child ) {
			node = node->child;
		}
		return node->owner;
	} else {
		node = this;
		while( node && node->sibling == NULL ) {
			node = node->parent;
		}
		if ( node ) {
			node = node->sibling;
			while ( node->child ) {
				node = node->child;
			}
			return node->owner;
		} else {
			return NULL;
		}
	}
}

#endif /* !__HIERARCHY_H__ */

