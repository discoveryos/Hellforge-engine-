#ifndef __REGEXP_H__
#define __REGEXP_H__

class hfTokenParser;
class hfWindow;
class hfWinVar;

class hfRegister {
public:
						hfRegister();
						hfRegister( const char *p, int t );

	enum REGTYPE { VEC4 = 0, FLOAT, BOOL, INT, STRING, VEC2, VEC3, RECTANGLE, NUMTYPES } ;
	static int REGCOUNT[NUMTYPES];

	bool				enabled;
	short				type;
	hfStr				name;
	int					regCount;
	unsigned short		regs[4];
	hfWinVar *			var;

	void				SetToRegs( float *registers );
	void				GetFromRegs( float *registers );
	void				CopyRegs( hfRegister *src );
	void				Enable( bool b ) { enabled = b; }
	void				ReadFromDemoFile( hfDemoFile *f );
	void				WriteToDemoFile( hfDemoFile *f );
	void				WriteToSaveGame( hfFile *savefile );
	void				ReadFromSaveGame( hfFile *savefile );
};

HF_INLINE hfRegister::hfRegister() {
}

HF_INLINE hfRegister::hfRegister( const char *p, int t ) {
	name = p;
	type = t;
	assert( t >= 0 && t < NUMTYPES );
	regCount = REGCOUNT[t];
	enabled = ( type == STRING ) ? false : true;
	var = NULL;
};

HF_INLINE void hfRegister::CopyRegs( hfRegister *src ) {
	regs[0] = src->regs[0];
	regs[1] = src->regs[1];
	regs[2] = src->regs[2];
	regs[3] = src->regs[3];
}

class hfRegisterList {
public:

						hfRegisterList();
						~hfRegisterList();

	void				AddReg( const char *name, int type, hfTokenParser *src, hfWindow *win, hfWinVar *var );
	void				AddReg( const char *name, int type, hfVec4 data, hfWindow *win, hfWinVar *var );

	hfRegister *		FindReg( const char *name );
	void				SetToRegs( float *registers );
	void				GetFromRegs( float *registers );
	void				Reset();
	void				ReadFromDemoFile( hfDemoFile *f );
	void				WriteToDemoFile( hfDemoFile *f );
	void				WriteToSaveGame( hfFile *savefile );
	void				ReadFromSaveGame( hfFile *savefile );

private:
	hfList<hfRegister*>	regs;
	hfHashIndex			regHash;
};

HF_INLINE hfRegisterList::hfRegisterList() {
	regs.SetGranularity( 4 );
	regHash.SetGranularity( 4 );
	regHash.Clear( 32, 4 );
}

HF_INLINE hfRegisterList::~hfRegisterList() {
}

#endif /* !__REGEXP_H__ */

