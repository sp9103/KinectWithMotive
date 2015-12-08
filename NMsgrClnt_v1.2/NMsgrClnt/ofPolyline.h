#pragma once
#include <deque>
#include "ofVec2f.h"

typedef ofVec2f ofPoint;

using namespace std;
//#include "ofRectangle.h"

// ofPolyline
// A line composed of straight line segments.

class ofRectangle;

class ofPolyline {
public:
	ofPolyline();
	ofPolyline(const vector<ofPoint>& verts);

	/// remove all the points
	void clear();

	/// add a vertex
	void addVertex( const ofPoint& p );
	void addVertex( float x, float y, float z=0 );
	void addVertices( const vector<ofPoint>& verts );
	void addVertices(const ofPoint* verts, int numverts);

	// adds a straight line to the polyline
	void lineTo(const ofPoint & to ){ addVertex(to); }
	void lineTo(float x, float y, float z=0){
		addVertex(x,y,z);
	}
    
    void insertVertex(const ofPoint &p, int index);
    void insertVertex(float x, float y, float z, int index);

	// adds an arc to the polyline
	// if the arc doesn't start at the same point
	// the last vertex finished a straight line will
	// be created to join both
    
 
	/// points vector access
	size_t size() const;
	const ofPoint& operator[] (int index) const;
	ofPoint& operator[] (int index);
	void resize(size_t size);

	/// closed
	void setClosed( bool tf );
	bool isClosed() const;
	void close();

	bool hasChanged();
    void flagHasChanged();

	vector<ofPoint> & getVertices();
	const vector<ofPoint> & getVertices() const;

    
private:

	vector<ofPoint> points;
    mutable float area;
    
    
	deque<ofPoint> curveVertices;
	vector<ofPoint> circlePoints;

	bool bClosed;
	bool bHasChanged;   // public API has access to this
    mutable bool bCacheIsDirty;   // used only internally, no public API to read
 
};

