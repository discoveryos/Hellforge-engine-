#ifndef __DECLFX_H__
#define __DECLFX_H__

/*
===============================================================================

	hfDeclFX

===============================================================================
*/

enum { 
	FX_LIGHT,
	FX_PARTICLE,
	FX_DECAL,
	FX_MODEL,
	FX_SOUND,
	FX_SHAKE,
	FX_ATTACHLIGHT,
	FX_ATTACHENTITY,
	FX_LAUNCH,
	FX_SHOCKWAVE
};

//
// single fx structure
//
typedef struct {
	int						type;
	int						sibling;

	hfStr					data;
	hfStr					name;
	hfStr					fire;

	float					delay;
	float					duration;
	float					restart;
	float					size;
	float					fadeInTime;
	float					fadeOutTime;
	float					shakeTime;
	float					shakeAmplitude;
	float					shakeDistance;
	float					shakeImpulse;
	float					lightRadius;
	float					rotate;
	float					random1;
	float					random2;

	hfVec3					lightColor;
	hfVec3					offset;
	hfMat3					axis;

	bool					soundStarted;
	bool					shakeStarted;
	bool					shakeFalloff;
	bool					shakeIgnoreMaster;
	bool					bindParticles;
	bool					explicitAxis;
	bool					noshadows;
	bool					particleTrackVelocity;
	bool					trackOrigin;
} hfFXSingleAction;

//
// grouped fx structures
//
class hfDeclFX : public hfDecl {
public:
	virtual size_t			Size() const;
	virtual const char *	DefaultDefinition() const;
	virtual bool			Parse( const char *text, const int textLength, bool allowBinaryVersion );
	virtual void			FreeData();
	virtual void			Print() const;
	virtual void			List() const;

	hfList<hfFXSingleAction, TAG_FX>events;
	hfStr					joint;

private:
	void					ParseSingleFXAction( hfLexer &src, hfFXSingleAction& FXAction );
};

#endif /* !__DECLFX_H__ */

