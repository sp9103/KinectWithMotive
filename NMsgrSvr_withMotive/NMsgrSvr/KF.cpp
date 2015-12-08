#include "stdafx.h"
#include "KF.h"
#include "NMsgrSvrDlg.h"

KF::KF(void)
{
	frameNum = 1;
	m_pDlg = NULL;
}


KF::~KF(void)
{
}


void KF::Initialize(CDialog* pDlg)
{
	// F, G, H	initialization	
	m_pDlg = pDlg;

	kalman  = cvCreateKalman(STATE_DIM, STATE_DIM, STATE_DIM);

/*	int col = kalman->measurement_noise_cov->cols;
	int row = kalman->measurement_noise_cov->rows;

	cvSetIdentity(kalman->measurement_matrix, cvRealScalar(1.0));
	float val = cvGetReal2D(kalman->measurement_matrix,1,1);
	val = cvGetReal2D(kalman->measurement_matrix,1,2);
	val = cvGetReal2D(kalman->measurement_matrix,2,2);
*/
//	memset(kalman, 0x00, sizeof(CvKalman));
	// set state vector and inial cov matrix
	m_state = cvCreateMat(STATE_DIM, 1, CV_32FC1);
	memset(m_state->data.fl, 0x00, sizeof(float)*STATE_DIM);
	memcpy(kalman->state_post->data.fl,  m_state->data.fl, sizeof(float)*STATE_DIM);
	// P_0: initial values for covariance matrix for states (big value; will be reduced anyway)
	cvSetIdentity(kalman->error_cov_post, cvRealScalar(1e+10));	

	// set transition model
	m_controlVec = cvCreateMat(STATE_DIM, 1, CV_32FC1);
	memset(m_controlVec->data.fl, 0x00, sizeof(float)*STATE_DIM);
	m_G = cvCreateMat(STATE_DIM, STATE_DIM, CV_32FC1);
	cvSetIdentity(m_G, cvRealScalar(1.0));
	
	m_F = cvCreateMat(STATE_DIM, STATE_DIM, CV_32FC1);
	cvSetIdentity(m_F, cvRealScalar(1.0));
	memcpy(kalman->transition_matrix->data.fl,  m_F->data.fl, sizeof(float)*STATE_DIM*STATE_DIM);

	m_CovMatReader.Initialize();
	m_CovMatReader.ReadPredctionVec("PredictionVec.txt");
	m_Q = cvCreateMat(STATE_DIM, STATE_DIM, CV_32FC1);
	cvSetIdentity(m_Q, cvRealScalar(10.0));
	setProcessNoiseCov();
	memcpy(kalman->process_noise_cov->data.fl,  m_Q->data.fl, sizeof(float)*STATE_DIM*STATE_DIM);

	m_CovMatReader.ReadOccludedVec("OccludedVec.txt");
	m_CovMatReader.ReadOccludingVec("OccludingVec.txt");

	// set measurement model
	m_H = cvCreateMat(STATE_DIM, STATE_DIM, CV_32FC1);
	cvSetIdentity(m_H, cvRealScalar(1.0));
	memcpy(kalman->measurement_matrix->data.fl, m_H->data.fl, sizeof(float)*STATE_DIM*STATE_DIM);

	m_R = cvCreateMat(STATE_DIM, STATE_DIM, CV_32FC1);
	cvSetIdentity(m_R, cvRealScalar(1.0));

	m_Measurement = cvCreateMat(STATE_DIM, 1, CV_32FC1);
	memset(m_Measurement->data.fl, 0x00, sizeof(float)*STATE_DIM);

	memset(&m_lastVelVector, 0x00, sizeof(velVector));
	m_lastSyncSkels = new synconizedSkeletons;
	memset(m_boneLength, 0x00, sizeof(float)*Actual_BoneType_Count);

	m_backupSyncSkel = new synconizedSkeletons;
}


void KF::KFExec(synconizedSkeletons* syncSkel, double sampling_interval, int bodyIdx)
{
	int KalmanStartFrameNum = 5;
	for (int i=0; i<Actual_JointType_Count;i++){
		for (int j=0; j <KINECT_COUNT; j++) {
			m_JointsConfidennce[j].confidence[i] = false;
		}
	}

	memcpy(m_backupSyncSkel, syncSkel, sizeof(synconizedSkeletons));

	if (frameNum > KalmanStartFrameNum) {
		CheckMeasurementsConfidence(sampling_interval, syncSkel, ((CNMsgrSvrDlg*)m_pDlg)->m_upperIdxarr[bodyIdx]);
//		CheckBodyConstraints1(syncSkelAll->syncSkels[0], prevSyncSkelAll->syncSkels[0]);
		MeasurementFusion(syncSkel);
	}else {
		for (int i=0; i<Actual_JointType_Count;i++){
			for (int j=0; j <KINECT_COUNT; j++) {
				if ( j== ((CNMsgrSvrDlg*)m_pDlg)->m_upperIdxarr[bodyIdx])			// SyncSkelsData_arm3 #1   SyncSkelsData_leg3  #2
					m_JointsConfidennce[j].confidence[i] = true;
				else
					m_JointsConfidennce[j].confidence[i] = false;
			}
		}
		
		initializeBonesLength(syncSkel, ((CNMsgrSvrDlg*)m_pDlg)->m_upperIdxarr[bodyIdx]);
		MeasurementFusion2(syncSkel);
	}

	preiction(sampling_interval);
	update();

	if (frameNum > 5)
		correction(syncSkel, ((CNMsgrSvrDlg*)m_pDlg)->m_upperIdxarr[bodyIdx]);

	for(int i = 0; i < ACTUAL_JOINT_COUNT; i++) {
		m_StateVector.joints[i].X = kalman->state_post->data.fl[i*3]; 
		m_StateVector.joints[i].Y = kalman->state_post->data.fl[i*3+1];  
		m_StateVector.joints[i].Z = kalman->state_post->data.fl[i*3+2]; 
	}

	if (frameNum > 1)
		calcVelocity(sampling_interval);

	for(int i = 0; i < ACTUAL_JOINT_COUNT; i++) {
		m_prevStateVector.joints[i].X = m_StateVector.joints[i].X;
		m_prevStateVector.joints[i].Y = m_StateVector.joints[i].Y; 
		m_prevStateVector.joints[i].Z = m_StateVector.joints[i].Z;
	}

	memcpy(m_lastSyncSkels, m_backupSyncSkel, sizeof(synconizedSkeletons));

	if (frameNum == KalmanStartFrameNum) {
		for (int i=0; i < Actual_BoneType_Count; i++)
			m_boneLength[i] = m_boneLength[i] / (float)KalmanStartFrameNum;
	}

	frameNum++;

	if (frameNum > 60000) frameNum = TIMEWINDOW + 1; 
}

