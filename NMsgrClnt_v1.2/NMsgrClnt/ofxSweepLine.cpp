//
//  ofxSweepLine.cpp
//  an implementation of the Bentley–Ottmann line 
//  intersection algorithm for openFrameworks
//
//  by Alex Wolfe @ the Studio for Creative Inquiry

#include "stdafx.h"
#include <iostream>
#include <math.h>
#include "ofxSweepLine.h"

LineSeg::LineSeg(){}
LineSeg::LineSeg(int i, int e, ofVec2f l, ofVec2f r){
    pIndex = i;
    edge = e;
    left = l;
    right = r;
    yCoord = getYCoord(l.x);
}

float LineSeg::getYCoord(int time){
    float slope = (left.y-right.y)/(left.x-right.x);
    yCoord = slope*(time-left.x)+left.y;
    return yCoord;
}
/*---------------------------------------------*/
EventPoint::EventPoint(){}

EventPoint::EventPoint(Type t, ofVec2f p, LineSeg i1, LineSeg i2, int pI, int lI, int e){
    type = t;
    point = p;
    intersection.first = i1;
    intersection.second = i2;
    pIndex = pI;
    lIndex = lI;
    edge = e;
}

bool EventPoint::pointSorter(ofVec2f a, ofVec2f b){
    if(a.x > b.x) return true;
    if(a.x < b.x) return false;
    if(a.y > b.y) return true;
    if(a.y < b.y) return false;
}

bool EventPoint::eventPointSorter(EventPoint a, EventPoint b){
    ofVec2f pA = a.point;
    ofVec2f pB = b.point;
    return(pointSorter(pA, pB));
}
/*
void EventPoint::printPoint(){
    cout << this->type << " " << this->point<< " " << this->pIndex<< " " << this->edge <<endl;
}*/
/*---------------------------------------------*/

EventQueue::EventQueue(){}

EventQueue::EventQueue(vector<ofPolyline> p, vector<SegmentInfo> segmentInfos){
    //pull out all the endpoints of the line seqments
    //and put into the queue.
    
    //the problem when dealing with polylines, is that
    //the aren't intrinsically straight, usually they're
    //not. treat each segment of the polyline as a seperate
    //line.
	set<EventPoint, EventPointCompare>::iterator itr;
	SegmentInfo segmentInfo;

	int size;
    for(int i=0; i<p.size(); i++){
        ofPolyline line = p[i];
		segmentInfo	= segmentInfos[i];

        for(int j=0; j<line.getVertices().size()-1; j++){
            ofVec2f l = p[i][j];
            ofVec2f r = p[i][j+1];
            if(!(l.x < r.x)){
                ofVec2f tmp = l;
                l = r;
                r = tmp;
            }
            EventPoint lPoint, rPoint;
            LineSeg newLine;
            
            lPoint.type = EventPoint::LEFT;
            lPoint.pIndex = i;
            lPoint.edge = j;
            lPoint.point = l;
            
            rPoint.type = EventPoint::RIGHT;
            rPoint.pIndex = i;
            rPoint.edge = j;
            rPoint.point = r;
            
            newLine.pIndex = i;
            newLine.edge = j;
            newLine.left = l;
            newLine.right = r;
			newLine.jointIdx1 = segmentInfo.jointIdx1;
			newLine.jointIdx2 = segmentInfo.jointIdx2;
            newLine.yCoord = newLine.getYCoord(lPoint.point.x);
            newLine.lIndex = lines.size();
            lines.push_back(newLine);
            
            
            rPoint.lIndex = lines.size()-1;
            lPoint.lIndex = lines.size()-1;
            
            pointSet.insert(lPoint);
            pointSet.insert(rPoint);

			size = pointSet.size();

			itr = pointSet.begin();
			for (int i = 0; i < size; i++) {
				itr++;
			}
        }
    }
}

