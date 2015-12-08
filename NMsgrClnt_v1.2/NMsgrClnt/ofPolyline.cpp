#include "stdafx.h"
#include "ofPolyline.h"
//#include "ofGraphics.h"

//----------------------------------------------------------
ofPolyline::ofPolyline(){
//    setRightVector();
	clear();
}

//----------------------------------------------------------
ofPolyline::ofPolyline(const vector<ofPoint>& verts){
//    setRightVector();
	clear();
	addVertices(verts);
}

//----------------------------------------------------------
void ofPolyline::clear() {
	setClosed(false);
	points.clear();
	curveVertices.clear();
    flagHasChanged();
}

//----------------------------------------------------------
void ofPolyline::addVertex(const ofPoint& p) {
	curveVertices.clear();
	points.push_back(p);
    flagHasChanged();
}

//----------------------------------------------------------
void ofPolyline::addVertex(float x, float y, float z) {
	curveVertices.clear();
//	addVertex(ofPoint(x,y,z));
    flagHasChanged();
}

//----------------------------------------------------------
void ofPolyline::addVertices(const vector<ofPoint>& verts) {
	curveVertices.clear();
	points.insert( points.end(), verts.begin(), verts.end() );
    flagHasChanged();
}

//----------------------------------------------------------
void ofPolyline::addVertices(const ofPoint* verts, int numverts) {
	curveVertices.clear();
	points.insert( points.end(), verts, verts + numverts );
    flagHasChanged();
}

//----------------------------------------------------------
void ofPolyline::insertVertex(const ofPoint &p, int index) {
    curveVertices.clear();
    points.insert(points.begin()+index, p);
    flagHasChanged();
}

//----------------------------------------------------------
void ofPolyline::insertVertex(float x, float y, float z, int index) {
//    insertVertex(ofPoint(x, y, z), index);
}


//----------------------------------------------------------
size_t ofPolyline::size() const {
	return points.size();
}

//----------------------------------------------------------
const ofPoint& ofPolyline::operator[] (int index) const {
	return points[index];
}

//----------------------------------------------------------
ofPoint& ofPolyline::operator[] (int index) {
    flagHasChanged();
	return points[index];
}

//----------------------------------------------------------
void ofPolyline::resize(size_t size){
	points.resize(size);
    flagHasChanged();
}

//----------------------------------------------------------
void ofPolyline::setClosed( bool tf ) {
	bClosed = tf;
    flagHasChanged();
}

//----------------------------------------------------------
bool ofPolyline::isClosed() const {
	return bClosed;
}

//----------------------------------------------------------
void ofPolyline::close(){
    setClosed(true);
}

//----------------------------------------------------------
bool ofPolyline::hasChanged(){
    if(bHasChanged){
        bHasChanged=false;
        return true;
    }else{
        return false;
    }
}

//----------------------------------------------------------
void ofPolyline::flagHasChanged() {
    bHasChanged = true;
    bCacheIsDirty = true;
}

//----------------------------------------------------------
vector<ofPoint> & ofPolyline::getVertices(){
    flagHasChanged();
	return points;
}

//----------------------------------------------------------
const vector<ofPoint> & ofPolyline::getVertices() const {
	return points;
}

//This is for polygon/contour simplification - we use it to reduce the number of points needed in
//representing the letters as openGL shapes - will soon be moved to ofGraphics.cpp

// From: http://softsurfer.com/Archive/algorithm_0205/algorithm_0205.htm
// Copyright 2002, softSurfer (www.softsurfer.com)
// This code may be freely used and modified for any purpose
// providing that this copyright notice is included with it.
// SoftSurfer makes no warranty for this code, and cannot be held
// liable for any real or imagined damage resulting from its use.
// Users of this code must verify correctness for their application.

typedef struct{
	ofPoint P0;
	ofPoint P1;
}Segment;

// dot product (3D) which allows vector operations in arguments
#define dot(u,v)   ((u).x * (v).x + (u).y * (v).y + (u).z * (v).z)
#define norm2(v)   dot(v,v)        // norm2 = squared length of vector
#define norm(v)    sqrt(norm2(v))  // norm = length of vector
#define d2(u,v)    norm2(u-v)      // distance squared = norm2 of difference
#define d(u,v)     norm(u-v)       // distance = norm of difference