void KF::correction(synconizedSkeletons* syncSkel, int refBodyIdx)
{
	float dist;
	float thresh = 0.12;

	int index[19];
	GetActualJointIdx(index);

	dist = calculateABoneDist(0, 0, 1);
	if( dist > thresh) {
		kalman->state_post->data.fl[0] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[0].Position.X;
		kalman->state_post->data.fl[1] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[0].Position.Y;
		kalman->state_post->data.fl[2] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[0].Position.Z;
		kalman->state_post->data.fl[3] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[1].Position.X;
		kalman->state_post->data.fl[4] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[1].Position.Y;
		kalman->state_post->data.fl[5] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[1].Position.Z;
	}	
	dist = calculateABoneDist(1, 1, 2);
	if( dist > thresh) {
		kalman->state_post->data.fl[3] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[1].Position.X;
		kalman->state_post->data.fl[4] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[1].Position.Y;
		kalman->state_post->data.fl[5] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[1].Position.Z;
		kalman->state_post->data.fl[6] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[2].Position.X;
		kalman->state_post->data.fl[7] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[2].Position.Y;
		kalman->state_post->data.fl[8] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[2].Position.Z;
	}
	dist = calculateABoneDist(2, 2, 3);
	if( dist > thresh) {
		kalman->state_post->data.fl[6] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[2].Position.X;
		kalman->state_post->data.fl[7] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[2].Position.Y;
		kalman->state_post->data.fl[8] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[2].Position.Z;
		kalman->state_post->data.fl[9] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[3].Position.X;
		kalman->state_post->data.fl[10] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[3].Position.Y;
		kalman->state_post->data.fl[11] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[3].Position.Z;
	}
	dist = calculateABoneDist(3, 18, 4);
	if( dist > thresh) {
		kalman->state_post->data.fl[3*18] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[index[18]].Position.X;
		kalman->state_post->data.fl[3*18+1] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[index[18]].Position.Y;
		kalman->state_post->data.fl[3*18+2] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[index[18]].Position.Z;
		kalman->state_post->data.fl[3*4] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[4].Position.X;
		kalman->state_post->data.fl[3*4+1] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[4].Position.Y;
		kalman->state_post->data.fl[3*4+2] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[4].Position.Z;
	}
	dist = calculateABoneDist(4, 4, 5);
	if( dist > thresh) {
		kalman->state_post->data.fl[3*4] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[4].Position.X;
		kalman->state_post->data.fl[3*4+1] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[4].Position.Y;
		kalman->state_post->data.fl[3*4+2] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[4].Position.Z;
		kalman->state_post->data.fl[3*5] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[5].Position.X;
		kalman->state_post->data.fl[3*5+1] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[5].Position.Y;
		kalman->state_post->data.fl[3*5+2] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[5].Position.Z;
	}
	dist = calculateABoneDist(5, 5, 6);
	if( dist > thresh) {
		kalman->state_post->data.fl[3*5] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[5].Position.X;
		kalman->state_post->data.fl[3*5+1] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[5].Position.Y;
		kalman->state_post->data.fl[3*5+2] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[5].Position.Z;
		kalman->state_post->data.fl[3*6] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[6].Position.X;
		kalman->state_post->data.fl[3*6+1] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[6].Position.Y;
		kalman->state_post->data.fl[3*6+2] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[6].Position.Z;
	}
	dist = calculateABoneDist(6, 6, 7);
	if( dist > thresh) {
		kalman->state_post->data.fl[3*6] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[6].Position.X;
		kalman->state_post->data.fl[3*6+1] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[6].Position.Y;
		kalman->state_post->data.fl[3*6+2] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[6].Position.Z;
		kalman->state_post->data.fl[3*7] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[7].Position.X;
		kalman->state_post->data.fl[3*7+1] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[7].Position.Y;
		kalman->state_post->data.fl[3*7+2] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[7].Position.Z;
	}
	dist = calculateABoneDist(7, 18, 8);
	if( dist > thresh) {
		kalman->state_post->data.fl[3*18] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[index[18]].Position.X;
		kalman->state_post->data.fl[3*18+1] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[index[18]].Position.Y;
		kalman->state_post->data.fl[3*18+2] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[index[18]].Position.Z;
		kalman->state_post->data.fl[3*8] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[8].Position.X;
		kalman->state_post->data.fl[3*8+1] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[8].Position.Y;
		kalman->state_post->data.fl[3*8+2] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[8].Position.Z;
	}
	dist = calculateABoneDist(8, 8, 9);
	if( dist > thresh) {
		kalman->state_post->data.fl[3*8] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[8].Position.X;
		kalman->state_post->data.fl[3*8+1] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[8].Position.Y;
		kalman->state_post->data.fl[3*8+2] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[8].Position.Z;
		kalman->state_post->data.fl[3*9] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[9].Position.X;
		kalman->state_post->data.fl[3*9+1] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[9].Position.Y;
		kalman->state_post->data.fl[3*9+2] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[9].Position.Z;	
	}
	dist = calculateABoneDist(9, 9, 10);
	if( dist > thresh) {
		kalman->state_post->data.fl[3*9] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[9].Position.X;
		kalman->state_post->data.fl[3*9+1] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[9].Position.Y;
		kalman->state_post->data.fl[3*9+2] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[9].Position.Z;
		kalman->state_post->data.fl[3*10] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[10].Position.X;
		kalman->state_post->data.fl[3*10+1] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[10].Position.Y;
		kalman->state_post->data.fl[3*10+2] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[10].Position.Z;
	}
	dist = calculateABoneDist(10, 10, 11);
	if( dist > thresh) {
		kalman->state_post->data.fl[3*10] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[10].Position.X;
		kalman->state_post->data.fl[3*10+1] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[10].Position.Y;
		kalman->state_post->data.fl[3*10+2] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[10].Position.Z;
		kalman->state_post->data.fl[3*11] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[11].Position.X;
		kalman->state_post->data.fl[3*11+1] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[11].Position.Y;
		kalman->state_post->data.fl[3*11+2] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[11].Position.Z;
	}
	dist = calculateABoneDist(11, 0, 12);
	if( dist > thresh) {
		kalman->state_post->data.fl[3*0] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[0].Position.X;
		kalman->state_post->data.fl[3*0+1] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[0].Position.Y;
		kalman->state_post->data.fl[3*0+2] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[0].Position.Z;
		kalman->state_post->data.fl[3*12] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[12].Position.X;
		kalman->state_post->data.fl[3*12+1] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[12].Position.Y;
		kalman->state_post->data.fl[3*12+2] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[12].Position.Z;
	}
	dist = calculateABoneDist(12, 12, 13);
	if( dist > thresh) {
		kalman->state_post->data.fl[3*12] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[12].Position.X;
		kalman->state_post->data.fl[3*12+1] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[12].Position.Y;
		kalman->state_post->data.fl[3*12+2] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[12].Position.Z;
		kalman->state_post->data.fl[3*13] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[13].Position.X;
		kalman->state_post->data.fl[3*13+1] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[13].Position.Y;
		kalman->state_post->data.fl[3*13+2] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[13].Position.Z;
	}
	dist = calculateABoneDist(13, 13, 14);
	if( dist > thresh) {
		kalman->state_post->data.fl[3*13] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[13].Position.X;
		kalman->state_post->data.fl[3*13+1] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[13].Position.Y;
		kalman->state_post->data.fl[3*13+2] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[13].Position.Z;
		kalman->state_post->data.fl[3*14] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[14].Position.X;
		kalman->state_post->data.fl[3*14+1] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[14].Position.Y;
		kalman->state_post->data.fl[3*14+2] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[14].Position.Z;		
	}
	dist = calculateABoneDist(14, 0, 15);
	if( dist > thresh) {
		kalman->state_post->data.fl[3*0] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[0].Position.X;
		kalman->state_post->data.fl[3*0+1] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[0].Position.Y;
		kalman->state_post->data.fl[3*0+2] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[0].Position.Z;
		kalman->state_post->data.fl[3*15] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[index[15]].Position.X;
		kalman->state_post->data.fl[3*15+1] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[index[15]].Position.Y;
		kalman->state_post->data.fl[3*15+2] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[index[15]].Position.Z;	
	}
	dist = calculateABoneDist(15, 15, 16);
	if( dist > thresh) {
		kalman->state_post->data.fl[3*15] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[index[15]].Position.X;
		kalman->state_post->data.fl[3*15+1] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[index[15]].Position.Y;
		kalman->state_post->data.fl[3*15+2] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[index[15]].Position.Z;
		kalman->state_post->data.fl[3*16] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[index[16]].Position.X;
		kalman->state_post->data.fl[3*16+1] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[index[16]].Position.Y;
		kalman->state_post->data.fl[3*16+2] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[index[16]].Position.Z;
	}
	dist = calculateABoneDist(16, 16, 17);
	if( dist > thresh) {
		kalman->state_post->data.fl[3*16] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[index[16]].Position.X;
		kalman->state_post->data.fl[3*16+1] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[index[16]].Position.Y;
		kalman->state_post->data.fl[3*16+2] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[index[16]].Position.Z;
		kalman->state_post->data.fl[3*17] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[index[17]].Position.X;
		kalman->state_post->data.fl[3*17+1] = m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[index[17]].Position.Y;
		kalman->state_post->data.fl[3*17+2]= m_backupSyncSkel->InfoBody[refBodyIdx].JointPos[index[17]].Position.Z;
	}		
}

float KF::calculateABoneDist(int targetBoneIdx, int fromJointIdx, int toJintIdx)
{
	float dist = 0.0;
	float length = 0.0;

	length = pow(m_StateVector.joints[fromJointIdx].X-m_StateVector.joints[toJintIdx].X,2);
	length = length + pow(m_StateVector.joints[fromJointIdx].Y-m_StateVector.joints[toJintIdx].Y,2);
	length = length + pow(m_StateVector.joints[fromJointIdx].Z-m_StateVector.joints[toJintIdx].Z,2);
	length = sqrt(length);

	dist = fabs(m_boneLength[targetBoneIdx] - length);

	return dist;
}

void KF::initializeBonesLength(synconizedSkeletons* syncSkel, int refIdx)
{
	calculateABoneLength(syncSkel, refIdx, 0, 0, 1);
	calculateABoneLength(syncSkel, refIdx, 1, 1, 2);
	calculateABoneLength(syncSkel, refIdx, 2, 2, 3);
	calculateABoneLength(syncSkel, refIdx, 3, 20, 4);
	calculateABoneLength(syncSkel, refIdx, 4, 4, 5);
	calculateABoneLength(syncSkel, refIdx, 5, 5, 6);
	calculateABoneLength(syncSkel, refIdx, 6, 6, 7);
	calculateABoneLength(syncSkel, refIdx, 7, 20, 8);
	calculateABoneLength(syncSkel, refIdx, 8, 8, 9);
	calculateABoneLength(syncSkel, refIdx, 9, 9, 10);
	calculateABoneLength(syncSkel, refIdx, 10, 10, 11);
	calculateABoneLength(syncSkel, refIdx, 11, 0, 12);
	calculateABoneLength(syncSkel, refIdx, 12, 12, 13);
	calculateABoneLength(syncSkel, refIdx, 13, 13, 14);
	calculateABoneLength(syncSkel, refIdx, 14, 0, 16);
	calculateABoneLength(syncSkel, refIdx, 15, 16, 17);
	calculateABoneLength(syncSkel, refIdx, 16, 17, 18);
}

void KF::calculateABoneLength(synconizedSkeletons* syncSkel, int refIdx, int targetBoneIdx, int fromJointIdx, int toJintIdx)
{
	float length = 0.0;
	float aveLength = 0.0;
	int count=0;

	for(int i=0; i < BODY_COUNT; i++) {
		if(syncSkel->bObsevingKinect[i]) { 
			length = pow(syncSkel->InfoBody[i].JointPos[fromJointIdx].Position.X-syncSkel->InfoBody[i].JointPos[toJintIdx].Position.X,2);
			length = length + pow(syncSkel->InfoBody[i].JointPos[fromJointIdx].Position.Y-syncSkel->InfoBody[i].JointPos[toJintIdx].Position.Y,2);
			length = length + pow(syncSkel->InfoBody[i].JointPos[fromJointIdx].Position.Z-syncSkel->InfoBody[i].JointPos[toJintIdx].Position.Z,2);
			length = sqrt(length);
			aveLength = aveLength + length;
			count++;
		}
	}
	aveLength = aveLength/float(count);
	m_boneLength[targetBoneIdx] = m_boneLength[targetBoneIdx] + aveLength;
}

void KF::setProcessNoiseCov()
{
	for(int i = 0; i < STATE_DIM; i++){
		for(int j = 0; j < STATE_DIM; j++){
			if (i == j)
				m_Q->data.fl[i*STATE_DIM+j] = cvGetReal1D(m_CovMatReader.PredVec,i%3)*50000;
		}
	}
}

void KF::copyVelVecToDouble(double *vVec)
{
	int i, j;
	
	for(i=0; i < Actual_JointType_Count; i++) {
		for(j=0; j < TIMEWINDOW; j++) {
			vVec[i*(TIMEWINDOW*3)+j] = m_VelVectors[j]->Jointvels[i].dx;
		}
		for(j=0; j < TIMEWINDOW; j++) {
			vVec[i*(TIMEWINDOW*3)+TIMEWINDOW+j] = m_VelVectors[j]->Jointvels[i].dy;
		}
		for(j=0; j < TIMEWINDOW; j++) {
			vVec[i*(TIMEWINDOW*3)+TIMEWINDOW*2+j] = m_VelVectors[j]->Jointvels[i].dz;
		}
	}
}

