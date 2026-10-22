#ifndef __SWF_SPRITES_H__
#define __SWF_SPRITES_H__

/*
================================================
What the swf file format calls a "sprite" is known as a "movie clip" in Flash
There is one main sprite, and many many sub-sprites
Only the main sprite is allowed to add things to the dictionary
================================================
*/
class hfSWFSprite {
public:
	hfSWFSprite( class hfSWF * swf );
	~hfSWFSprite();

	void	Load( hfSWFBitStream & bitstream, bool parseDictionary );

	void	Read( hfFile * f );
	void	Write( hfFile * f );
	
	class hfSWF * GetSWF() { return swf; }

private:
	friend class hfSWFSpriteInstance;
	friend class hfSWFScriptFunction_Script;

	class hfSWF * swf;	// this is required so things can access the dictionary, it would be kind of nice if we just had an hfSWFDictionary pointer instead

	uint16	frameCount;

	// frameOffsets contains offsets into the commands list for each frame
	// the first command for frame 3 is frameOffsets[2] and the last command is frameOffsets[3]
	hfList< uint32, TAG_SWF >	frameOffsets;

	struct swfFrameLabel_t {
		hfStr frameLabel;
		uint32 frameNum;
	};
	hfList< swfFrameLabel_t, TAG_SWF > frameLabels;

	struct swfSpriteCommand_t {
		swfTag_t		tag;
		hfSWFBitStream	stream;
	};
	hfList< swfSpriteCommand_t, TAG_SWF > commands;

	//// [ES-BrianBugh 1/16/10] - There can be multiple DoInitAction tags, and all need to be executed.
	hfList<hfSWFBitStream, TAG_SWF> doInitActions;

	byte * commandBuffer;

};

#endif // !__SWF_SPRITES_H__

