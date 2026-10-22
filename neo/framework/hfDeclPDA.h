#ifndef __DECLPDA_H__
#define __DECLPDA_H__

/*
===============================================================================

	hfDeclPDA

===============================================================================
*/


class hfDeclEmail : public hfDecl {
public:
							hfDeclEmail() {}

	virtual size_t			Size() const;
	virtual const char *	DefaultDefinition() const;
	virtual bool			Parse( const char *text, const int textLength, bool allowBinaryVersion );
	virtual void			FreeData();
	virtual void			Print() const;
	virtual void			List() const;

	const char *			GetFrom() const { return from; }
	const char *			GetBody() const { return text; }
	const char *			GetSubject() const { return subject; }
	const char *			GetDate() const { return date; }
	const char *			GetTo() const { return to; }

private:
	hfStr					text;
	hfStr					subject;
	hfStr					date;
	hfStr					to;
	hfStr					from;
};


class hfDeclVideo : public hfDecl {
public:
							hfDeclVideo() : preview( NULL ), video( NULL ), audio( NULL ) {};

	virtual size_t			Size() const;
	virtual const char *	DefaultDefinition() const;
	virtual bool			Parse( const char *text, const int textLength, bool allowBinaryVersion );
	virtual void			FreeData();
	virtual void			Print() const;
	virtual void			List() const;

	const hfMaterial *		GetRoq() const { return video; }
	const hfSoundShader *	GetWave() const { return audio; }
	const char *			GetVideoName() const { return videoName; }
	const char *			GetInfo() const { return info; }
	const hfMaterial *		GetPreview() const { return preview; }

private:
	const hfMaterial *		preview;
	const hfMaterial *		video;
	hfStr					videoName;
	hfStr					info;
	const hfSoundShader *	audio;
};


class hfDeclAudio : public hfDecl {
public:
							hfDeclAudio() : audio( NULL ) {};

	virtual size_t			Size() const;
	virtual const char *	DefaultDefinition() const;
	virtual bool			Parse( const char *text, const int textLength, bool allowBinaryVersion );
	virtual void			FreeData();
	virtual void			Print() const;
	virtual void			List() const;

	const char *			GetAudioName() const { return audioName; }
	const hfSoundShader *	GetWave() const { return audio; }
	const char *			GetInfo() const { return info; }

private:
	const hfSoundShader *	audio;
	hfStr					audioName;
	hfStr					info;
};

class hfDeclPDA : public hfDecl {
public:
							hfDeclPDA() { originalEmails = originalVideos = 0; };

	virtual size_t			Size() const;
	virtual const char *	DefaultDefinition() const;
	virtual bool			Parse( const char *text, const int textLength, bool allowBinaryVersion );
	virtual void			FreeData();
	virtual void			Print() const;
	virtual void			List() const;

	virtual void			AddVideo( const hfDeclVideo * video, bool unique = true ) const { if ( unique ) { videos.AddUnique( video ); } else { videos.Append( video ); } }
	virtual void			AddAudio( const hfDeclAudio * audio, bool unique = true ) const { if ( unique ) { audios.AddUnique( audio ); } else { audios.Append( audio ); } }
	virtual void			AddEmail( const hfDeclEmail * email, bool unique = true ) const { if ( unique ) { emails.AddUnique( email ); } else { emails.Append( email ); } }
	virtual void			RemoveAddedEmailsAndVideos() const;

	virtual const int		GetNumVideos() const { return videos.Num(); }
	virtual const int		GetNumAudios() const { return audios.Num(); }
	virtual const int		GetNumEmails() const { return emails.Num(); }
	virtual const hfDeclVideo *GetVideoByIndex( int index ) const { return ( index < 0 || index > videos.Num() ? NULL : videos[index] ); }
	virtual const hfDeclAudio *GetAudioByIndex( int index ) const { return ( index < 0 || index > audios.Num() ? NULL : audios[index] ); }
	virtual const hfDeclEmail *GetEmailByIndex( int index ) const { return ( index < 0 || index > emails.Num() ? NULL : emails[index] ); }

	virtual void			SetSecurity( const char *sec ) const;

	const char *			GetPdaName() const { return pdaName; }
	const char *			GetSecurity() const {return security; }
	const char *			GetFullName() const { return fullName; }
	const char *			GetIcon() const { return icon; }
	const char *			GetPost() const { return post; }
	const char *			GetID() const { return id; }
	const char *			GetTitle() const { return title; }

private:
	mutable hfList<const hfDeclVideo *>	videos;
	mutable hfList<const hfDeclAudio *>	audios;
	mutable hfList<const hfDeclEmail *>	emails;
	hfStr					pdaName;
	hfStr					fullName;
	hfStr					icon;
	hfStr					id;
	hfStr					post;
	hfStr					title;
	mutable hfStr			security;
	mutable	int				originalEmails;
	mutable int				originalVideos;
};

#endif /* !__DECLPDA_H__ */

