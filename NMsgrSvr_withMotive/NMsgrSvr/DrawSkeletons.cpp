#include "stdafx.h"
#include "NMsgrSvrDlg.h"
#include "DrawSkeletons.h"


DrawSkeletons::DrawSkeletons(void)
{
}


DrawSkeletons::~DrawSkeletons(void)
{
}


void DrawSkeletons::DrawSkelToMat(cv::Mat *src, Point2d* JointPoints, Joint* pJoint, int t_id){
	Scalar t_Color = Scalar((t_id*37)%256, (t_id*113)%256, (t_id*71)%256); 

	// Torso
	DrawSkelBone(src, pJoint, JointPoints, JointType_Head, JointType_Neck, t_Color);
	DrawSkelBone(src, pJoint, JointPoints, JointType_Neck, JointType_SpineShoulder, t_Color);
	DrawSkelBone(src, pJoint, JointPoints, JointType_SpineShoulder, JointType_SpineMid, t_Color);
	DrawSkelBone(src, pJoint, JointPoints, JointType_SpineMid, JointType_SpineBase, t_Color);
	DrawSkelBone(src, pJoint, JointPoints, JointType_SpineShoulder, JointType_ShoulderRight, t_Color);
	DrawSkelBone(src, pJoint, JointPoints, JointType_SpineShoulder, JointType_ShoulderLeft, t_Color);
	DrawSkelBone(src, pJoint, JointPoints, JointType_SpineBase, JointType_HipRight, t_Color);
	DrawSkelBone(src, pJoint, JointPoints, JointType_SpineBase, JointType_HipLeft, t_Color);

	// Right Arm
	DrawSkelBone(src, pJoint, JointPoints, JointType_ShoulderRight, JointType_ElbowRight, t_Color);
	DrawSkelBone(src, pJoint, JointPoints, JointType_ElbowRight, JointType_WristRight, t_Color);
	DrawSkelBone(src, pJoint, JointPoints, JointType_WristRight, JointType_HandRight, t_Color);
//	DrawSkelBone(src, pJoint, JointPoints, JointType_HandRight, JointType_HandTipRight, t_Color);
//	DrawSkelBone(src, pJoint, JointPoints, JointType_WristRight, JointType_ThumbRight, t_Color);

    // Left Arm
	DrawSkelBone(src, pJoint, JointPoints, JointType_ShoulderLeft, JointType_ElbowLeft, t_Color);
	DrawSkelBone(src, pJoint, JointPoints, JointType_ElbowLeft, JointType_WristLeft, t_Color);
	DrawSkelBone(src, pJoint, JointPoints, JointType_WristLeft, JointType_HandLeft, t_Color);
//	DrawSkelBone(src, pJoint, JointPoints, JointType_HandLeft, JointType_HandTipLeft, t_Color);
//	DrawSkelBone(src, pJoint, JointPoints, JointType_WristLeft, JointType_ThumbLeft, t_Color);

    // Right Leg
	DrawSkelBone(src, pJoint, JointPoints, JointType_HipRight, JointType_KneeRight, t_Color);
	DrawSkelBone(src, pJoint, JointPoints, JointType_KneeRight, JointType_AnkleRight, t_Color);
	DrawSkelBone(src, pJoint, JointPoints, JointType_AnkleRight, JointType_FootRight, t_Color);

    // Left Leg
	DrawSkelBone(src, pJoint, JointPoints, JointType_HipLeft, JointType_KneeLeft, t_Color);
	DrawSkelBone(src, pJoint, JointPoints, JointType_KneeLeft, JointType_AnkleLeft, t_Color);
	DrawSkelBone(src, pJoint, JointPoints, JointType_AnkleLeft, JointType_FootLeft, t_Color);

	//Draw the Joints	- not implemented.
}

void DrawSkeletons::DrawSkelBone(Mat *src, Joint* pJoints, Point2d* pJointPoints, JointType joint0, JointType joint1, Scalar t_Color){
	TrackingState joint0State = pJoints[joint0].TrackingState;
    TrackingState joint1State = pJoints[joint1].TrackingState;

    // If we can't find either of these joints, exit
 /*   if ((joint0State == TrackingState_NotTracked) || (joint1State == TrackingState_NotTracked))
    {
        return;
    }

    // Don't draw if both points are inferred
    if ((joint0State == TrackingState_Inferred) && (joint1State == TrackingState_Inferred))
    {
        return;
    }

    // We assume all drawn bones are inferred unless BOTH joints are tracked
    if ((joint0State == TrackingState_Tracked) && (joint1State == TrackingState_Tracked))
    {
		line(*src, pJointPoints[joint0], pJointPoints[joint1], t_Color, c_TrackedBoneThickness);
    }
    else
    {
        line(*src, pJointPoints[joint0], pJointPoints[joint1], t_Color, c_InferredBoneThickness);
    }*/
	if ((joint0State == TrackingState_Tracked) && (joint1State == TrackingState_Tracked))   // occluded 
	{
		t_Color = Scalar(0, 0, 0);
		line(*src, pJointPoints[joint0], pJointPoints[joint1], t_Color, c_TrackedBoneThickness);
	}
	else if ((joint0State == TrackingState_Inferred) && (joint1State == TrackingState_Inferred))   // occluding
	{
		t_Color = Scalar(0, 0, 255);
		line(*src, pJointPoints[joint0], pJointPoints[joint1], t_Color, c_TrackedBoneThickness);
	
	}else 
	{
		t_Color = Scalar(0, 0, 0);
		line(*src, pJointPoints[joint0], pJointPoints[joint1], t_Color, c_TrackedBoneThickness);
	}
}

//Not Implemented. - not work in SDK 2.0 preview version.. (나중에 사용해야할듯 handState가 항상 unknown.)
void DrawSkeletons::DrawHand(Mat *src, HandState handState, Point2d& handposition){
	switch(handState){
	case HandState_Closed:
		break;
	case HandState_Open:
		break;
	case HandState_Lasso:
		break;
	}
}
