#ifndef __DECLSKIN_H__
#define __DECLSKIN_H__

/*
===============================================================================

	hfDeclSkin

===============================================================================
*/

typedef struct {
	const hfMaterial *		from;			// 0 == any unmatched shader
	const hfMaterial *		to;
} skinMapping_t;

class hfDeclSkin : public hfDecl {
public:
	virtual size_t			Size() const;
	virtual bool			SetDefaultText();
	virtual const char *	DefaultDefinition() const;
	virtual bool			Parse( const char *text, const int textLength, bool allowBinaryVersion );
	virtual void			FreeData();

	const hfMaterial *		RemapShaderBySkin( const hfMaterial *shader ) const;

							// model associations are just for the preview dialog in the editor
	const int				GetNumModelAssociations() const;
	const char *			GetAssociatedModel( int index ) const;

private:
	hfList<skinMapping_t, TAG_IDLIB_LIST_DECL>	mappings;
	hfStrList				associatedModels;
};

#endif /* !__DECLSKIN_H__ */

