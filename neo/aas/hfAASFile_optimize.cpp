#pragma hdrstop
#include "../hflib/hfprecompiled.h"


#include "hfAASFile.h"
#include "hfAASFile_local.h"


//===============================================================
//
//	optimize file
//
//===============================================================

/*
================
hfAASFileLocal::Optimize
================
*/
void hfAASFileLocal::Optimize() {
	int i, j, k, faceNum, edgeNum, areaFirstFace, faceFirstEdge;
	aasArea_t *area;
	aasFace_t *face;
	aasEdge_t *edge;
	hfReachability *reach;
	hfList<int> vertexRemap;
	hfList<int> edgeRemap;
	hfList<int> faceRemap;
	hfList<aasVertex_t> newVertices;
	hfList<aasEdge_t> newEdges;
	hfList<aasIndex_t> newEdgeIndex;
	hfList<aasFace_t> newFaces;
	hfList<aasIndex_t> newFaceIndex;

	vertexRemap.AssureSize( vertices.Num(), -1 );
	edgeRemap.AssureSize( edges.Num(), 0 );
	faceRemap.AssureSize( faces.Num(), 0 );

	newVertices.Resize( vertices.Num() );
	newEdges.Resize( edges.Num() );
	newEdges.SetNum( 1 );
	newEdgeIndex.Resize( edgeIndex.Num() );
	newFaces.Resize( faces.Num() );
	newFaces.SetNum( 1 );
	newFaceIndex.Resize( faceIndex.Num() );

	for ( i = 0; i < areas.Num(); i++ ) {
		area = &areas[i];

		areaFirstFace = newFaceIndex.Num();
		for ( j = 0; j < area->numFaces; j++ ) {
			faceNum = faceIndex[area->firstFace + j];
			face = &faces[ abs(faceNum) ];

			// store face
			if ( !faceRemap[ abs(faceNum) ] ) {
				faceRemap[ abs(faceNum) ] = newFaces.Num();
				newFaces.Append( *face );

				// don't store edges for faces we don't care about
				if ( !( face->flags & ( FACE_FLOOR|FACE_LADDER ) ) ) {

					newFaces[ newFaces.Num()-1 ].firstEdge = 0;
					newFaces[ newFaces.Num()-1 ].numEdges = 0;

				} else {

					// store edges
					faceFirstEdge = newEdgeIndex.Num();
					for ( k = 0; k < face->numEdges; k++ ) {
						edgeNum = edgeIndex[ face->firstEdge + k ];
						edge = &edges[ abs(edgeNum) ];

						if ( !edgeRemap[ abs(edgeNum) ] ) {
							if ( edgeNum < 0 ) {
								edgeRemap[ abs(edgeNum) ] = -newEdges.Num();
							}
							else {
								edgeRemap[ abs(edgeNum) ] = newEdges.Num();
							}

							// remap vertices if not yet remapped
							if ( vertexRemap[ edge->vertexNum[0] ] == -1 ) {
								vertexRemap[ edge->vertexNum[0] ] = newVertices.Num();
								newVertices.Append( vertices[ edge->vertexNum[0] ] );
							}
							if ( vertexRemap[ edge->vertexNum[1] ] == -1 ) {
								vertexRemap[ edge->vertexNum[1] ] = newVertices.Num();
								newVertices.Append( vertices[ edge->vertexNum[1] ] );
							}

							newEdges.Append( *edge );
							newEdges[ newEdges.Num()-1 ].vertexNum[0] = vertexRemap[ edge->vertexNum[0] ];
							newEdges[ newEdges.Num()-1 ].vertexNum[1] = vertexRemap[ edge->vertexNum[1] ];
						}

						newEdgeIndex.Append( edgeRemap[ abs(edgeNum) ] );
					}

					newFaces[ newFaces.Num()-1 ].firstEdge = faceFirstEdge;
					newFaces[ newFaces.Num()-1 ].numEdges = newEdgeIndex.Num() - faceFirstEdge;
				}
			}

			if ( faceNum < 0 ) {
				newFaceIndex.Append( -faceRemap[ abs(faceNum) ] );
			} else {
				newFaceIndex.Append( faceRemap[ abs(faceNum) ] );
			}
		}

		area->firstFace = areaFirstFace;
		area->numFaces = newFaceIndex.Num() - areaFirstFace;

		// remap the reachability edges
		for ( reach = area->reach; reach; reach = reach->next ) {
			reach->edgeNum = abs( edgeRemap[reach->edgeNum] );
		}
	}

	// store new list
	vertices = newVertices;
	edges = newEdges;
	edgeIndex = newEdgeIndex;
	faces = newFaces;
	faceIndex = newFaceIndex;
}

