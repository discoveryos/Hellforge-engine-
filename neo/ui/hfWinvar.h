#ifndef __WINVAR_H__
#define __WINVAR_H__

#include "hfRectangle.h"

static const char *VAR_GUIPREFIX = "gui::";
static const int VAR_GUIPREFIX_LEN = strlen(VAR_GUIPREFIX);

class hfWindow;
class hfWinVar {
public:
	hfWinVar();
	virtual ~hfWinVar();

	void SetGuiInfo(hfDict *gd, const char *_name);
	const char *GetName() const { 
		if (name) {
			if (guiDict && *name == '*') {
				return guiDict->GetString(&name[1]);
			}
			return name;
		}
		return ""; 
	}
	void SetName(const char *_name) { 
		delete []name; 
		name = NULL;
		if (_name) {
			name = new (TAG_OLD_UI) char[strlen(_name)+1]; 
			strcpy(name, _name); 
		}
	}

	hfWinVar &operator=( const hfWinVar &other ) {
		guiDict = other.guiDict;
		SetName(other.name);
		return *this;
	}

	hfDict *GetDict() const { return guiDict; }
	bool NeedsUpdate() { return (guiDict != NULL); }

	virtual void Init(const char *_name, hfWindow* win) = 0;
	virtual void Set(const char *val) = 0;
	virtual void Update() = 0;
	virtual const char *c_str() const = 0;
	virtual size_t Size() {	size_t sz = (name) ? strlen(name) : 0; return sz + sizeof(*this); }

	virtual void WriteToSaveGame( hfFile *savefile ) = 0;
	virtual void ReadFromSaveGame( hfFile *savefile ) = 0;

	virtual float x() const = 0;

	void SetEval(bool b) {
		eval = b;
	}
	bool GetEval() {
		return eval;
	}
	
protected:
	hfDict *guiDict;
	char *name;
	bool eval;
};

class hfWinBool : public hfWinVar {
public:
	hfWinBool() : hfWinVar() {};
	~hfWinBool() {};
	virtual void Init(const char *_name, hfWindow *win) { hfWinVar::Init(_name, win);
		if (guiDict) {
			data = guiDict->GetBool(GetName());
		}
	}
	int	operator==(	const bool &other ) { return (other == data); }
	bool &operator=(	const bool &other ) {
		data = other;
		if (guiDict) {
			guiDict->SetBool(GetName(), data);
		}
		return data;
	}
	hfWinBool &operator=( const hfWinBool &other ) {
		hfWinVar::operator=(other);
		data = other.data;
		return *this;
	}

	operator bool() const { return data; }

	virtual void Set(const char *val) { 
		data = ( atoi( val ) != 0 );
		if (guiDict) {
			guiDict->SetBool(GetName(), data);
		}
	}

	virtual void Update() {	
		const char *s = GetName();
		if ( guiDict && s[0] != '\0' ) {
			data = guiDict->GetBool( s );
		}
	}

	virtual const char *c_str() const {return va("%i", data); }

	// SaveGames
	virtual void WriteToSaveGame( hfFile *savefile ) {
		savefile->Write( &eval, sizeof( eval ) );
		savefile->Write( &data, sizeof( data ) );
	}
	virtual void ReadFromSaveGame( hfFile *savefile ) {
		savefile->Read( &eval, sizeof( eval ) );
		savefile->Read( &data, sizeof( data ) );
	}

	virtual float x() const { return data ? 1.0f : 0.0f; };

protected:
	bool data;
};

class hfWinStr : public hfWinVar {
public:
	hfWinStr() : hfWinVar() {};
	~hfWinStr() {};
	virtual void Init(const char *_name, hfWindow *win) {
		hfWinVar::Init(_name, win);
		if (guiDict) {
			const char * name = GetName();
			if ( name[0] == 0 ) {
				data = "";
			} else {
				data = guiDict->GetString( name );
			}
		} 
	}
	int	operator==(	const hfStr &other ) const {
		return (other == data);
	}
	int	operator==(	const char *other ) const {
		return (data == other);
	}
	hfStr &operator=(	const hfStr &other ) {
		data = other;
		if (guiDict) {
			guiDict->Set(GetName(), data);
		}
		return data;
	}
	hfWinStr &operator=( const hfWinStr &other ) {
		hfWinVar::operator=(other);
		data = other.data;
		return *this;
	}
	operator const char *() const {
		return data.c_str();
	}
	operator const hfStr &() const {
		return data;
	}
	int LengthWithoutColors() {
		if (guiDict && name && *name) {
			data = guiDict->GetString(GetName());
		}
		return data.LengthWithoutColors();
	}
	int Length() {
		if (guiDict && name && *name) {
			data = guiDict->GetString(GetName());
		}
		return data.Length();
	}
	void RemoveColors() {
		if (guiDict && name && *name) {
			data = guiDict->GetString(GetName());
		}
		data.RemoveColors();
	}
	virtual const char *c_str() const {
		return data.c_str();
	}

