#pragma once

using namespace cv;

//스켈레톤 그리는 부분의 파라미터
static const float c_JointThickness = 3.0f;
static const float c_TrackedBoneThickness = 6.0f;
static const float c_InferredBoneThickness = 1.0f;
static const float c_HandSize = 30.f;

class DrawSkeletons
{
public:
	DrawSkeletons(void);
	~DrawSkeletons(void);

	void DrawSkelToMat(Mat *src, Point2d *JointPoints, Joint* pJoints, int t_id);
	void DrawSkelBone(Mat *src, Joint* pJoints, Point2d* pJointPoints, JointType joint0, JointType joint1, Scalar t_Color);

	//Draw Hand state. - but not implemented.
	void DrawHand(Mat *src, HandState handState, Point2d& handposition);

};