void KF::copyDoubleToVelVec(double *vVec)
{
	int i;
	for(i=0; i < Actual_JointType_Count; i++) {
		m_lastVelVector.Jointvels[i].dx = vVec[3*i];
		m_lastVelVector.Jointvels[i].dy = vVec[3*i+1];
		m_lastVelVector.Jointvels[i].dz = vVec[3*i+2];
	}
}

void KF::CheckMeasurementsConfidence(double sampling_interval, synconizedSkeletons* syncSkels, int RefBodyIdx)
{
	int i, j, k;
	float dist, dist2;
	float threshDiscont  = 0.05;
	float threshSlowMotion = 0.02;
	int count, count2;
	float moving_dists[Actual_JointType_Count];

	int index[19];
	bool bCheckDiscontinuity[KINECT_COUNT];
	bool bCheckSlowMotion[KINECT_COUNT];
	bool bConsistentWithPredict[KINECT_COUNT];
	bool slowMotion;
	bool bFind = false;

	GetActualJointIdx(index);

	for (i=0; i<Actual_JointType_Count;i++){
		PredictStateVector.joints[i].X = m_StateVector.joints[i].X + m_lastVelVector.Jointvels[i].dx*sampling_interval;
		dist = pow(m_lastVelVector.Jointvels[i].dx*sampling_interval,2);
		PredictStateVector.joints[i].Y = m_StateVector.joints[i].Y + m_lastVelVector.Jointvels[i].dy*sampling_interval;
		dist = dist + pow(m_lastVelVector.Jointvels[i].dy*sampling_interval,2);
		PredictStateVector.joints[i].Z = m_StateVector.joints[i].Z + m_lastVelVector.Jointvels[i].dz*sampling_interval;
		dist = dist + pow(m_lastVelVector.Jointvels[i].dz*sampling_interval,2);
		moving_dists[i] = sqrt(dist);

		if (i == 10)
			int aa = 0;

		count = 0;
		count2 = 0;
		for (j=0; j < KINECT_COUNT; j++) {
			if (syncSkels->bObsevingKinect[j] && m_lastSyncSkels->bObsevingKinect[j]) {
				dist = pow(m_lastSyncSkels->InfoBody[j].JointPos[index[i]].Position.X - syncSkels->InfoBody[j].JointPos[index[i]].Position.X,2);
				dist += pow(m_lastSyncSkels->InfoBody[j].JointPos[index[i]].Position.Y - syncSkels->InfoBody[j].JointPos[index[i]].Position.Y,2);
				dist += pow(m_lastSyncSkels->InfoBody[j].JointPos[index[i]].Position.Z - syncSkels->InfoBody[j].JointPos[index[i]].Position.Z,2);
				dist = sqrt(dist);

				if(dist < threshSlowMotion)
					count++;

				count2++;
			}
		}
		if (count > 1) {
			PredictStateVector.joints[i].X = m_StateVector.joints[i].X;
			PredictStateVector.joints[i].Y = m_StateVector.joints[i].Y;
			PredictStateVector.joints[i].Z = m_StateVector.joints[i].Z;
		}
	}

	/* discontinous motion */
	if (syncSkels->numObservingKinect > 3) {
		for (i=0; i<Actual_JointType_Count;i++){
			slowMotion = false;

			if(i== 10)
				int a = 0;

			dist = 0.0;
			for (j=0; j < KINECT_COUNT; j++) {
				bCheckDiscontinuity[j] = false;
				bCheckSlowMotion[j] = false;
				if (syncSkels->bObsevingKinect[j] && m_lastSyncSkels->bObsevingKinect[j]) {
					dist = pow(m_lastSyncSkels->InfoBody[j].JointPos[index[i]].Position.X - syncSkels->InfoBody[j].JointPos[index[i]].Position.X,2);
					dist += pow(m_lastSyncSkels->InfoBody[j].JointPos[index[i]].Position.Y - syncSkels->InfoBody[j].JointPos[index[i]].Position.Y,2);
					dist += pow(m_lastSyncSkels->InfoBody[j].JointPos[index[i]].Position.Z - syncSkels->InfoBody[j].JointPos[index[i]].Position.Z,2);
					dist = sqrt(dist);

					if (dist > threshDiscont) {
						bCheckDiscontinuity[j] = true;
					}
					if (dist < threshSlowMotion) {
						bCheckSlowMotion[j] = true;
					}
				}
			}

			count = 0;
			for (j=0; j < KINECT_COUNT; j++) {
				if (syncSkels->bObsevingKinect[j] && m_lastSyncSkels->bObsevingKinect[j]) {
					if(bCheckSlowMotion[j]) 
						count++;
				}
			}

			if(count > 1) {
				slowMotion = true;
			}

			if(i== 10)
				int a = 0;

			for (j=0; j < KINECT_COUNT; j++) {
				if(bCheckDiscontinuity[j])  {
					for (k=0; k < KINECT_COUNT; k++) {
						dist = 1.0;
						if(bCheckSlowMotion[k]) {
							dist = pow(m_lastSyncSkels->InfoBody[j].JointPos[index[i]].Position.X - syncSkels->InfoBody[k].JointPos[index[i]].Position.X,2);
							dist += pow(m_lastSyncSkels->InfoBody[j].JointPos[index[i]].Position.Y - syncSkels->InfoBody[k].JointPos[index[i]].Position.Y,2);
							dist += pow(m_lastSyncSkels->InfoBody[j].JointPos[index[i]].Position.Z - syncSkels->InfoBody[k].JointPos[index[i]].Position.Z,2);
							dist = sqrt(dist);

							if (dist < threshSlowMotion) {
								syncSkels->InfoBody[j].JointPos[index[i]].Position.X =  syncSkels->InfoBody[k].JointPos[index[i]].Position.X;
								syncSkels->InfoBody[j].JointPos[index[i]].Position.Y =  syncSkels->InfoBody[k].JointPos[index[i]].Position.Y;
								syncSkels->InfoBody[j].JointPos[index[i]].Position.Z =  syncSkels->InfoBody[k].JointPos[index[i]].Position.Z;

								m_backupSyncSkel->InfoBody[j].JointPos[index[i]].Position.X =  syncSkels->InfoBody[j].JointPos[index[i]].Position.X;
								m_backupSyncSkel->InfoBody[j].JointPos[index[i]].Position.Y =  syncSkels->InfoBody[j].JointPos[index[i]].Position.Y;
								m_backupSyncSkel->InfoBody[j].JointPos[index[i]].Position.Z =  syncSkels->InfoBody[j].JointPos[index[i]].Position.Z;
								break;
							}
						}
					}
				}
			}
		}
	}
	/* discontinous motion */

	/* beleive Reference Kinect */
/*	bFind = false;

	for (i=0; i<Actual_JointType_Count;i++){
		bFind = false;

		dist = pow(m_lastSyncSkels->InfoBody[RefBodyIdx].JointPos[index[i]].Position.X - syncSkels->InfoBody[RefBodyIdx].JointPos[index[i]].Position.X,2);
		dist += pow(m_lastSyncSkels->InfoBody[RefBodyIdx].JointPos[index[i]].Position.Y - syncSkels->InfoBody[RefBodyIdx].JointPos[index[i]].Position.Y,2);
		dist += pow(m_lastSyncSkels->InfoBody[RefBodyIdx].JointPos[index[i]].Position.Z - syncSkels->InfoBody[RefBodyIdx].JointPos[index[i]].Position.Z,2);
		dist = sqrt(dist);

		if (dist < threshSlowMotion){//moving_dists[i] + 0.03 && dist > moving_dists[i] - 0.03) {
			bFind = true;
		}

		if (bFind) {
			for (j=0; j < KINECT_COUNT; j++) {
				if (fabs(syncSkels->InfoBody[RefBodyIdx].JointPos[index[i]].Position.X - syncSkels->InfoBody[j].JointPos[index[i]].Position.X) > 0.05)
					syncSkels->InfoBody[j].JointPos[index[i]].Position.X = 0.0;
				if (fabs(syncSkels->InfoBody[RefBodyIdx].JointPos[index[i]].Position.Y - syncSkels->InfoBody[j].JointPos[index[i]].Position.Y) > 0.05)
					syncSkels->InfoBody[j].JointPos[index[i]].Position.Y = 0.0;
				if (fabs(syncSkels->InfoBody[RefBodyIdx].JointPos[index[i]].Position.Z - syncSkels->InfoBody[j].JointPos[index[i]].Position.Z) > 0.05)
					syncSkels->InfoBody[j].JointPos[index[i]].Position.Z = 0.0;
			}
		}
	}
*/
	/* beleive PredictStateVector */
	bFind = false;
	for (i=0; i<Actual_JointType_Count;i++){
		memset(bConsistentWithPredict, 0x00, sizeof(bool)*KINECT_COUNT);
		bFind = false;

		if (i == 10)
			int a = 0;

		count = 0;
		for (j=0; j < KINECT_COUNT; j++) {
			dist = pow(PredictStateVector.joints[i].X - syncSkels->InfoBody[j].JointPos[index[i]].Position.X,2);
			dist += pow(PredictStateVector.joints[i].Y - syncSkels->InfoBody[j].JointPos[index[i]].Position.Y,2);
			dist += pow(PredictStateVector.joints[i].Z - syncSkels->InfoBody[j].JointPos[index[i]].Position.Z,2);
			dist = sqrt(dist);

			if (dist < 0.03) {
				bConsistentWithPredict[j] = true;
				count++;
			}
		}
		if(count > 1)			
			bFind = true;

		if(bFind) {
			if (i == 10)
				int a = 0;

			for (j=0; j < KINECT_COUNT; j++) {
				if (!bConsistentWithPredict[j]) {
					syncSkels->InfoBody[j].JointPos[index[i]].Position.X = 0.0;
					syncSkels->InfoBody[j].JointPos[index[i]].Position.Y = 0.0;
					syncSkels->InfoBody[j].JointPos[index[i]].Position.Z = 0.0;
				}
			}
		}
	}

}

