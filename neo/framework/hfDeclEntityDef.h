#ifndef __DECLENTITYDEF_H__
#define __DECLENTITYDEF_H__

/*
===============================================================================

	hfDeclEntityDef

===============================================================================
*/

class hfDeclEntityDef : public hfDecl {
public:
	hfDict					dict;

	virtual size_t			Size() const;
	virtual const char *	DefaultDefinition() const;
	virtual bool			Parse( const char *text, const int textLength, bool allowBinaryVersion );
	virtual void			FreeData();
	virtual void			Print();
};

#endif /* !__DECLENTITYDEF_H__ */

