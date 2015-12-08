#include "stdafx.h"
#include "CovMatReader.h"


CovMatReader::CovMatReader(void)
{

}


void CovMatReader::Initialize()
{
	OccludedVec = cvCreateMat(3, 1, CV_32FC1); 
	memset(OccludedVec->data.fl, 0x00, sizeof(float)*3);
	OccludingVec = cvCreateMat(3, 1, CV_32FC1); 
	memset(OccludingVec->data.fl, 0x00, sizeof(float)*3);
	OccludedCov = cvCreateMat(19, 19, CV_32FC1);
	OccludingCov = cvCreateMat(19, 19, CV_32FC1);
	PredVec = cvCreateMat(3, 1, CV_32FC1); 
	memset(PredVec->data.fl, 0x00, sizeof(float)*3);
	PredCov = cvCreateMat(19, 19, CV_32FC1);

	NotConfidenceOccludedVec = cvCreateMat(3, 1, CV_32FC1); 
	memset(NotConfidenceOccludedVec->data.fl, 0x00, sizeof(float)*3);
	NotConfidenceOccludingVec = cvCreateMat(3, 1, CV_32FC1); 
	memset(NotConfidenceOccludingVec->data.fl, 0x00, sizeof(float)*3);
	NotConfidenceNormalVec = cvCreateMat(3, 1, CV_32FC1); 
	memset(NotConfidenceNormalVec->data.fl, 0x00, sizeof(float)*3);

}

CovMatReader::~CovMatReader(void)
{
/*	cvRelease(OccludedVec);
	OccludedVec.release();
	OccludingVec.release();
	OccludedCov.release();
	OccludingCov.release();
	PredVec.release();
	PredCov.release();*/
}

void CovMatReader::CovDiagonalize(){
	for(int i = 0; i < 19*3; i++){
		for(int j = 0; j < 19*3; j++){
			if(i == j) continue;

			cvSetReal2D(OccludedCov,i,j, 0.0);
			cvSetReal2D(OccludingCov,i,j, 0.0);
			cvSetReal2D(PredCov,i,j, 0.0);
			// CvMat test(OccludedCov);
			// OccludedCov = test;
		}
	}
}

void CovMatReader::ReadOccludingVec(char *filePath){
	FILE *fp = fopen(filePath, "r");
	for(int i = 0; i < 3; i++){
		fscanf(fp, "%f\n", OccludingVec->data.fl + i);
		cvSetReal1D(NotConfidenceOccludingVec, i, OccludingVec->data.fl[i] * 500);
		//OccludingVec->data.fl[i] = OccludingVec->data.fl[i]*10;
		printf("%f\n", cvGetReal1D(OccludingVec,i));
	}
	fclose(fp);

/*	Mat temp = Mat_<float>(19*3,1);
	for(int i = 0; i < 19*3; i++)
		temp.at<float>(i) = cvGetReal1D(OccludingVec,i%3);
		*/
/*	Mat tempCov = Mat_<float>(19,19);
	tempCov = temp * temp.t();
	CvMat cvMat1 = tempCov;
	OccludingCov = &cvMat1;*/
}

void CovMatReader::ReadOccludedVec(char *filePath){
	FILE *fp = fopen(filePath, "r");
	for(int i = 0; i < 3; i++){
		fscanf(fp, "%f\n", OccludedVec->data.fl + i);
		cvSetReal1D(NotConfidenceOccludedVec, i, OccludedVec->data.fl[i] * 500);
		//OccludedVec->data.fl[i] = OccludedVec->data.fl[i];
		printf("%f\n", cvGetReal1D(OccludedVec,i));
	}
	fclose(fp);

/*	Mat temp = Mat_<float>(19*3,1);
	for(int i = 0; i < 19*3; i++){
		temp.at<float>(i) = cvGetReal1D(OccludedVec,i%3);
	}*/

/*	temp = temp * temp.t();
	CvMat cvMat1 = temp;
	OccludedCov = &cvMat1;*/
}

void CovMatReader::ReadPredctionVec(char *filePath){
	FILE *fp = fopen(filePath, "r");
	for(int i = 0; i < 3; i++){
		fscanf(fp, "%f\n", PredVec->data.fl + i);
		NotConfidenceNormalVec->data.fl[i] = PredVec->data.fl[i] * 500;
		//PredVec->data.fl[i] = PredVec->data.fl[i]*10;
		//cvSetReal1D(NotConfidenceNormalVec, i, (double)*PredVec->data.fl * 10);
		printf("%f\n", cvGetReal1D(PredVec,i));
	}
	fclose(fp);

/*	Mat temp = Mat_<float>(19*3,1);
	for(int i = 0; i < 19*3; i++){
		temp.at<float>(i) = cvGetReal1D(PredVec,i%3);
	}*/

/*	temp = temp * temp.t();
	CvMat cvMat1 = temp;
	PredCov = &cvMat1;*/
}