bool KF::CheckBodyConstraints3(synconizedSkeletons* syncSkels, int i, int refsIdx, int refeIdx, int compsIdx, int compeIdx)
{
	XYZ refS; 	XYZ refE;
	XYZ compS; 	XYZ compE;

	XYZ dP;
	float dist;

	refS.x = PredictStateVector.joints[refsIdx].X;
	refS.y = PredictStateVector.joints[refsIdx].Y;
	refS.z = PredictStateVector.joints[refsIdx].Z;
	refE.x = PredictStateVector.joints[refeIdx].X;
	refE.y = PredictStateVector.joints[refeIdx].Y;
	refE.z = PredictStateVector.joints[refeIdx].Z;

	compS.x = syncSkels->InfoBody[i].JointPos[compsIdx].Position.X;
	compS.y = syncSkels->InfoBody[i].JointPos[compsIdx].Position.Y;
	compS.z = syncSkels->InfoBody[i].JointPos[compsIdx].Position.Z;
	compE.x = syncSkels->InfoBody[i].JointPos[compeIdx].Position.X;
	compE.y = syncSkels->InfoBody[i].JointPos[compeIdx].Position.Y;
	compE.z = syncSkels->InfoBody[i].JointPos[compeIdx].Position.Z;

	if(!LineLineIntersect(refS, refE, compS, compE, &dP))
		return false;

	dist = norm(dP);

	if (dist < 0.05)
		return false;

	return true;
}

bool KF::LineLineIntersect(XYZ p1,XYZ p2,XYZ p3,XYZ p4,XYZ *dP)
{
   XYZ u,v,w;
   XYZ p12, p34;

   u.x = p2.x - p1.x;
   u.y = p2.y - p1.y;
   u.z = p2.z - p1.z;

   v.x = p4.x - p3.x;
   v.y = p4.y - p3.y;
   v.z = p4.z - p3.z;

   w.x = p1.x - p3.x;
   w.y = p1.y - p3.y;
   w.z = p1.z - p3.z;

   float    a = dot(u,u);         // always >= 0
   float    b = dot(u,v);
   float    c = dot(v,v);         // always >= 0
   float    d = dot(u,w);
   float    e = dot(v,w);
   float    D = a*c - b*b;        // always >= 0
   float    sD, tD;
   float    sN, tN;
   float    sc, tc;
   sD = D;
   tD = D;
    // compute the line parameters of the two closest points
    if (D < SMALL_NUM){  // the lines are almost parallel
        sN = 0.0;       // force using point P0 on segment S1
        sD = 1.0;       // to prevent possible division by 0.0 later
        tN = e;
        tD = c;
	}else {                //% get the closest points on the infinite lines
        sN = (b*e - c*d);
        tN = (a*e - b*d);
        if (sN < 0.0){   //% sc < 0 => the s=0 edge is visible       
            sN = 0.0;
            tN = e;
            tD = c;
		} else if (sN > sD){ //% sc > 1 => the s=1 edge is visible
            sN = sD;
            tN = e + b;
            tD = c;
		}
	}

    if (tN < 0.0) {           //% tc < 0 => the t=0 edge is visible
        tN = 0.0;
        //% recompute sc for this edge
        if (-d < 0.0) {
            sN = 0.0;
		}else if (-d > a) {
            sN = sD;
		}else {
            sN = -d; sD = a;
		}
	} else if (tN > tD) {       //% tc > 1 => the t=1 edge is visible
        tN = tD;
        //% recompute sc for this edge
        if ((-d + b) < 0.0) {
            sN = 0;
		}else if ((-d + b) > a) {
            sN = sD;
		}else { 
            sN = (-d + b); sD = a;
	  }
	}

    //% finally do the division to get sc and tc
    if(fabs(sN) < SMALL_NUM) {
        sc = 0.0;
	}else {
        sc = sN / sD;
	}
    
    if(fabs(tN) < SMALL_NUM) {
        tc = 0.0;
	}else {
        tc = tN / tD;
	}

    // get the difference of the two closest points
    dP->x = w.x + (sc * u.x) - (tc * v.x);  // =  L1(sc) - L2(tc)
	dP->y = w.y + (sc * u.y) - (tc * v.y);  // =  L1(sc) - L2(tc)
	dP->z = w.z + (sc * u.z) - (tc * v.z);  // =  L1(sc) - L2(tc)

	p12.x = p1.x*sc*u.x;
	p12.y = p1.y*sc*u.y;
	p12.z = p1.z*sc*u.z;

	p34.x = p3.x*tc*v.x;
	p34.y = p3.y*tc*v.y;
	p34.z = p3.z*tc*v.z;

   return(TRUE);
}

bool KF::CheckBodyConstraints2(synconizedSkeletons* syncSkels, int i)
{
	float DotProductVal;
	float Denorm;
	float Degree;

	MyVector vHipL;
	MyVector vHipR;

	vHipL.sx1 = PredictStateVector.joints[0].X; 
	vHipL.sx2 = PredictStateVector.joints[0].Z;
	vHipL.ex1 = syncSkels->InfoBody[i].JointPos[12].Position.X;
	vHipL.ex2 = syncSkels->InfoBody[i].JointPos[12].Position.Z; 
	vHipL.x1 = vHipL.ex1 - vHipL.sx1;
	vHipL.x2 = vHipL.ex2 - vHipL.sx2;

	vHipR.sx1 = PredictStateVector.joints[0].X;
	vHipR.sx2 = PredictStateVector.joints[0].Z;
	vHipR.ex1 = syncSkels->InfoBody[i].JointPos[16].Position.X;
	vHipR.ex2 = syncSkels->InfoBody[i].JointPos[16].Position.Z;
	vHipR.x1 = vHipR.ex1 - vHipR.sx1;
	vHipR.x2 = vHipR.ex2 - vHipR.sx2;

	DotProductVal = vHipL.x1*vHipR.x1 + vHipL.x2*vHipR.x2;
	Denorm = sqrt(pow(vHipL.x1,2)+pow(vHipL.x2,2))*sqrt(pow(vHipR.x1,2)+pow(vHipR.x2,2));
	Degree = acos (DotProductVal/Denorm) * 180.0 / PI;

	if (vHipL.ex2 > vHipR.ex2)
		if (vHipL.ex1 > vHipL.sx1)
			return false;

	if (vHipL.ex2 <= vHipR.ex2)
		if (vHipR.ex1 < vHipR.sx1)
			return false;
		
	if (Degree > 110)
		return true;

/*	if (vHipL.sx1 > vHipL.ex1 && vHipR.sx1 > vHipR.ex1)
		if (Degree < 50.0 || Degree > (180 - 50.0))
			return true;
*/
	return false;	
}

bool KF::CheckBodyConstraints1(synconizedSkeletons* syncSkels, int i)
{
	float DotProductVal;
	float Denorm;
	float Degree;

	MyVector vShoulder;
	MyVector vHip;

	vShoulder.sx1 = PredictStateVector.joints[4].X;
	vShoulder.sx2 = PredictStateVector.joints[4].Z;
	vShoulder.ex1 = PredictStateVector.joints[8].X;
	vShoulder.ex2 = PredictStateVector.joints[8].Z;
	vShoulder.x1 = vShoulder.ex1 - vShoulder.sx1;
	vShoulder.x2 = vShoulder.ex2 - vShoulder.sx2;

	vHip.sx1 = syncSkels->InfoBody[i].JointPos[12].Position.X;
	vHip.sx2 = syncSkels->InfoBody[i].JointPos[12].Position.Z;
	vHip.ex1 = syncSkels->InfoBody[i].JointPos[16].Position.X;
	vHip.ex2 = syncSkels->InfoBody[i].JointPos[16].Position.Z;
	vHip.x1 = vHip.ex1 - vHip.sx1;
	vHip.x2 = vHip.ex2 - vHip.sx2;

	DotProductVal = vShoulder.x1*vHip.x1 + vShoulder.x2*vHip.x2;
	Denorm = sqrt(pow(vShoulder.x1,2)+pow(vShoulder.x2,2))*sqrt(pow(vHip.x1,2)+pow(vHip.x2,2));
	Degree = acos (DotProductVal/Denorm) * 180.0 / PI;

//	if (vShoulder.sx1 < vShoulder.ex1 && vHip.sx1 < vHip.ex1)
	if (Degree < 60.0)
		return true;

	return false;

}

bool KF::CheckBodyConstraints4(synconizedSkeletons* syncSkels, int i)
{
	float DotProductVal;
	float Denorm;
	float Degree;

	MyVector LShoulder;
	MyVector RShoulder;

	LShoulder.sx1 = PredictStateVector.joints[18].X; 
	LShoulder.sx2 = PredictStateVector.joints[18].Z;
	LShoulder.ex1 = syncSkels->InfoBody[i].JointPos[4].Position.X;
	LShoulder.ex2 = syncSkels->InfoBody[i].JointPos[4].Position.Z; 
	LShoulder.x1 = LShoulder.ex1 - LShoulder.sx1;
	LShoulder.x2 = LShoulder.ex2 - LShoulder.sx2;

	RShoulder.sx1 = PredictStateVector.joints[18].X;
	RShoulder.sx2 = PredictStateVector.joints[18].Z;
	RShoulder.ex1 = syncSkels->InfoBody[i].JointPos[8].Position.X;
	RShoulder.ex2 = syncSkels->InfoBody[i].JointPos[8].Position.Z;
	RShoulder.x1 = RShoulder.ex1 - RShoulder.sx1;
	RShoulder.x2 = RShoulder.ex2 - RShoulder.sx2;

	DotProductVal = LShoulder.x1*RShoulder.x1 + LShoulder.x2*RShoulder.x2;
	Denorm = sqrt(pow(LShoulder.x1,2)+pow(LShoulder.x2,2))*sqrt(pow(RShoulder.x1,2)+pow(RShoulder.x2,2));
	Degree = acos (DotProductVal/Denorm) * 180.0 / PI;

//	if (vShoulder.sx1 < vShoulder.ex1 && vHip.sx1 < vHip.ex1)
	if (Degree > 160.0)
		return true;

	return false;
}