	virtual void Set(const char *val) {
		data = val;
		if ( guiDict ) {
			guiDict->Set(GetName(), data);
		}
	}

	virtual void Update() {
		const char *s = GetName();
		if ( guiDict && s[0] != '\0' ) {
			data = guiDict->GetString( s );
		}
	}

	virtual size_t Size() {
		size_t sz = hfWinVar::Size();
		return sz +data.Allocated();
	}

	// SaveGames
	virtual void WriteToSaveGame( hfFile *savefile ) {
		savefile->Write( &eval, sizeof( eval ) );

		int len = data.Length();
		savefile->Write( &len, sizeof( len ) );
		if ( len > 0 ) {
			savefile->Write( data.c_str(), len );
		}
	}
	virtual void ReadFromSaveGame( hfFile *savefile ) {
		savefile->Read( &eval, sizeof( eval ) );

		int len;
		savefile->Read( &len, sizeof( len ) );
		if ( len > 0 ) {
			data.Fill( ' ', len );
			savefile->Read( &data[0], len );
		}
	}

	// return wether string is emtpy
	virtual float x() const { return data[0] ? 1.0f : 0.0f; };

protected:
	hfStr data;
};

class hfWinInt : public hfWinVar {
public:
	hfWinInt() : hfWinVar() {};
	~hfWinInt() {};
	virtual void Init(const char *_name, hfWindow *win) {
		hfWinVar::Init(_name,  win);
		if (guiDict) {
			data = guiDict->GetInt(GetName());
		} 
	}
	int &operator=(	const int &other ) {
		data = other;
		if (guiDict) {
			guiDict->SetInt(GetName(), data);
		}
		return data;
	}
	hfWinInt &operator=( const hfWinInt &other ) {
		hfWinVar::operator=(other);
		data = other.data;
		return *this;
	}
	operator int () const {
		return data;
	}
	virtual void Set(const char *val) {
		data = atoi(val);;
		if (guiDict) {
			guiDict->SetInt(GetName(), data);
		}
	}

	virtual void Update() {
		const char *s = GetName();
		if ( guiDict && s[0] != '\0' ) {
			data = guiDict->GetInt( s );
		}
	}
	virtual const char *c_str() const {
		return va("%i", data);
	}

	// SaveGames
	virtual void WriteToSaveGame( hfFile *savefile ) {
		savefile->Write( &eval, sizeof( eval ) );
		savefile->Write( &data, sizeof( data ) );
	}
	virtual void ReadFromSaveGame( hfFile *savefile ) {
		savefile->Read( &eval, sizeof( eval ) );
		savefile->Read( &data, sizeof( data ) );
	}

	// no suitable conversion
	virtual float x() const { assert( false ); return 0.0f; };

protected:
	int data;
};

class hfWinFloat : public hfWinVar {
public:
	hfWinFloat() : hfWinVar() {};
	~hfWinFloat() {};
	virtual void Init(const char *_name, hfWindow *win) {
		hfWinVar::Init(_name, win);
		if (guiDict) {
			data = guiDict->GetFloat(GetName());
		} 
	}
	hfWinFloat &operator=( const hfWinFloat &other ) {
		hfWinVar::operator=(other);
		data = other.data;
		return *this;
	}
	float &operator=(	const float &other ) {
		data = other;
		if (guiDict) {
			guiDict->SetFloat(GetName(), data);
		}
		return data;
	}
	operator float() const {
		return data;
	}
	virtual void Set(const char *val) {
		data = atof(val);
		if (guiDict) {
			guiDict->SetFloat(GetName(), data);
		}
	}
	virtual void Update() {
		const char *s = GetName();
		if ( guiDict && s[0] != '\0' ) {
			data = guiDict->GetFloat( s );
		}
	}
	virtual const char *c_str() const {
		return va("%f", data);
	}

