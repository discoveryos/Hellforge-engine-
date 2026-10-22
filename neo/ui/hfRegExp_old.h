#ifndef REGEXP_H_
#define REGEXP_H_

class hfWindow;

class hfRegister {
public:
	hfRegister() {};
	hfRegister(const char *p, int t) {
		name = p;
		type = t;
		assert(t >= 0 && t < NUMTYPES);
		regCount = REGCOUNT[t];
		enabled = (type == STRING) ? false : true;
	};
	bool enabled;
	int type;
	int regCount;
	enum REGTYPE { VEC4 = 0, FLOAT, BOOL, INT, STRING, VEC2, VEC3, NUMTYPES } ;
	static int REGCOUNT[NUMTYPES];
	hfStr name;
	int regs[4];
	void SetToRegs(float *registers, hfTypedDict *state);
	void SetToRegList(hfList<float> *registers, hfTypedDict *state);
	void GetFromRegs(float *registers, hfTypedDict *state);
	void CopyRegs(hfRegister *src) {
		regs[0] = src->regs[0];
		regs[1] = src->regs[1];
		regs[2] = src->regs[2];
		regs[3] = src->regs[3];
	}
	void Enable(bool b) {
		enabled = b;
	}
	void ReadFromDemoFile(hfDemoFile *f);
	void WriteToDemoFile(hfDemoFile *f);

};

class hfRegisterList {
	hfList<hfRegister> regs;
public:
	
	// 
	void RemoveReg ( const char* name );
	// 

	void AddReg(const char *name, int type, hfTokenParser *src, hfWindow *win);
	void AddReg(const char *name, int type, hfVec4 data, hfWindow *win);
	hfRegister *FindReg(const char *name);
	int			FindRegIndex ( const char* name );
	void SetToRegs(float *registers, hfTypedDict *state);
	void GetFromRegs(float *registers, hfTypedDict *state);
	void Reset();
	void ReadFromDemoFile(hfDemoFile *f);
	void WriteToDemoFile(hfDemoFile *f);

};

#endif