float KF::normal_pdf(float x, float m, TrackingState TrackingState, int jointIdx, int dim)
{
	float variance, variance1, variance2;
	float sigma, sigma1, sigma2;
	float weigh;

	//kalman->error_cov_post
	variance1 = cvGetReal2D(kalman->error_cov_post,jointIdx*3+dim,jointIdx*3+dim);
	sigma1 = sqrt(variance1);

	variance2 = cvGetReal1D(m_CovMatReader.PredVec,dim); 
	sigma2 = sqrt(variance2);

	variance1 = 0.1;
//	variance1 = 0.03;

//	if (jointIdx == 12 || jointIdx == 15)
//		variance1 = 0.01;

	variance = variance1/10.0;// + variance2;
//	variance = (variance1/10.0);// + variance2;
//	variance = (variance1/10.0);// + variance2;
//	variance = (variance1 + variance2)*20.0;// + variance2;

    float denom = 1.0/sqrt(2*PI* variance); 
	float a = (x - m) ;

	float prob = denom*exp(-0.5 * a * a / variance);

//	weigh = prob

    return prob;
}

void KF::calcVelocity(double sampling_interval)
{
	int i, j;
	velVector *velVec, *lastVelVec;
	velVec = new velVector;
	double *vel, *filteredVel=NULL;
	float alpha = 0.7;

//	vel = new double[STATE_DIM/2];
	for (i=0; i<Actual_JointType_Count;i++){
		velVec->Jointvels[i].dx = (m_StateVector.joints[i].X - m_prevStateVector.joints[i].X)/sampling_interval;
		velVec->Jointvels[i].dy = (m_StateVector.joints[i].Y - m_prevStateVector.joints[i].Y)/sampling_interval;
		velVec->Jointvels[i].dz = (m_StateVector.joints[i].Z - m_prevStateVector.joints[i].Z)/sampling_interval;
	}

	
	//memcpy(&m_lastVelVector, velVec, sizeof(velVector));
	//memset(&m_lastVelVector, 0x00, sizeof(velVector));
	if (m_VelVectors.size() > 1) {
		lastVelVec = m_VelVectors.back();

		for (int k=0; k<Actual_JointType_Count; k++){

			if(k == 10) 
				int a = 0;

			m_lastVelVector.Jointvels[k].dx = alpha * velVec->Jointvels[k].dx + (1-alpha)*lastVelVec->Jointvels[k].dx;
			m_lastVelVector.Jointvels[k].dy = alpha * velVec->Jointvels[k].dy + (1-alpha)*lastVelVec->Jointvels[k].dy;
			m_lastVelVector.Jointvels[k].dz = alpha * velVec->Jointvels[k].dz + (1-alpha)*lastVelVec->Jointvels[k].dz;
		}
		memcpy(velVec, &m_lastVelVector, sizeof(velVector));
	}

	m_VelVectors.push_back(velVec);
	
	if (m_VelVectors.size() > TIMEWINDOW) {
		velVec = m_VelVectors.front();
		delete velVec;
		m_VelVectors.erase(m_VelVectors.begin());
	}
/*	if (m_VelVectors.size() > TIMEWINDOW) {

		for (int k=0; k<Actual_JointType_Count; k++){
			m_lastVelVector.Jointvels[k].dx = alpha * velVec->Jointvels[k].dx + (1-alpha)*m_lastVelVector.Jointvels[k].dx;
			m_lastVelVector.Jointvels[k].dy = alpha * velVec->Jointvels[k].dy + (1-alpha)*m_lastVelVector.Jointvels[k].dy;
			m_lastVelVector.Jointvels[k].dz = alpha * velVec->Jointvels[k].dz + (1-alpha)*m_lastVelVector.Jointvels[k].dz;
		}

		velVec = m_VelVectors.front();
		delete velVec;
		m_VelVectors.erase(m_VelVectors.begin());

		vel = new double[TIMEWINDOW*STATE_DIM/2];
		copyVelVecToDouble(vel);
		
		filteredVel = ((CNMsgrSvrDlg*)m_pDlg)->calcFilteredVel(vel, TIMEWINDOW, STATE_DIM/2);
	
		copyDoubleToVelVec(filteredVel);
		delete filteredVel;
		delete vel;

	}*/
/*	else{
		memcpy(&m_lastVelVector, velVec, sizeof(velVector));
	}
*/
}

void KF::GetActualJointIdx(int *index)
{
	index[0] = _ActualJointType::SpineBase; 	index[1] = _ActualJointType::SpineMid;
	index[2] = _ActualJointType::Neck;			index[3] = _ActualJointType::Head;
	index[4] = _ActualJointType::ShoulderLeft; 	index[5] = _ActualJointType::ElbowLeft;
	index[6] = _ActualJointType::WristLeft; 	index[7] = _ActualJointType::HandLeft;
	index[8] = _ActualJointType::ShoulderRight; index[9] = _ActualJointType::ElbowRight;
	index[10] = _ActualJointType::WristRight; 	index[11] = _ActualJointType::HandRight;
	index[12] = _ActualJointType::HipLeft;		index[13] = _ActualJointType::KneeLeft;
	index[14] = _ActualJointType::AnkleLeft; 	index[15] = _ActualJointType::HipRight;
	index[16] = _ActualJointType::KneeRight; 	index[17] = _ActualJointType::AnkleRight;
	index[18] = _ActualJointType::SpineShoulder;
}

