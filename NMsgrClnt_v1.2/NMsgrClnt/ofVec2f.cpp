#include "stdafx.h"
#include "ofVec2f.h"


ofVec2f::ofVec2f( const ofVec3f& vec ) {
	x = vec.x;
	y = vec.y;
}

ofVec2f::ofVec2f( const ofVec4f& vec ) {
	x = vec.x;
	y = vec.y;
}