	virtual void WriteToSaveGame( hfFile *savefile ) {
		savefile->Write( &eval, sizeof( eval ) );
		savefile->Write( &data, sizeof( data ) );
	}
	virtual void ReadFromSaveGame( hfFile *savefile ) {
		savefile->Read( &eval, sizeof( eval ) );
		savefile->Read( &data, sizeof( data ) );
	}

	virtual float x() const { return data; };
protected:
	float data;
};

class hfWinRectangle : public hfWinVar {
public:
	hfWinRectangle() : hfWinVar() {};
	~hfWinRectangle() {};
	virtual void Init(const char *_name, hfWindow *win) {
		hfWinVar::Init(_name, win);
		if (guiDict) {
			hfVec4 v = guiDict->GetVec4(GetName());
			data.x = v.x;
			data.y = v.y;
			data.w = v.z;
			data.h = v.w;
		} 
	}
	
	int	operator==(	const hfRectangle &other ) const {
		return (other == data);
	}

	hfWinRectangle &operator=( const hfWinRectangle &other ) {
		hfWinVar::operator=(other);
		data = other.data;
		return *this;
	}
	hfRectangle &operator=(	const hfVec4 &other ) {
		data = other;
		if (guiDict) {
			guiDict->SetVec4(GetName(), other);
		}
		return data;
	}

	hfRectangle &operator=(	const hfRectangle &other ) {
		data = other;
		if (guiDict) {
			hfVec4 v = data.ToVec4();
			guiDict->SetVec4(GetName(), v);
		}
		return data;
	}
	
	operator const hfRectangle&() const {
		return data;
	}

	float x() const {
		return data.x;
	}
	float y() const {
		return data.y;
	}
	float w() const {
		return data.w;
	}
	float h() const {
		return data.h;
	}
	float Right() const {
		return data.Right();
	}
	float Bottom() const {
		return data.Bottom();
	}
	hfVec4 &ToVec4() {
		static hfVec4 ret;
		ret = data.ToVec4();
		return ret;
	}
	virtual void Set(const char *val) {
		if ( strchr ( val, ',' ) ) {
			sscanf( val, "%f,%f,%f,%f", &data.x, &data.y, &data.w, &data.h );
		} else {
			sscanf( val, "%f %f %f %f", &data.x, &data.y, &data.w, &data.h );
		}
		if (guiDict) {
			hfVec4 v = data.ToVec4();
			guiDict->SetVec4(GetName(), v);
		}
	}
	virtual void Update() {
		const char *s = GetName();
		if ( guiDict && s[0] != '\0' ) {
			hfVec4 v = guiDict->GetVec4( s );
			data.x = v.x;
			data.y = v.y;
			data.w = v.z;
			data.h = v.w;
		}
	}

	virtual const char *c_str() const {
		return data.ToVec4().ToString();
	}

	virtual void WriteToSaveGame( hfFile *savefile ) {
		savefile->Write( &eval, sizeof( eval ) );
		savefile->Write( &data, sizeof( data ) );
	}
	virtual void ReadFromSaveGame( hfFile *savefile ) {
		savefile->Read( &eval, sizeof( eval ) );
		savefile->Read( &data, sizeof( data ) );
	}

protected:
	hfRectangle data;
};

class hfWinVec2 : public hfWinVar {
public:
	hfWinVec2() : hfWinVar() {};
	~hfWinVec2() {};
	virtual void Init(const char *_name, hfWindow *win) {
		hfWinVar::Init(_name, win);
		if (guiDict) {
			data = guiDict->GetVec2(GetName());
		} 
	}
	int	operator==(	const hfVec2 &other ) const {
		return (other == data);
	}
	hfWinVec2 &operator=( const hfWinVec2 &other ) {
		hfWinVar::operator=(other);
		data = other.data;
		return *this;
	}
	
