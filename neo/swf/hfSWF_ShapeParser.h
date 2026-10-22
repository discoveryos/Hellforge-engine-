#ifndef __SWF_SHAPEPARSER_H__
#define __SWF_SHAPEPARSER_H__

/*
================================================
This class handles parsing and triangulating a shape
================================================
*/
class hfSWFShapeParser {
public:
	hfSWFShapeParser() { }
	void Parse( hfSWFBitStream & bitstream, hfSWFShape & shape, int recordType );
	void ParseMorph( hfSWFBitStream & bitstream, hfSWFShape & shape );
	void ParseFont( hfSWFBitStream & bitstream, hfSWFFontGlyph & shape );

private:
	bool extendedCount;
	bool rgba;
	bool morph;
	bool lineStyle2;

	struct swfSPEdge_t {
		uint16 v0;
		uint16 v1;
		uint16 cp;	// control point if this is a curve, 0xFFFF otherwise
	};
	struct swfSPMorphEdge_t {
		swfSPEdge_t start;
		swfSPEdge_t end;
	};
	struct swfSPLineLoop_t {
		bool hole;
		hfList< uint16, TAG_SWF > vindex1;
		hfList< uint16, TAG_SWF > vindex2;
	};
	struct swfSPDrawFill_t {
		swfFillStyle_t style;
		hfList< swfSPMorphEdge_t, TAG_SWF > edges;
		hfList< swfSPLineLoop_t, TAG_SWF > loops;
	};
	struct swfSPDrawLine_t {
		swfLineStyle_t style;
		hfList< swfSPMorphEdge_t, TAG_SWF > edges;
	};
	hfList< hfVec2, TAG_SWF > verts;
	hfList< swfSPDrawFill_t, TAG_SWF > fillDraws;
	hfList< swfSPDrawLine_t, TAG_SWF > lineDraws;


private:
	void ParseShapes( hfSWFBitStream & bitstream1, hfSWFBitStream * bitstream2, bool swap );
	void ReadFillStyle( hfSWFBitStream & bitstream );
	void ParseEdge( hfSWFBitStream & bitstream, int32 & penX, int32 & penY, swfSPEdge_t & edge );
	void MakeLoops();
	void TriangulateSoup( hfSWFShape & shape );
	void TriangulateSoup( hfSWFFontGlyph & shape );
	int FindEarVert( const swfSPLineLoop_t & loop );
	void AddUniqueVert( hfSWFShapeDrawFill & drawFill, const hfVec2 & start, const hfVec2 & end );

};

#endif // !__SWF_SHAPEPARSER_H__