void EventQueue::SetFirstLineSegsYcoord(int idx, int lIndex, float x, float y, float yVal){
	int i;
	set<EventPoint, EventPointCompare>::iterator itr;
	int size = pointSet.size();
	itr = pointSet.begin();

	for(i = 0; i < size; i++) {	
//		if (itr->point.x == x && itr->point.y == y ) {
		if (itr->intersection.first.lIndex == lIndex)
			const_cast<EventPoint&>(*itr).intersection.first.yCoord = yVal;
		if (itr->intersection.second.lIndex == lIndex)
			const_cast<EventPoint&>(*itr).intersection.second.yCoord = yVal;
//		}
		itr++;
	}
}

/*
void EventQueue::sanityTest(){
    set<EventPoint>::iterator it;
    cout<< "ordered point set" << endl;
    for ( it=pointSet.begin() ; it != pointSet.end(); it++ ){
        cout << it->type << " " << it->point<< " " << it->pIndex<< " " << it->edge <<endl;
    }
}*/
/*---------------------------------------------*/


ofxSweepLine::ofxSweepLine(vector<ofPolyline> p){
//    eq = *new EventQueue(p);
}

ofxSweepLine::ofxSweepLine(){}

/*
void ofxSweepLine::test(){
    set<LineSeg, lineSegCompare>::iterator itr;
    cout<<"printing tree"<<endl;
    for (itr=sweptLines.begin(); itr!=sweptLines.end(); itr++)
        cout << (*itr).pIndex << " " << (*itr).edge << " " <<(*itr).yCoord << endl;
}*/

bool ofxSweepLine::checkExistingInterPoint(EventQueue eq, EventPoint &newPoint) 
{
	int i, size;
	EventPoint ep;

	size = eq.interPointSet.size();

	for(i = 0; i < size; i++)
	{
		ep = *eq.interPointSet.begin();
		if (ep.point.x == newPoint.point.x && ep.point.y == newPoint.point.y)
			return true;

		eq.interPointSet.erase(eq.interPointSet.begin());
	}
	return false;
}