void KF::MeasurementFusion(synconizedSkeletons* syncSkel)
{
	int i, j, k;
	float val, val2, prob_th;
	float obvX, obvY, obvZ;
	float probX, probY, probZ;
	float thresh = 6.0;
	cv::Mat MeasureNoiyVecs[KINECT_COUNT];
	cv::Mat MeasureNoiyCovs[KINECT_COUNT];
	cv::Mat MeasureVecs[KINECT_COUNT];
	cv::Mat MeasureFusedVec;
	cv::Mat MeasureFusedCov;
	cv::Mat MeasureMatrix;
	cv::Mat TmpMeasureMatrix;

/*	cv::Mat MeasureFusedVec2;
	MeasureFusedVec2.create(STATE_DIM, 1, CV_32FC1);
	cv::Mat MeasureFusedCov2;;
	MeasureFusedCov2.create(STATE_DIM, STATE_DIM, CV_32FC1);
*/
	MeasureFusedVec.create(STATE_DIM, 1, CV_32FC1);
	MeasureFusedCov.create(STATE_DIM, STATE_DIM, CV_32FC1);
	MeasureMatrix.create(STATE_DIM, STATE_DIM, CV_32FC1);
	TmpMeasureMatrix.create(STATE_DIM, STATE_DIM, CV_32FC1);

	_JointStatus jointStatus;
	TrackingState trackingState;
	
	MeasureMatrix.setTo(cv::Scalar(0.0));
	MeasureMatrix.diag().setTo(1.0); 
	MeasureFusedVec.setTo(cv::Scalar(0.0));
	MeasureFusedCov.setTo(cv::Scalar(0.0));
	TmpMeasureMatrix.setTo(cv::Scalar(0.0));

	int index[19];
	GetActualJointIdx(index);

	prob_th = 0.5;
//	prob_th = 0.1;

	for(i=0; i < KINECT_COUNT; i++) {
		MeasureNoiyVecs[i].create(STATE_DIM, 1, CV_32FC1);
		MeasureNoiyVecs[i].setTo(cv::Scalar(0.0));
		MeasureNoiyCovs[i].create(STATE_DIM, STATE_DIM, CV_32FC1);
		MeasureNoiyCovs[i].setTo(cv::Scalar(0.0));
		MeasureVecs[i].create(STATE_DIM, 1, CV_32FC1);
		MeasureVecs[i].setTo(cv::Scalar(0.0));
		for(j=0; j < Actual_JointType_Count; j++) {

			trackingState = syncSkel->InfoBody[i].JointPos[index[j]].TrackingState;

			if ( index[j] == 10)			
				int aaa = 0;

/*			if (j == 0 || j == 1 || j == 2 || j == 3 || j == 18 || j == 12 || j == 15) 
				prob_th = 3.0;
			else
				prob_th = 1.5;
*/
			obvX = syncSkel->InfoBody[i].JointPos[index[j]].Position.X;
			MeasureVecs[i].at<float>(j*3) = obvX;
			obvY = syncSkel->InfoBody[i].JointPos[index[j]].Position.Y;
			MeasureVecs[i].at<float>(j*3+1) = obvY;
			obvZ = syncSkel->InfoBody[i].JointPos[index[j]].Position.Z;
			MeasureVecs[i].at<float>(j*3+2) = obvZ;


			probX = normal_pdf(PredictStateVector.joints[j].X, obvX, trackingState, j, 0);
			if (probX < prob_th) MeasureNoiyVecs[i].at<float>(j*3) = MAX_WEIGHT;
			else MeasureNoiyVecs[i].at<float>(j*3) = 1.0/probX;
			probY = normal_pdf(PredictStateVector.joints[j].Y, obvY, trackingState, j, 0);
			if (probY < prob_th) MeasureNoiyVecs[i].at<float>(j*3+1) = MAX_WEIGHT;
			else MeasureNoiyVecs[i].at<float>(j*3+1) = 1.0/probY;
			probZ = normal_pdf(PredictStateVector.joints[j].Z, obvZ, trackingState, j, 0);
			if (probZ < prob_th) MeasureNoiyVecs[i].at<float>(j*3+2) = MAX_WEIGHT;
			else MeasureNoiyVecs[i].at<float>(j*3+2) = 1.0/probZ;

			if(obvZ == 0.0){
				MeasureNoiyVecs[i].at<float>(j*3) = MAX_WEIGHT;
				MeasureNoiyVecs[i].at<float>(j*3+1) = MAX_WEIGHT;
				MeasureNoiyVecs[i].at<float>(j*3+2) = MAX_WEIGHT;
			}
		}

/*		if(!CheckBodyConstraints1(syncSkel, i)) {
			if( 1.0/MeasureNoiyVecs[i].at<float>(12*3) < thresh) MeasureNoiyVecs[i].at<float>(12*3) = MAX_WEIGHT; 
			if( 1.0/MeasureNoiyVecs[i].at<float>(12*3+1) < thresh) MeasureNoiyVecs[i].at<float>(12*3+1) = MAX_WEIGHT; 
			if( 1.0/MeasureNoiyVecs[i].at<float>(12*3+2) < thresh) MeasureNoiyVecs[i].at<float>(12*3+2) = MAX_WEIGHT;
			if( 1.0/MeasureNoiyVecs[i].at<float>(15*3) < thresh) MeasureNoiyVecs[i].at<float>(15*3) = MAX_WEIGHT; 
			if( 1.0/MeasureNoiyVecs[i].at<float>(15*3+1) < thresh) MeasureNoiyVecs[i].at<float>(15*3+1) = MAX_WEIGHT; 
			if( 1.0/MeasureNoiyVecs[i].at<float>(12*3+2) < thresh) MeasureNoiyVecs[i].at<float>(15*3+2) = MAX_WEIGHT;
		}
		if(!CheckBodyConstraints2(syncSkel, i)) {
			if( 1.0/MeasureNoiyVecs[i].at<float>(12*3) < thresh) MeasureNoiyVecs[i].at<float>(12*3) = MAX_WEIGHT; 
			if( 1.0/MeasureNoiyVecs[i].at<float>(12*3+1) < thresh) MeasureNoiyVecs[i].at<float>(12*3+1) = MAX_WEIGHT; 
			if( 1.0/MeasureNoiyVecs[i].at<float>(12*3+2) < thresh) MeasureNoiyVecs[i].at<float>(12*3+2) = MAX_WEIGHT;
			if( 1.0/MeasureNoiyVecs[i].at<float>(15*3) < thresh) MeasureNoiyVecs[i].at<float>(15*3) = MAX_WEIGHT; 
			if( 1.0/MeasureNoiyVecs[i].at<float>(15*3+1) < thresh) MeasureNoiyVecs[i].at<float>(15*3+1) = MAX_WEIGHT; 
			if( 1.0/MeasureNoiyVecs[i].at<float>(12*3+2) < thresh) MeasureNoiyVecs[i].at<float>(15*3+2) = MAX_WEIGHT;
		}*/
		if(!CheckBodyConstraints3(syncSkel, i, 5, 6, 9, 10)) {
			if( 1.0/MeasureNoiyVecs[i].at<float>(9*3) < thresh) MeasureNoiyVecs[i].at<float>(9*3) = MAX_WEIGHT;	
			if( 1.0/MeasureNoiyVecs[i].at<float>(9*3+1) < thresh) MeasureNoiyVecs[i].at<float>(9*3+1) = MAX_WEIGHT; 
			if( 1.0/MeasureNoiyVecs[i].at<float>(9*3+2) < thresh) MeasureNoiyVecs[i].at<float>(9*3+2) = MAX_WEIGHT;
			if( 1.0/MeasureNoiyVecs[i].at<float>(10*3) < thresh) MeasureNoiyVecs[i].at<float>(10*3) = MAX_WEIGHT; 
			if( 1.0/MeasureNoiyVecs[i].at<float>(10*3+1) < thresh) MeasureNoiyVecs[i].at<float>(10*3+1) = MAX_WEIGHT; 
			if( 1.0/MeasureNoiyVecs[i].at<float>(10*3+2) < thresh) MeasureNoiyVecs[i].at<float>(10*3+2) = MAX_WEIGHT;
		}
/*		if(!CheckBodyConstraints3(syncSkelAll->syncSkels[0], i, 8, 9, 5, 6)) {
			if( 1.0/MeasureNoiyVecs[i].at<float>(5*3) < 3.7) MeasureNoiyVecs[i].at<float>(9*3) = MAX_WEIGHT;	
			if( 1.0/MeasureNoiyVecs[i].at<float>(5*3+1) < 3.7) MeasureNoiyVecs[i].at<float>(9*3+1) = MAX_WEIGHT; 
			if( 1.0/MeasureNoiyVecs[i].at<float>(5*3+2) < 3.7) MeasureNoiyVecs[i].at<float>(9*3+2) = MAX_WEIGHT;
			if( 1.0/MeasureNoiyVecs[i].at<float>(6*3) < 3.7) MeasureNoiyVecs[i].at<float>(10*3) = MAX_WEIGHT; 
			if( 1.0/MeasureNoiyVecs[i].at<float>(6*3+1) < 3.7) MeasureNoiyVecs[i].at<float>(10*3+1) = MAX_WEIGHT; 
			if( 1.0/MeasureNoiyVecs[i].at<float>(6*3+2) < 3.7) MeasureNoiyVecs[i].at<float>(10*3+2) = MAX_WEIGHT;
		}*/
		if(!CheckBodyConstraints3(syncSkel, i, 9, 10, 5, 6)) {
			if( 1.0/MeasureNoiyVecs[i].at<float>(5*3) < thresh) MeasureNoiyVecs[i].at<float>(5*3) = MAX_WEIGHT; 
			if( 1.0/MeasureNoiyVecs[i].at<float>(5*3+1) < thresh) MeasureNoiyVecs[i].at<float>(5*3+1) = MAX_WEIGHT; 
			if( 1.0/MeasureNoiyVecs[i].at<float>(5*3+2) < thresh) MeasureNoiyVecs[i].at<float>(5*3+2) = MAX_WEIGHT;
			if( 1.0/MeasureNoiyVecs[i].at<float>(6*3) < thresh) MeasureNoiyVecs[i].at<float>(6*3) = MAX_WEIGHT; 
			if( 1.0/MeasureNoiyVecs[i].at<float>(6*3+1) < thresh) MeasureNoiyVecs[i].at<float>(6*3+1) = MAX_WEIGHT; 
			if( 1.0/MeasureNoiyVecs[i].at<float>(6*3+2) < thresh) MeasureNoiyVecs[i].at<float>(6*3+2) = MAX_WEIGHT;
		}
/*		if(!CheckBodyConstraints3(syncSkelAll->syncSkels[0], i, 4, 5, 9, 10)) {
			if( 1.0/MeasureNoiyVecs[i].at<float>(9*3) < 3.7) MeasureNoiyVecs[i].at<float>(5*3) = MAX_WEIGHT; 
			if( 1.0/MeasureNoiyVecs[i].at<float>(9*3+1) < 3.7) MeasureNoiyVecs[i].at<float>(5*3+1) = MAX_WEIGHT; 
			if( 1.0/MeasureNoiyVecs[i].at<float>(9*3+2) < 3.7) MeasureNoiyVecs[i].at<float>(5*3+2) = MAX_WEIGHT;
			if( 1.0/MeasureNoiyVecs[i].at<float>(10*3) < 3.7) MeasureNoiyVecs[i].at<float>(6*3) = MAX_WEIGHT; 
			if( 1.0/MeasureNoiyVecs[i].at<float>(10*3+1) < 3.7) MeasureNoiyVecs[i].at<float>(6*3+1) = MAX_WEIGHT; 
			if( 1.0/MeasureNoiyVecs[i].at<float>(10*3+2) < 3.7) MeasureNoiyVecs[i].at<float>(6*3+2) = MAX_WEIGHT;
		}*/
		if(!CheckBodyConstraints3(syncSkel, i, 13, 14, 17, 18)) {
			if( 1.0/MeasureNoiyVecs[i].at<float>(16*3) < thresh) MeasureNoiyVecs[i].at<float>(16*3) = MAX_WEIGHT; 
			if( 1.0/MeasureNoiyVecs[i].at<float>(16*3+1) < thresh) MeasureNoiyVecs[i].at<float>(16*3+1) = MAX_WEIGHT; 
			if( 1.0/MeasureNoiyVecs[i].at<float>(16*3+2) < thresh) MeasureNoiyVecs[i].at<float>(16*3+2) = MAX_WEIGHT;
			if( 1.0/MeasureNoiyVecs[i].at<float>(17*3) < thresh) MeasureNoiyVecs[i].at<float>(17*3) = MAX_WEIGHT; 
			if( 1.0/MeasureNoiyVecs[i].at<float>(17*3+1) < thresh) MeasureNoiyVecs[i].at<float>(17*3+1) = MAX_WEIGHT; 
			if( 1.0/MeasureNoiyVecs[i].at<float>(17*3+2) < thresh) MeasureNoiyVecs[i].at<float>(17*3+2) = MAX_WEIGHT;
		}
		if(!CheckBodyConstraints3(syncSkel, i, 16, 17, 13, 14)) {
			if( 1.0/MeasureNoiyVecs[i].at<float>(13*3) < thresh) MeasureNoiyVecs[i].at<float>(13*3) = MAX_WEIGHT; 
			if( 1.0/MeasureNoiyVecs[i].at<float>(13*3+1) < thresh) MeasureNoiyVecs[i].at<float>(13*3+1) = MAX_WEIGHT; 
			if( 1.0/MeasureNoiyVecs[i].at<float>(13*3+2) < thresh) MeasureNoiyVecs[i].at<float>(13*3+2) = MAX_WEIGHT;
			if( 1.0/MeasureNoiyVecs[i].at<float>(14*3) < thresh) MeasureNoiyVecs[i].at<float>(14*3) = MAX_WEIGHT; 
			if( 1.0/MeasureNoiyVecs[i].at<float>(14*3+1) < thresh) MeasureNoiyVecs[i].at<float>(14*3+1) = MAX_WEIGHT; 
			if( 1.0/MeasureNoiyVecs[i].at<float>(14*3+2)< thresh) MeasureNoiyVecs[i].at<float>(14*3+2) = MAX_WEIGHT;
		}
/*		if(!CheckBodyConstraints4(syncSkel, i)) {
			if( 1.0/MeasureNoiyVecs[i].at<float>(4*3) < thresh) MeasureNoiyVecs[i].at<float>(4*3) = MAX_WEIGHT; 
			if( 1.0/MeasureNoiyVecs[i].at<float>(4*3+1) < thresh) MeasureNoiyVecs[i].at<float>(4*3+1) = MAX_WEIGHT; 
			if( 1.0/MeasureNoiyVecs[i].at<float>(4*3+2) < thresh) MeasureNoiyVecs[i].at<float>(4*3+2) = MAX_WEIGHT;
			if( 1.0/MeasureNoiyVecs[i].at<float>(8*3) < thresh) MeasureNoiyVecs[i].at<float>(8*3) = MAX_WEIGHT; 
			if( 1.0/MeasureNoiyVecs[i].at<float>(8*3+1) < thresh) MeasureNoiyVecs[i].at<float>(8*3+1) = MAX_WEIGHT; 
			if( 1.0/MeasureNoiyVecs[i].at<float>(8*3+2) < thresh) MeasureNoiyVecs[i].at<float>(8*3+2) = MAX_WEIGHT;
		}*/

		for(j = 0; j < STATE_DIM; j++){
			for(k = 0; k < STATE_DIM; k++){
				if (j == k)
					MeasureNoiyCovs[i].at<float>(j*STATE_DIM+k) = MeasureNoiyVecs[i].at<float>(j);
			}
		}
//		MeasureNoiyCovs[i] = MeasureNoiyVecs[i] * MeasureNoiyVecs[i].t();
/*		if ( i == 1) {
			MeasureFusedCov2 = MeasureFusedCov.inv(DECOMP_SVD);
			val = MeasureFusedCov.at<float>(17*3*STATE_DIM+17*3);
			val = MeasureFusedCov2.at<float>(17*3*STATE_DIM+17*3);
				MeasureFusedVec2 = MeasureFusedCov2*MeasureFusedVec;
			val = MeasureFusedVec.at<float>(17*3);
			val = MeasureFusedVec2.at<float>(17*3);
			
		}*/
		MeasureFusedVec +=  MeasureNoiyCovs[i].inv(DECOMP_SVD) * MeasureVecs[i];
//		val = MeasureVecs[i].at<float>(17*3);
		MeasureFusedCov += MeasureNoiyCovs[i].inv(DECOMP_SVD);
//		val = MeasureFusedCov.at<float>(17*3*STATE_DIM+17*3);
		TmpMeasureMatrix += MeasureNoiyCovs[i].inv(DECOMP_SVD) * MeasureMatrix;

/*		val = MeasureFusedVec.at<float>(17*3);
		val = MeasureFusedVec.at<float>(17*3+1);
		val = MeasureFusedVec.at<float>(17*3+2);

		val = MeasureNoiyCovs[i].at<float>(17*3*STATE_DIM+17*3);
		val = MeasureNoiyCovs[i].at<float>(17*3*STATE_DIM+STATE_DIM+17*3+1);
		val = MeasureNoiyCovs[i].at<float>(17*3*STATE_DIM+STATE_DIM*2+17*3+2);*/
	}
	MeasureFusedCov = MeasureFusedCov.inv(DECOMP_SVD);
	MeasureFusedVec = MeasureFusedCov*MeasureFusedVec;
	MeasureMatrix = MeasureFusedCov*TmpMeasureMatrix;

/*	val = MeasureFusedVec2.at<float>(17*3);
	val = MeasureFusedVec2.at<float>(17*3+1);
	val = MeasureFusedVec2.at<float>(17*3+2);
*/
	for(i = 0; i < STATE_DIM; i++){
		m_Measurement->data.fl[i] = MeasureFusedVec.at<float>(i);
		for(int j = 0; j < STATE_DIM; j++){
			m_H->data.fl[i*STATE_DIM+j] = MeasureMatrix.at<float>(i*STATE_DIM+j);
			m_R->data.fl[i*STATE_DIM+j] = MeasureFusedCov.at<float>(i*STATE_DIM+j);
		}
	}

	for(i = 0; i < STATE_DIM; i++) {
		for(j = 0; j < STATE_DIM; j++){
			if (i >= 0*3 && i < 0*3+3 && i == j) {
				val = MeasureFusedCov.at<float>(i*STATE_DIM+j);
			}
		}
	}
	for(i = 0; i < 19*3; i++) {
		if (i >= 10*3 && i < 10*3+3) {
			val = m_Measurement->data.fl[i];
			val2 = MeasureNoiyVecs[0].at<float>(i);
			val2 = MeasureNoiyVecs[1].at<float>(i);
			val2 = MeasureNoiyVecs[2].at<float>(i);
			val2 = MeasureNoiyVecs[3].at<float>(i);
			val2 = MeasureNoiyVecs[4].at<float>(i);
		}
	}

	memcpy(kalman->measurement_matrix->data.fl, m_H->data.fl, sizeof(float)*STATE_DIM*STATE_DIM);
	memcpy(kalman->measurement_noise_cov->data.fl, m_R->data.fl, sizeof(float)*STATE_DIM*STATE_DIM);


	MeasureFusedVec.release();
	MeasureFusedCov.release();
	MeasureMatrix.release();
	for(i=0; i < KINECT_COUNT; i++) {
		MeasureNoiyVecs[i].release();
		MeasureNoiyCovs[i].release();
		MeasureVecs[i].release();
	}
}