	hfVec2 &operator=(	const hfVec2 &other ) {
		data = other;
		if (guiDict) {
			guiDict->SetVec2(GetName(), data);
		}
		return data;
	}
	float x() const {
		return data.x;
	}
	float y() const {
		return data.y;
	}
	virtual void Set(const char *val) {
		if ( strchr ( val, ',' ) ) {
			sscanf( val, "%f,%f", &data.x, &data.y );
		} else {
		sscanf( val, "%f %f", &data.x, &data.y);
		}
		if (guiDict) {
			guiDict->SetVec2(GetName(), data);
		}
	}
	operator const hfVec2&() const {
		return data;
	}
	virtual void Update() {
		const char *s = GetName();
		if ( guiDict && s[0] != '\0' ) {
			data = guiDict->GetVec2( s );
		}
	}
	virtual const char *c_str() const {
		return data.ToString();
	}
	void Zero() {
		data.Zero();
	}

	virtual void WriteToSaveGame( hfFile *savefile ) {
		savefile->Write( &eval, sizeof( eval ) );
		savefile->Write( &data, sizeof( data ) );
	}
	virtual void ReadFromSaveGame( hfFile *savefile ) {
		savefile->Read( &eval, sizeof( eval ) );
		savefile->Read( &data, sizeof( data ) );
	}

protected:
	hfVec2 data;
};

class hfWinVec4 : public hfWinVar {
public:
	hfWinVec4() : hfWinVar() {};
	~hfWinVec4() {};
	virtual void Init(const char *_name, hfWindow *win) {
		hfWinVar::Init(_name, win);
		if (guiDict) {
			data = guiDict->GetVec4(GetName());
		} 
	}
	int	operator==(	const hfVec4 &other ) const {
		return (other == data);
	}
	hfWinVec4 &operator=( const hfWinVec4 &other ) {
		hfWinVar::operator=(other);
		data = other.data;
		return *this;
	}
	hfVec4 &operator=(	const hfVec4 &other ) {
		data = other;
		if (guiDict) {
			guiDict->SetVec4(GetName(), data);
		}
		return data;
	}
	operator const hfVec4&() const {
		return data;
	}

	float x() const {
		return data.x;
	}

	float y() const {
		return data.y;
	}

	float z() const {
		return data.z;
	}

	float w() const {
		return data.w;
	}
	virtual void Set(const char *val) {
		if ( strchr ( val, ',' ) ) {
			sscanf( val, "%f,%f,%f,%f", &data.x, &data.y, &data.z, &data.w );
		} else {
			sscanf( val, "%f %f %f %f", &data.x, &data.y, &data.z, &data.w);
		}
		if ( guiDict ) {
			guiDict->SetVec4( GetName(), data );
		}
	}
	virtual void Update() {
		const char *s = GetName();
		if ( guiDict && s[0] != '\0' ) {
			data = guiDict->GetVec4( s );
		}
	}
	virtual const char *c_str() const {
		return data.ToString();
	}

	void Zero() {
		data.Zero();
		if ( guiDict ) {
			guiDict->SetVec4(GetName(), data);
		}
	}

	const hfVec3 &ToVec3() const {
		return data.ToVec3();
	}

	virtual void WriteToSaveGame( hfFile *savefile ) {
		savefile->Write( &eval, sizeof( eval ) );
		savefile->Write( &data, sizeof( data ) );
	}
	virtual void ReadFromSaveGame( hfFile *savefile ) {
		savefile->Read( &eval, sizeof( eval ) );
		savefile->Read( &data, sizeof( data ) );
	}

protected:
	hfVec4 data;
};

class hfWinVec3 : public hfWinVar {
public:
	hfWinVec3() : hfWinVar() {};
	~hfWinVec3() {};
	virtual void Init(const char *_name, hfWindow *win) {
		hfWinVar::Init(_name, win);
		if (guiDict) {
			data = guiDict->GetVector(GetName());
		} 
	}
	int	operator==(	const hfVec3 &other ) const {
		return (other == data);
	}
	hfWinVec3 &operator=( const hfWinVec3 &other ) {
		hfWinVar::operator=(other);
		data = other.data;
		return *this;
	}
	hfVec3 &operator=(	const hfVec3 &other ) {
		data = other;
		if (guiDict) {
			guiDict->SetVector(GetName(), data);
		}
		return data;
	}
	operator const hfVec3&() const {
		return data;
	}

	float x() const {
		return data.x;
	}

	float y() const {
		return data.y;
	}

	float z() const {
		return data.z;
	}

