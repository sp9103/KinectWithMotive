#pragma once

#include "CovMatReader.h"


typedef struct {
   float x,y,z;
} XYZ;

#define SMALL_NUM   0.00000001 // anything that avoids division overflow
// dot product (3D) which allows vector operations in arguments
#define dot(u,v)   ((u).x * (v).x + (u).y * (v).y + (u).z * (v).z)
#define norm(v)    sqrt(dot(v,v))  // norm = length of  vector
#define d(u,v)     norm(u-v)        // distance = norm of difference
#define abs(x)     ((x) >= 0 ? (x) : -(x))   //  absolute value


using namespace cv;
class KF
{
public:
	KF(void);
	~KF(void);

	void KFExec(synconizedSkeletons* syncSkel, double sampling_interval, int bodyIdx);
	void Initialize(CDialog* pDlg);
	void calcVelocity(double sampling_interval);
	void preiction(double sampling_interval);
	void update();
	void MeasurementFusion(synconizedSkeletons* syncSkel);
	void MeasurementFusion2(synconizedSkeletons* syncSkel);
	void CheckMeasurementsConfidence(double sampling_interval, synconizedSkeletons* syncSkels, int RefBodyIdx);
	float normal_pdf(float x, float m, TrackingState TrackingState, int jointIdx, int dim);
	void setProcessNoiseCov();
	void copyVelVecToDouble(double *vVec);
	void copyDoubleToVelVec(double *vVec);
	void GetActualJointIdx(int *index);
	bool CheckBodyConstraints1(synconizedSkeletons* syncSkels, int i);
	bool CheckBodyConstraints2(synconizedSkeletons* syncSkels, int i);
	bool CheckBodyConstraints3(synconizedSkeletons* syncSkels, int i, int refsIdx, int refeIdx, int compsIdx, int compeIdx);
	bool CheckBodyConstraints4(synconizedSkeletons* syncSkels, int i);
	bool LineLineIntersect(XYZ p1,XYZ p2,XYZ p3,XYZ p4,XYZ *dP);
	void calculateABoneLength(synconizedSkeletons* syncSkel, int refIdx, int targetBoneIdx, int fromJointIdx, int toJintIdx);
	void initializeBonesLength(synconizedSkeletons* syncSkel, int refIdx);
	void correction(synconizedSkeletons* syncSkel, int refBodyIdx);
	float calculateABoneDist(int targetBoneIdx, int fromJointIdx, int toJintIdx);

	CDialog*	m_pDlg;
	StateVector m_StateVector;
	StateVector m_prevStateVector;
	StateVector m_PredictionState;
	StateVector PredictStateVector;
	StateVector	m_lastStateVector;
	synconizedSkeletons* m_lastSyncSkels;
	synconizedSkeletons* m_backupSyncSkel;

	vector<JointHistory*> m_JointsHistory[KINECT_COUNT];
	
	CvKalman* kalman;
	CovMatReader	m_CovMatReader;

	CvMat* m_state;		// State vector
	CvMat* m_controlVec;

	CvMat* m_F;		// State transition matrix
	CvMat* m_G;		// Input transition matrix
	CvMat* m_Q;		// Process noise covariance 
	vector<velVector*> m_VelVectors;
	velVector		m_lastVelVector;

	CvMat* m_Measurement;
	CvMat* m_H;		// Measurement matrix
	CvMat* m_R;		// Measurement  noise covariance 

	JointsConfidennce	m_JointsConfidennce[KINECT_COUNT];
	vector<velVector*>	m_StateHytrVectors;
	
	float m_boneLength[Actual_BoneType_Count];
//	cv::Mat MeasureNoiyVecs[KINECT_COUNT];
//	cv::Mat MeasureNoiyCovs[KINECT_COUNT];
//	cv::Mat MeasureVecs[KINECT_COUNT];

	int frameNum;

};