bool ofxSweepLine::sweep(vector<ofPolyline> p, vector<SegmentInfo> segmentInfos, vector<IntersectInfo>& intersectInfos, vector<ofVec2f>& ret){
    eq = *new EventQueue(p, segmentInfos); //based on b-tree, auto sorted    
    //  sweptLines = *new std::set<LineSeg, lineSegCompare>; 
    
	int count = 0;
    pair<set<LineSeg, lineSegCompare>::iterator,bool> inserted;
    set<LineSeg, lineSegCompare>::iterator itr, tItr;
	set<LineSeg, lineSegCompare>::iterator itr2;
    //vector<ofVec2f> ret;
	bool insertFlag = false;
	int size;
    LineSeg* above = new LineSeg;
    LineSeg* below = new LineSeg;
	bool delFlag;

//	std::set<LineSeg, lineSegCompare> sweptLines;
//	set<EventPoint, EventPointCompare>::iterator itr;
    
    while(!eq.pointSet.empty()){   //While (x is nonempty) {
        EventPoint e = *eq.pointSet.begin(); //get next Event
        ofVec2f i;
		count++;

		insertFlag = false;
        if(e.type == EventPoint::LEFT){
            int above, below;
            LineSeg curLine = eq.lines[e.lIndex];
			delFlag = false;
            //DEBUG:: check to see if inserted.second is true;
            inserted = sweptLines.insert(curLine);
            itr = inserted.first;
//          test();
			tItr = itr;
            if(itr != sweptLines.begin()){
				tItr--;
                above = tItr->lIndex;
            }else 
                above = -1;

			itr2 = sweptLines.end();
			itr2--;
			tItr = itr;
			if(itr != itr2){
				tItr++;
                below = tItr->lIndex;
			}else
                below = -1;
            
            if( above>=0 && testIntersect(curLine.left, curLine.right, eq.lines[above].left, eq.lines[above].right, i)){
                EventPoint newPoint(EventPoint::INTERSECTION, i, (LineSeg)curLine, 
                                    eq.lines[above], -1,-1,-1);

				insertFlag = true;
				eq.pointSet.erase(eq.pointSet.begin());
				delFlag = true;

				if (!checkExistingInterPoint(eq, newPoint)) {
					eq.pointSet.insert(newPoint);
					eq.interPointSet.insert(newPoint);
				}
                
            }
            if(below>=0 && testIntersect(curLine.left, curLine.right, eq.lines[below].left, eq.lines[below].right, i)){
                EventPoint newPoint(EventPoint::INTERSECTION, i, (LineSeg)curLine, 
                                    eq.lines[below], -1,-1,-1);

				insertFlag = true;
				if(!delFlag)
					eq.pointSet.erase(eq.pointSet.begin());

				if (!checkExistingInterPoint(eq, newPoint)) {
					eq.pointSet.insert(newPoint);
					eq.interPointSet.insert(newPoint);
				}
            }
        }
        else if(e.type == EventPoint::RIGHT){
            //set<LineSeg, lineSegCompare>::iterator itr;
            int above, below;
			LineSeg lineSeg = eq.lines[e.lIndex];
            itr = sweptLines.find(lineSeg);
			if (itr == sweptLines.end()) {
				eq.pointSet.clear();
				eq.interPointSet.clear();
				eq.lines.clear();
				return false;	
			}

			tItr = itr;
            //find the lines directly above and below this one in the list
            //if they exist, check for an intersection
			itr2 = sweptLines.end();
			itr2--;

//			int aaa = eq.lines.size();
//			if (aaa == 2)
//				int aa = 2;

			if(itr != sweptLines.begin() && itr != itr2){
                tItr--;
				above = tItr->lIndex;
				tItr = itr;
				tItr++;
                below = tItr->lIndex;
                if(testIntersect( eq.lines[above].left, eq.lines[above].right, eq.lines[below].left, eq.lines[below].right, i)){
                    EventPoint newPoint(EventPoint::INTERSECTION, i, eq.lines[above], 
                                        eq.lines[below], -1,-1,-1);

					insertFlag = true;
					eq.pointSet.erase(eq.pointSet.begin());

					if (!checkExistingInterPoint(eq, newPoint)) {
						eq.pointSet.insert(newPoint);
						eq.interPointSet.insert(newPoint);
					}
                }
            }

            sweptLines.erase(itr);
        }
        else{ //intersection
            int intersect1, intersect2;
			delFlag = false;
			IntersectInfo intersectInfo;
            pair<set<LineSeg>::iterator,bool> check;
            
            ret.push_back(e.point); //store the intersection in the output vector
			intersectInfo.occludingLindIdx = e.intersection.first.lIndex;
			intersectInfo.occludedLindIdx = e.intersection.second.lIndex;
			intersectInfo.occludingJointsIdx[0] = e.intersection.first.jointIdx1;
			intersectInfo.occludingJointsIdx[1] = e.intersection.first.jointIdx2;
			intersectInfo.occludedJointsIdx[0] = e.intersection.second.jointIdx1;
			intersectInfo.occludedJointsIdx[1] = e.intersection.second.jointIdx2;
			intersectInfos.push_back(intersectInfo);

            //remove intersecting lines from the set, update their y coord and reinsert
            //to maintain proper sorting
            
            
            //gah, might want to redo all of this so its just a pointer to the 
            //line seg stored in the line vector rather than the lines themselves
            //so you don't have to update twice
//            test();
            
            itr = sweptLines.find(e.intersection.first);
			if (itr == sweptLines.end()) {
				eq.pointSet.clear();
				eq.interPointSet.clear();
				eq.lines.clear();
				return false;	
			}
            intersect1 = itr->lIndex;
            eq.lines[intersect1].getYCoord(e.point.x+1);			
//			e.intersection.first.yCoord = eq.lines[intersect1].yCoord;
            sweptLines.erase(itr);
            itr = sweptLines.insert(eq.lines[intersect1]).first;
			eq.SetFirstLineSegsYcoord(1, itr->lIndex, e.point.x, e.point.y, itr->yCoord);

            itr2 = sweptLines.find(e.intersection.second);
			if (itr2 == sweptLines.end()) {
				eq.pointSet.clear();
				eq.interPointSet.clear();
				eq.lines.clear();
				return false;	
			}
            intersect2 = itr2->lIndex;
            eq.lines[intersect2].getYCoord(e.point.x+1);
//			e.intersection.second.yCoord = eq.lines[intersect2].yCoord;
            sweptLines.erase(itr2);
            check = sweptLines.insert(eq.lines[intersect2]);
			eq.SetFirstLineSegsYcoord(2, check.first->lIndex, e.point.x, e.point.y, check.first->yCoord);


            //err keep thinking about this, resinsertion should replace
            //the two lines in the proper place
            //-----------
//            test();
            
			itr2 = sweptLines.end();
			itr2--;
			tItr = itr;
            if(tItr != itr2){
				tItr++;
                *below = *tItr;

//				below->yCoord = below->getYCoord(e.point.x+1);

                if(testIntersect(eq.lines[intersect1].right, eq.lines[intersect1].left, below->right, below->left, i)){
                    EventPoint newPoint(EventPoint::INTERSECTION, i, eq.lines[intersect1], 
                                        (LineSeg)*tItr, -1,-1,-1);

					insertFlag = true;
					eq.pointSet.erase(eq.pointSet.begin());
					delFlag = true;

					if (!checkExistingInterPoint(eq, newPoint)) {
						eq.pointSet.insert(newPoint);
						eq.interPointSet.insert(newPoint);
					}
                }
            }
//            test();
            itr = check.first;
//            test();
			tItr = itr;
            if(itr != sweptLines.begin()){
				tItr--;
                *above = *tItr;
//                above->yCoord = above->getYCoord(e.point.x+1);

                if(testIntersect(eq.lines[intersect2].right, eq.lines[intersect2].left, above->right, above->left, i)){
                    EventPoint newPoint(EventPoint::INTERSECTION, i, eq.lines[intersect2], 
                                        (LineSeg)*tItr, -1,-1,-1);
                   
					insertFlag = true;
					if(!delFlag)
						eq.pointSet.erase(eq.pointSet.begin());

					if (!checkExistingInterPoint(eq, newPoint)) {
						eq.pointSet.insert(newPoint);
						eq.interPointSet.insert(newPoint);
					}
                }
            }
        }
        if (!insertFlag)
			eq.pointSet.erase(eq.pointSet.begin());
    }

	eq.pointSet.clear();
	eq.interPointSet.clear();
	eq.lines.clear();
	delete above;
	delete below;

    return true;
}

