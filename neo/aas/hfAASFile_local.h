#ifndef __AASFILELOCAL_H__
#define __AASFILELOCAL_H__

/*
===============================================================================

	AAS File Local

===============================================================================
*/

class hfAASFileLocal : public hfAASFile {
	friend class hfAASBuild;
	friend class hfAASReach;
	friend class hfAASCluster;
public:
								hfAASFileLocal();
	virtual 					~hfAASFileLocal();

public:
	virtual hfVec3				EdgeCenter( int edgeNum ) const;
	virtual hfVec3				FaceCenter( int faceNum ) const;
	virtual hfVec3				AreaCenter( int areaNum ) const;

	virtual hfBounds			EdgeBounds( int edgeNum ) const;
	virtual hfBounds			FaceBounds( int faceNum ) const;
	virtual hfBounds			AreaBounds( int areaNum ) const;

	virtual int					PointAreaNum( const hfVec3 &origin ) const;
	virtual int					PointReachableAreaNum( const hfVec3 &origin, const hfBounds &searchBounds, const int areaFlags, const int excludeTravelFlags ) const;
	virtual int					BoundsReachableAreaNum( const hfBounds &bounds, const int areaFlags, const int excludeTravelFlags ) const;
	virtual void				PushPointIntoAreaNum( int areaNum, hfVec3 &point ) const;
	virtual bool				Trace( aasTrace_t &trace, const hfVec3 &start, const hfVec3 &end ) const;
	virtual void				PrintInfo() const;

public:
	bool						Load( const hfStr &fileName, unsigned int mapFileCRC );
	bool						Write( const hfStr &fileName, unsigned int mapFileCRC );

	int							MemorySize() const;
	void						ReportRoutingEfficiency() const;
	void						Optimize();
	void						LinkReversedReachability();
	void						FinishAreas();

	void						Clear();
	void						DeleteReachabilities();
	void						DeleteClusters();

private:
	bool						ParseIndex( hfLexer &src, hfList<aasIndex_t> &indexes );
	bool						ParsePlanes( hfLexer &src );
	bool						ParseVertices( hfLexer &src );
	bool						ParseEdges( hfLexer &src );
	bool						ParseFaces( hfLexer &src );
	bool						ParseReachabilities( hfLexer &src, int areaNum );
	bool						ParseAreas( hfLexer &src );
	bool						ParseNodes( hfLexer &src );
	bool						ParsePortals( hfLexer &src );
	bool						ParseClusters( hfLexer &src );

private:
	int							BoundsReachableAreaNum_r( int nodeNum, const hfBounds &bounds, const int areaFlags, const int excludeTravelFlags ) const;
	void						MaxTreeDepth_r( int nodeNum, int &depth, int &maxDepth ) const;
	int							MaxTreeDepth() const;
	int							AreaContentsTravelFlags( int areaNum ) const;
	hfVec3						AreaReachableGoal( int areaNum ) const;
	int							NumReachabilities() const;
};

#endif /* !__AASFILELOCAL_H__ */

