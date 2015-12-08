#include "stdafx.h"
#include "MATLABConn.h"

#define BUFSIZE 1000
MATLABConn::MATLABConn(void)
{

}


MATLABConn::~MATLABConn(void)
{
	engClose(matlab);
}

bool MATLABConn::initialize()
{
	matlab = NULL;
	matlab = engOpen(NULL);
	
	if ( matlab == NULL)
		return false;

	return true;
}

double* MATLABConn::LPFiteringNoisyVel(double* noisyVel, double dataLen, double dim)
{
	// 2.1. Pre-work: capture MATLAB output. Ensure the buffer is always NULL terminated.
	char buffer[BUFSIZE+1];
	buffer[BUFSIZE] = '\0';
	engOutputBuffer(matlab, buffer, BUFSIZE);
	engEvalString(matlab, "clc;clear;"); // clear all variables (optional)

	m_nDatalength = dataLen*dim;
	
//	double* Vel = new double[57*20];
//	memset(Vel, 0x00, sizeof(double)*57*20);
//	WriteMatFile("matlabtest.mat", noisyVel);
/*	// 2.2. Setup inputs: a, b
	mxArray *velHatN = mxCreateDoubleMatrix(1,dataLen,mxREAL); 

	engPutVariable(matlab, "velHatN", velHatN); // put into matlab

	// 2.3. Call MATLAB
	engEvalString(matlab, "cd \'C:\\'");  
	engEvalString(matlab, "[velHatNF] = filtered_velocity_cal(velHatN);");
	printf("%s\n", buffer); // get error messages or prints (optional)

	// 2.4. Get result:
	mxArray *velHatNF = engGetVariable(matlab, "velHatNF");
	double *pVal = (double*)mxGetPr(velHatNF);
	mwSize nRow = mxGetM(velHatNF);
	mwSize nCol = mxGetN(velHatNF);
*/
	// 2.2. Setup inputs: a, b
	mwSize dims[] = {1, dataLen*dim};
	mxArray *velN = mxCreateNumericArray(2,dims,mxDOUBLE_CLASS,mxREAL); 
	double *ptr = (double *) mxGetData(velN);
	memcpy(ptr, noisyVel, dataLen*dim*sizeof(double));

	engPutVariable(matlab, "velN", velN); // put into matlab

	// 2.3. Call MATLAB
	engEvalString(matlab, "cd \'C:\\'");  
	engEvalString(matlab, "[velF] = filtered_velocity_cal(velN);");
	printf("%s\n", buffer); // get error messages or prints (optional)

	// 2.4. Get result:
	mxArray *velF = engGetVariable(matlab, "velF");
	double *pVal = (double*)mxGetData(velF);
	mwSize nRow = mxGetM(velF);
	mwSize nCol = mxGetN(velF);

	double *filteredNoisyVel = new double[nCol*nRow];

	for (int i = 0; i < nRow*nCol; i++)
		filteredNoisyVel[i] = *pVal++;

	mxDestroyArray(velF);

	return filteredNoisyVel;

}

double* MATLABConn::ReadMatFile(const char *file)
{
	MATFile *pmat;
	const char *name;
	mxArray *velData;
	int	  i, ndir, ndim;
	const char **dir;
	double* noisyVel = NULL;

	pmat = matOpen(file, "r");
	if (pmat == NULL) {
		printf("Error opening file %s\n", file);
		return noisyVel;
	}

//    pa = matGetNextVariableInfo(pmat, &name);
	velData = matGetVariable(pmat, "velHatN");
    if (velData == NULL) {
		printf("Error reading in file %s\n", file);
		return noisyVel;
	}

//	ndim = mxGetNumberOfDimensions(pa);

	mwSize nRow = mxGetM(velData);
	mwSize nCol = mxGetN(velData);

	m_nDatalength = nCol;

	double *pVal = (double*)mxGetPr(velData);

	noisyVel = new double[nCol*nRow];

	for (i = 0; i < nRow*nCol; i++)
		noisyVel[i] = *pVal++;

	mxDestroyArray(velData);

	if (matClose(pmat) != 0) {
		printf("Error closing file %s\n",file);
		return noisyVel;
	}

	return noisyVel;
}

bool MATLABConn::WriteMatFile(const char *file, double* filterdNoisyVel)
{
	MATFile *pmat;
	mxArray *filterdVel;
	int status; 

	printf("Creating file %s...\n\n", file);
		pmat = matOpen(file, "w");
		if (pmat == NULL) {
		printf("Error creating file %s\n", file);
		printf("(Do you have write permission in this directory?)\n");
		return false;
	}

	filterdVel = mxCreateDoubleMatrix(1,m_nDatalength,mxREAL);
	if (filterdVel == NULL) {
		printf("%s : Out of memory on line %d\n", __FILE__, __LINE__); 
		printf("Unable to create mxArray.\n");
		return false;
	}
	memcpy((void *)(mxGetPr(filterdVel)), (void *)filterdNoisyVel, sizeof(double)*m_nDatalength);

	status = matPutVariable(pmat, "velHatNF2", filterdVel);
	if (status != 0) {
		printf("%s :  Error using matPutVariable on line %d\n", __FILE__, __LINE__);
		return false;
	}  

	/* clean up */
	mxDestroyArray(filterdVel);

	if (matClose(pmat) != 0) {
		printf("Error closing file %s\n",file);
		return false;
	}

	return true;
}