void KF::preiction( double sampling_interval)
{
	if (frameNum < 3) {
		memset(m_controlVec->data.fl, 0x00, sizeof(float)*STATE_DIM);
	}else {

		for(int i = 0; i < ACTUAL_JOINT_COUNT; i++) {
			cvSetReal1D(m_controlVec, i*3, m_lastVelVector.Jointvels[i].dx);  
			cvSetReal1D(m_controlVec, i*3+1, m_lastVelVector.Jointvels[i].dy);  
			cvSetReal1D(m_controlVec, i*3+2, m_lastVelVector.Jointvels[i].dz);  
		}
	}
//	memset(m_controlVec->data.fl, 0x00, sizeof(float)*STATE_DIM);
//	memset(m_controlVec->data.fl, 0x00, sizeof(float)*STATE_DIM);
	cvSetIdentity(m_G, cvRealScalar(1.0*sampling_interval));	
	memcpy(kalman->control_matrix->data.fl,  m_G->data.fl, sizeof(float)*STATE_DIM*STATE_DIM);

	Mat prediction = cvKalmanPredict(kalman, m_controlVec);	

	for(int i = 0; i < ACTUAL_JOINT_COUNT; i++) {
		m_PredictionState.joints[i].X = prediction.at<float>(i*3);
		m_PredictionState.joints[i].Y = prediction.at<float>(i*3+1); 
		m_PredictionState.joints[i].Z = prediction.at<float>(i*3+2);
	}
}

void KF::update()
{
	Mat update = cvKalmanCorrect(kalman, m_Measurement);

	float val, val1, val2;
	for(int i = 0; i < STATE_DIM; i++) {

		if (i >= 8*3 && i < 8*3+3) {
				int aaa = 0;
		}

//		val = m_controlVec->data.fl[i];
//		val = m_lastVelVector.Jointvels[0].dx;
		val = kalman->state_pre->data.fl[i];
		val1 = m_Measurement->data.fl[i];
		val2 = kalman->state_post->data.fl[i];

		//val = kalman->->data.fl[i];
		for(int j = 0;j < STATE_DIM; j++) {

			if (i >= 15*3 && i < 15*3+3 && i == j) {
					int aaa = 0;
			}
			if ( i == j)
				int aaaaa = 0;

			val = kalman->error_cov_post->data.fl[i*STATE_DIM+j];
			//val = kalman->measurement_noise_cov->data.fl[i*STATE_DIM+j];
			//val = kalman->measurement_matrix->data.fl[i*STATE_DIM+j];
		}
	}

	for(int i = 0; i < ACTUAL_JOINT_COUNT; i++) {
		m_StateVector.joints[i].X = update.at<float>(i*3);
		m_StateVector.joints[i].Y = update.at<float>(i*3+1);  
		m_StateVector.joints[i].Z = update.at<float>(i*3+2);
	}
}