bool ofxSweepLine::testIntersect(ofVec2f a1, ofVec2f a2, ofVec2f b1, ofVec2f b2, ofVec2f &i){
    
    ofVec2f a = a2-a1;
    ofVec2f b = b2-b1;
    ofVec2f c = b2-a2;
    
	if (abs(a1.x - b1.x) < 1 && abs(a1.y - b1.y) < 1) return false;
	if (abs(a2.x - b1.x) < 1 && abs(a2.y - b1.y) < 1) return false;
	if (abs(a1.x - b2.x) < 1 && abs(a1.y - b2.y) < 1) return false;
	if (abs(a2.x - b2.x) < 1 && abs(a2.y - b2.y) < 1) return false;


    double f = perpDot(a,b);
    if(!f)
        return false; //lines are parallel
    
    double aa = perpDot(a,c);
    double bb = perpDot(b,c);
    
    if(f<0){
        if( aa>0 || bb>0 || aa<f || bb<f)
            return false;
    }
    else{
        if(aa<0 || bb<0 || aa>f || bb>f)
            return false;
    }
    double mu = 1.0-(aa/f);
    ofVec2f intersection = ((b2-b1)*mu) + b1;
/*
	if ( abs(intersection.x - a1.x) < 1 && abs(intersection.y - a1.y) < 1) return false;
	if ( abs(intersection.x - a2.x) < 1 && abs(intersection.y - a2.y) < 1) return false;
	if ( abs(intersection.x - b1.x) < 1 && abs(intersection.y - b1.y) < 1) return false;
	if ( abs(intersection.x - b2.x) < 1 && abs(intersection.y - b2.y) < 1) return false;
*/
    i.set(intersection.x, intersection.y);
    if(i!= a1 && i!= a2 && i != b1 && i !=b2)
        return true;
    else {
        return false;
    }
}

double ofxSweepLine::perpDot(ofVec2f a , ofVec2f b){
    return (a.y*b.x)-(a.x*b.y);
}



