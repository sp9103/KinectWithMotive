#pragma once

#include "engine.h"
#include "mat.h"

class MATLABConn
{
public:
	MATLABConn(void);
	~MATLABConn(void);

	bool initialize();
	double* LPFiteringNoisyVel(double* noisyVel, double dataLen, double dim);
	double* ReadMatFile(const char *file);
	bool WriteMatFile(const char *file, double* filterdNoisyVel);

	Engine *matlab;
	int m_nDatalength;
};

