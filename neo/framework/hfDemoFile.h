#ifndef __DEMOFILE_H__
#define __DEMOFILE_H__

/*
===============================================================================

	Demo file

===============================================================================
*/

typedef enum {
	DS_FINISHED,
	DS_RENDER,
	DS_SOUND,
	DS_VERSION
} demoSystem_t;

class hfDemoFile : public hfFile {
public:
					hfDemoFile();
					~hfDemoFile();

	const char *	GetName() { return (f?f->GetName():""); }
	const char *	GetFullPath() { return (f?f->GetFullPath():""); }

	void			SetLog( bool b, const char *p );
	void			Log( const char *p );
	bool			OpenForReading( const char *fileName );
	bool			OpenForWriting( const char *fileName );
	void			Close();

	const char *	ReadHashString();
	void			WriteHashString( const char *str );

	void			ReadDict( hfDict &dict );
	void			WriteDict( const hfDict &dict );

	int				Read( void *buffer, int len );
	int				Write( const void *buffer, int len );

private:
	static hfCompressor *AllocCompressor( int type );

	bool			writing;
	byte *			fileImage;
	hfFile *		f;
	hfCompressor *	compressor;

	hfList<hfStr*>	demoStrings;
	hfFile *		fLog;
	bool			log;
	hfStr			logStr;

	static hfCVar	com_logDemos;
	static hfCVar	com_compressDemos;
	static hfCVar	com_preloadDemos;
};

#endif /* !__DEMOFILE_H__ */

