#include <opencv.hpp>

using namespace cv;

class CovMatReader
{
public:
	CovMatReader(void);
	~CovMatReader(void);

	CvMat* OccludedVec;
	CvMat* OccludingVec;
	CvMat* PredVec;
	CvMat* NotConfidenceOccludedVec;
	CvMat* NotConfidenceOccludingVec;
	CvMat* NotConfidenceNormalVec;
	CvMat* OccludedCov;
	CvMat* OccludingCov;
	CvMat* PredCov;

	void CovDiagonalize();
	void Initialize();

	void ReadOccludingVec(char *filePath);
	void ReadOccludedVec(char *filePath);
	void ReadPredctionVec(char *filePath);

};

