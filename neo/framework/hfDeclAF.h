#ifndef __DECLAF_H__
#define __DECLAF_H__

/*
===============================================================================

	Articulated Figure

===============================================================================
*/

class hfDeclAF;

typedef enum {
	DECLAF_CONSTRAINT_INVALID,
	DECLAF_CONSTRAINT_FIXED,
	DECLAF_CONSTRAINT_BALLANDSOCKETJOINT,
	DECLAF_CONSTRAINT_UNIVERSALJOINT,
	DECLAF_CONSTRAINT_HINGE,
	DECLAF_CONSTRAINT_SLIDER,
	DECLAF_CONSTRAINT_SPRING
} declAFConstraintType_t;

typedef enum {
	DECLAF_JOINTMOD_AXIS,
	DECLAF_JOINTMOD_ORIGIN,
	DECLAF_JOINTMOD_BOTH
} declAFJointMod_t;

typedef bool (*getJointTransform_t)( void *model, const hfJointMat *frame, const char *jointName, hfVec3 &origin, hfMat3 &axis );

class hfAFVector {
public:
	enum {
		VEC_COORDS = 0,
		VEC_JOINT,
		VEC_BONECENTER,
		VEC_BONEDIR
	}						type;
	hfStr					joint1;
	hfStr					joint2;

public:
							hfAFVector();

	bool					Parse( hfLexer &src );
	bool					Finish( const char *fileName, const getJointTransform_t GetJointTransform, const hfJointMat *frame, void *model ) const;
	bool					Write( hfFile *f ) const;
	const char *			ToString( hfStr &str, const int precision = 8 );
	const hfVec3 &			ToVec3() const { return vec; }
	hfVec3 &				ToVec3() { return vec; }

private:
	mutable hfVec3			vec;
	bool					negate;
};

class hfDeclAF_Body {
public:
	hfStr					name;
	hfStr					jointName;
	declAFJointMod_t		jointMod;
	int						modelType;
	hfAFVector				v1, v2;
	int						numSides;
	float					width;
	float					density;
	hfAFVector				origin;
	hfAngles				angles;
	int						contents;
	int						clipMask;
	bool					selfCollision;
	hfMat3					inertiaScale;
	float					linearFriction;
	float					angularFriction;
	float					contactFriction;
	hfStr					containedJoints;
	hfAFVector				frictionDirection;
	hfAFVector				contactMotorDirection;
public:
	void					SetDefault( const hfDeclAF *file );
};

class hfDeclAF_Constraint {
public:
	hfStr					name;
	hfStr					body1;
	hfStr					body2;
	declAFConstraintType_t	type;
	float					friction;
	float					stretch;
	float					compress;
	float					damping;
	float					restLength;
	float					minLength;
	float					maxLength;
	hfAFVector				anchor;
	hfAFVector				anchor2;
	hfAFVector				shaft[2];
	hfAFVector				axis;
	enum {
		LIMIT_NONE = -1,
		LIMIT_CONE,
		LIMIT_PYRAMID
	}						limit;
	hfAFVector				limitAxis;
	float					limitAngles[3];

public:
	void					SetDefault( const hfDeclAF *file );
};

class hfDeclAF : public hfDecl {
	friend class hfAFFileManager;
public:
							hfDeclAF();
	virtual					~hfDeclAF();

	virtual size_t			Size() const;
	virtual const char *	DefaultDefinition() const;
	virtual bool			Parse( const char *text, const int textLength, bool allowBinaryVersion );
	virtual void			FreeData();

	virtual void			Finish( const getJointTransform_t GetJointTransform, const hfJointMat *frame, void *model ) const;

	bool					Save();

	void					NewBody( const char *name );
	void					RenameBody( const char *oldName, const char *newName );
	void					DeleteBody( const char *name );

	void					NewConstraint( const char *name );
	void					RenameConstraint( const char *oldName, const char *newName );
	void					DeleteConstraint( const char *name );

	static int				ContentsFromString( const char *str );
	static const char *		ContentsToString( const int contents, hfStr &str );

	static declAFJointMod_t	JointModFromString( const char *str );
	static const char *		JointModToString( declAFJointMod_t jointMod );

public:
	bool					modified;
	hfStr					model;
	hfStr					skin;
	float					defaultLinearFriction;
	float					defaultAngularFriction;
	float					defaultContactFriction;
	float					defaultConstraintFriction;
	float					totalMass;
	hfVec2					suspendVelocity;
	hfVec2					suspendAcceleration;
	float					noMoveTime;
	float					noMoveTranslation;
	float					noMoveRotation;
	float					minMoveTime;
	float					maxMoveTime;
	int						contents;
	int						clipMask;
	bool					selfCollision;
	hfList<hfDeclAF_Body *, TAG_IDLIB_LIST_PHYSICS>			bodies;
	hfList<hfDeclAF_Constraint *, TAG_IDLIB_LIST_PHYSICS>	constraints;

private:
	bool					ParseContents( hfLexer &src, int &c ) const;
	bool					ParseBody( hfLexer &src );
	bool					ParseFixed( hfLexer &src );
	bool					ParseBallAndSocketJoint( hfLexer &src );
	bool					ParseUniversalJoint( hfLexer &src );
	bool					ParseHinge( hfLexer &src );
	bool					ParseSlider( hfLexer &src );
	bool					ParseSpring( hfLexer &src );
	bool					ParseSettings( hfLexer &src );

	bool					WriteBody( hfFile *f, const hfDeclAF_Body &body ) const;
	bool					WriteFixed( hfFile *f, const hfDeclAF_Constraint &c ) const;
	bool					WriteBallAndSocketJoint( hfFile *f, const hfDeclAF_Constraint &c ) const;
	bool					WriteUniversalJoint( hfFile *f, const hfDeclAF_Constraint &c ) const;
	bool					WriteHinge( hfFile *f, const hfDeclAF_Constraint &c ) const;
	bool					WriteSlider( hfFile *f, const hfDeclAF_Constraint &c ) const;
	bool					WriteSpring( hfFile *f, const hfDeclAF_Constraint &c ) const;
	bool					WriteConstraint( hfFile *f, const hfDeclAF_Constraint &c ) const;
	bool					WriteSettings( hfFile *f ) const;

	bool					RebuildTextSource();
};

#endif /* !__DECLAF_H__ */