	virtual void Set(const char *val) {
		sscanf( val, "%f %f %f", &data.x, &data.y, &data.z);
		if (guiDict) {
			guiDict->SetVector(GetName(), data);
		}
	}
	virtual void Update() {
		const char *s = GetName();
		if ( guiDict && s[0] != '\0' ) {
			data = guiDict->GetVector( s );
		}
	}
	virtual const char *c_str() const {
		return data.ToString();
	}

	void Zero() {
		data.Zero();
		if (guiDict) {
			guiDict->SetVector(GetName(), data);
		}
	}

	virtual void WriteToSaveGame( hfFile *savefile ) {
		savefile->Write( &eval, sizeof( eval ) );
		savefile->Write( &data, sizeof( data ) );
	}
	virtual void ReadFromSaveGame( hfFile *savefile ) {
		savefile->Read( &eval, sizeof( eval ) );
		savefile->Read( &data, sizeof( data ) );
	}

protected:
	hfVec3 data;
};

class hfWinBackground : public hfWinStr {
public:
	hfWinBackground() : hfWinStr() {
		mat = NULL;
	};
	~hfWinBackground() {};
	virtual void Init(const char *_name, hfWindow *win) {
		hfWinStr::Init(_name, win);
		if (guiDict) {
			data = guiDict->GetString(GetName());
		} 
	}
	int	operator==(	const hfStr &other ) const {
		return (other == data);
	}
	int	operator==(	const char *other ) const {
		return (data == other);
	}
	hfStr &operator=(	const hfStr &other ) {
		data = other;
		if (guiDict) {
			guiDict->Set(GetName(), data);
		}
		if (mat) {
			if ( data == "" ) {
				(*mat) = NULL;
			} else {
				(*mat) = declManager->FindMaterial(data);
			}
		}
		return data;
	}
	hfWinBackground &operator=( const hfWinBackground &other ) {
		hfWinVar::operator=(other);
		data = other.data;
		mat = other.mat;
		if (mat) {
			if ( data == "" ) {
				(*mat) = NULL;
			} else {
				(*mat) = declManager->FindMaterial(data);
			}
		}
		return *this;
	}
	operator const char *() const {
		return data.c_str();
	}
	operator const hfStr &() const {
		return data;
	}
	int Length() {
		if (guiDict) {
			data = guiDict->GetString(GetName());
		}
		return data.Length();
	}
	virtual const char *c_str() const {
		return data.c_str();
	}

	virtual void Set(const char *val) {
		data = val;
		if (guiDict) {
			guiDict->Set(GetName(), data);
		}
		if (mat) {
			if ( data == "" ) {
				(*mat) = NULL;
			} else {
				(*mat) = declManager->FindMaterial(data);
			}
		}
	}

	virtual void Update() {
		const char *s = GetName();
		if ( guiDict && s[0] != '\0' ) {
			data = guiDict->GetString( s );
			if (mat) {
				if ( data == "" ) {
					(*mat) = NULL;
				} else {
					(*mat) = declManager->FindMaterial(data);
				}
			}
		}
	}

	virtual size_t Size() {
		size_t sz = hfWinVar::Size();
		return sz +data.Allocated();
	}

	void SetMaterialPtr( const hfMaterial **m ) {
		mat = m;
	}

	virtual void WriteToSaveGame( hfFile *savefile ) {
		savefile->Write( &eval, sizeof( eval ) );

		int len = data.Length();
		savefile->Write( &len, sizeof( len ) );
		if ( len > 0 ) {
			savefile->Write( data.c_str(), len );
		}
	}
	virtual void ReadFromSaveGame( hfFile *savefile ) {
		savefile->Read( &eval, sizeof( eval ) );

		int len;
		savefile->Read( &len, sizeof( len ) );
		if ( len > 0 ) {
			data.Fill( ' ', len );
			savefile->Read( &data[0], len );
		}
		if ( mat ) {
			if ( len > 0 ) {
				(*mat) = declManager->FindMaterial( data );
			} else {
				(*mat) = NULL;
			}
		}
	}

protected:
	hfStr data;
	const hfMaterial **mat;
};

/*
================
hfMultiWinVar
multiplexes access to a list if hfWinVar*
================
*/
class hfMultiWinVar : public hfList< hfWinVar * > {
public:
	void Set( const char *val );
	void Update();
	void SetGuiInfo( hfDict *dict );
};

#endif /* !__WINVAR_H__ */