void KF::MeasurementFusion2(synconizedSkeletons* syncSkel)
{
	int i, j, k;
	float val;
	cv::Mat MeasureNoiyVecs[KINECT_COUNT];
	cv::Mat MeasureNoiyCovs[KINECT_COUNT];
	cv::Mat MeasureVecs[KINECT_COUNT];
	cv::Mat MeasureFusedVec;
	cv::Mat MeasureFusedCov;
	cv::Mat MeasureMatrix;
	cv::Mat TmpMeasureMatrix;

/*	cv::Mat MeasureFusedVec2;
	MeasureFusedVec2.create(STATE_DIM, 1, CV_32FC1);
	cv::Mat MeasureFusedCov2;;
	MeasureFusedCov2.create(STATE_DIM, STATE_DIM, CV_32FC1);
*/
	MeasureFusedVec.create(STATE_DIM, 1, CV_32FC1);
	MeasureFusedCov.create(STATE_DIM, STATE_DIM, CV_32FC1);
	MeasureMatrix.create(STATE_DIM, STATE_DIM, CV_32FC1);
	TmpMeasureMatrix.create(STATE_DIM, STATE_DIM, CV_32FC1);

	_JointStatus jointStatus;
	TrackingState trackingState;
	
	MeasureMatrix.setTo(cv::Scalar(0.0));
	MeasureMatrix.diag().setTo(1.0); 
	MeasureFusedVec.setTo(cv::Scalar(0.0));
	MeasureFusedCov.setTo(cv::Scalar(0.0));
	TmpMeasureMatrix.setTo(cv::Scalar(0.0));

	int index[19];
	GetActualJointIdx(index);

	for(i=0; i < KINECT_COUNT; i++) {
		MeasureNoiyVecs[i].create(STATE_DIM, 1, CV_32FC1);
		MeasureNoiyVecs[i].setTo(cv::Scalar(0.0));
		MeasureNoiyCovs[i].create(STATE_DIM, STATE_DIM, CV_32FC1);
		MeasureNoiyCovs[i].setTo(cv::Scalar(0.0));
		MeasureVecs[i].create(STATE_DIM, 1, CV_32FC1);
		MeasureVecs[i].setTo(cv::Scalar(0.0));

		for(j=0; j < Actual_JointType_Count; j++) {

			trackingState = syncSkel->InfoBody[i].JointPos[index[j]].TrackingState;

			if ( index[j] == 7)			
				int aaa = 0;

			MeasureVecs[i].at<float>(j*3) = syncSkel->InfoBody[i].JointPos[index[j]].Position.X;
			MeasureVecs[i].at<float>(j*3+1) = syncSkel->InfoBody[i].JointPos[index[j]].Position.Y;
			MeasureVecs[i].at<float>(j*3+2) = syncSkel->InfoBody[i].JointPos[index[j]].Position.Z;
			if (trackingState == TrackingState_Tracked && m_JointsConfidennce[i].confidence[j]) {
				jointStatus = ConfidenceOccluding;
				MeasureNoiyVecs[i].at<float>(j*3) = cvGetReal1D(m_CovMatReader.PredVec,0); 
				MeasureNoiyVecs[i].at<float>(j*3+1)= cvGetReal1D(m_CovMatReader.PredVec,1); 
				MeasureNoiyVecs[i].at<float>(j*3+2)= cvGetReal1D(m_CovMatReader.PredVec,2); 
			}else if (trackingState == TrackingState_Tracked && !m_JointsConfidennce[i].confidence[j]) {
				jointStatus = NotConfidenceOccluding;
				MeasureNoiyVecs[i].at<float>(j*3) = cvGetReal1D(m_CovMatReader.NotConfidenceNormalVec,0); 
				MeasureNoiyVecs[i].at<float>(j*3+1)= cvGetReal1D(m_CovMatReader.NotConfidenceNormalVec,1); 
				MeasureNoiyVecs[i].at<float>(j*3+2)= cvGetReal1D(m_CovMatReader.NotConfidenceNormalVec,2); 
			}else if (trackingState == TrackingState_Inferred && m_JointsConfidennce[i].confidence[j]) {
				jointStatus = ConfidenceOccluded;
				MeasureNoiyVecs[i].at<float>(j*3) = cvGetReal1D(m_CovMatReader.PredVec,0); 
				MeasureNoiyVecs[i].at<float>(j*3+1)= cvGetReal1D(m_CovMatReader.PredVec,1); 
				MeasureNoiyVecs[i].at<float>(j*3+2)= cvGetReal1D(m_CovMatReader.PredVec,2); 
			}else if (trackingState == TrackingState_Inferred && !m_JointsConfidennce[i].confidence[j]) {
				jointStatus = NotConfidenceOccluded;
				MeasureNoiyVecs[i].at<float>(j*3) = cvGetReal1D(m_CovMatReader.NotConfidenceNormalVec,0); 
				MeasureNoiyVecs[i].at<float>(j*3+1)= cvGetReal1D(m_CovMatReader.NotConfidenceNormalVec,1); 
				MeasureNoiyVecs[i].at<float>(j*3+2)= cvGetReal1D(m_CovMatReader.NotConfidenceNormalVec,2); 
			}else if (trackingState == TrackingState_NotTracked && m_JointsConfidennce[i].confidence[j]) {
				jointStatus = ConfidenceNormal;
				MeasureNoiyVecs[i].at<float>(j*3) = cvGetReal1D(m_CovMatReader.PredVec,0); 
				MeasureNoiyVecs[i].at<float>(j*3+1)= cvGetReal1D(m_CovMatReader.PredVec,1); 
				MeasureNoiyVecs[i].at<float>(j*3+2)= cvGetReal1D(m_CovMatReader.PredVec,2); 
			}else if (trackingState == TrackingState_NotTracked && !m_JointsConfidennce[i].confidence[j]) {
				jointStatus = NotConfidenceNormal;
				MeasureNoiyVecs[i].at<float>(j*3) = m_CovMatReader.NotConfidenceNormalVec->data.fl[0]; 
				MeasureNoiyVecs[i].at<float>(j*3+1) = m_CovMatReader.NotConfidenceNormalVec->data.fl[1]; 
				MeasureNoiyVecs[i].at<float>(j*3+2)= m_CovMatReader.NotConfidenceNormalVec->data.fl[2];
			}
		}
		for(j = 0; j < STATE_DIM; j++){
			for(k = 0; k < STATE_DIM; k++){
				if (j == k) {
					MeasureNoiyCovs[i].at<float>(j*STATE_DIM+k) = MeasureNoiyVecs[i].at<float>(j);
					val = MeasureNoiyCovs[i].at<float>(j*STATE_DIM+k);
				}
			}
		}
//		MeasureNoiyCovs[i] = MeasureNoiyVecs[i] * MeasureNoiyVecs[i].t();
		MeasureFusedVec +=  MeasureNoiyCovs[i].inv(DECOMP_SVD) * MeasureVecs[i];
		MeasureFusedCov += MeasureNoiyCovs[i].inv(DECOMP_SVD);
		TmpMeasureMatrix += MeasureNoiyCovs[i].inv(DECOMP_SVD) * MeasureMatrix;
	}
	MeasureFusedCov = MeasureFusedCov.inv(DECOMP_SVD);
	MeasureFusedVec = MeasureFusedCov*MeasureFusedVec;
	MeasureMatrix = MeasureFusedCov*TmpMeasureMatrix;

	for(i = 0; i < STATE_DIM; i++){
		m_Measurement->data.fl[i] = MeasureFusedVec.at<float>(i);
		val = m_Measurement->data.fl[i];
		for(int j = 0; j < STATE_DIM; j++){
			m_H->data.fl[i*STATE_DIM+j] = MeasureMatrix.at<float>(i*STATE_DIM+j);
			m_R->data.fl[i*STATE_DIM+j] = MeasureFusedCov.at<float>(i*STATE_DIM+j);
		}
	}

	for(i = 0; i < 19*3; i++) {
		if (i >= 7*3 && i < 7*3+3) {
			val = m_Measurement->data.fl[i];
			val = MeasureNoiyVecs[0].at<float>(i);
		}
	}

	memcpy(kalman->measurement_matrix->data.fl, m_H->data.fl, sizeof(float)*STATE_DIM*STATE_DIM);
	memcpy(kalman->measurement_noise_cov->data.fl, m_R->data.fl, sizeof(float)*STATE_DIM*STATE_DIM);


	MeasureFusedVec.release();
	MeasureFusedCov.release();
	MeasureMatrix.release();
	for(i=0; i < KINECT_COUNT; i++) {
		MeasureNoiyVecs[i].release();
		MeasureNoiyCovs[i].release();
		MeasureVecs[i].release();
	}
}

/*		if(!fastMotion) {
			for (j=0; j < KINECT_COUNT; j++) {
				if (syncSkels->bObsevingKinect[j] && m_lastSyncSkels->bObsevingKinect[j]) {
					if(bCheckDiscontinuity[j]) {
						dist = fabs(PredictStateVector.joints[i].X - syncSkels->InfoBody[j].JointPos[i].Position.X);
						if (dist > thresh2) {
							syncSkels->InfoBody[j].JointPos[i].Position.X = PredictStateVector.joints[i].X; 
						}
						dist = fabs(PredictStateVector.joints[i].Y - syncSkels->InfoBody[j].JointPos[i].Position.Y);
						if (dist > thresh2) {
							syncSkels->InfoBody[j].JointPos[i].Position.Y = PredictStateVector.joints[i].Y; 
						}
						dist = fabs(PredictStateVector.joints[i].Z - syncSkels->InfoBody[j].JointPos[i].Position.Z);
						if (dist > thresh2) {
							syncSkels->InfoBody[j].JointPos[i].Position.Z = PredictStateVector.joints[i].Z; 
						}
					}
				}
			}
		}
*/
/*
				dist = fabs(m_lastSyncSkels->InfoBody[j].JointPos[i].Position.Y - syncSkels->InfoBody[j].JointPos[i].Position.Y);
				if (dist > thresh1) {
					bCheckDiscontinuity[j] = true;
				}
				dist = fabs(m_lastSyncSkels->InfoBody[j].JointPos[i].Position.Z - syncSkels->InfoBody[j].JointPos[i].Position.Z);
				if (dist > thresh1) {
					bCheckDiscontinuity[j] = true;
				}
*/
/*					dist = fabs(PredictStateVector.joints[i].X - syncSkels->InfoBody[j].JointPos[i].Position.X);
					if (dist > thresh2) {
						syncSkels->InfoBody[j].JointPos[i].Position.X = PredictStateVector.joints[i].X; 
					}
					dist = fabs(PredictStateVector.joints[i].Y - syncSkels->InfoBody[j].JointPos[i].Position.Y);
					if (dist > thresh2) {
						syncSkels->InfoBody[j].JointPos[i].Position.Y = PredictStateVector.joints[i].Y; 
					}
					dist = fabs(PredictStateVector.joints[i].Z - syncSkels->InfoBody[j].JointPos[i].Position.Z);
					if (dist > thresh2) {
						syncSkels->InfoBody[j].JointPos[i].Position.Z = PredictStateVector.joints[i].Z; 
					}
*/
/*	for (j=0; j < KINECT_COUNT; j++) {
		if (syncSkels->bObsevingKinect[j] && m_lastSyncSkels->bObsevingKinect[j]) {
			dist = pow(syncSkels->InfoBody[j].JointPos[5].Position.X-syncSkels->InfoBody[j].JointPos[6].Position.X,2);
			dist = dist + pow(syncSkels->InfoBody[j].JointPos[5].Position.Y-syncSkels->InfoBody[j].JointPos[6].Position.Y,2);
			dist = dist + pow(syncSkels->InfoBody[j].JointPos[5].Position.Z-syncSkels->InfoBody[j].JointPos[6].Position.Z,2);
			dist = sqrt(dist);

			dist = pow(m_lastSyncSkels->InfoBody[j].JointPos[5].Position.X-m_lastSyncSkels->InfoBody[j].JointPos[6].Position.X,2);
			dist = dist + pow(m_lastSyncSkels->InfoBody[j].JointPos[5].Position.Y-m_lastSyncSkels->InfoBody[j].JointPos[6].Position.Y,2);
			dist = dist + pow(m_lastSyncSkels->InfoBody[j].JointPos[5].Position.Z-m_lastSyncSkels->InfoBody[j].JointPos[6].Position.Z,2);
			dist = sqrt(dist);
		}
	}
*/
