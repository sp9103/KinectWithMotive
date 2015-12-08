#include "define.h"

using namespace cv;

//스켈레톤 그리는 부분의 파라미터
static const float c_JointThickness = 3.0f;
static const float c_TrackedBoneThickness = 6.0f;
static const float c_InferredBoneThickness = 1.0f;
static const float c_HandSize = 30.f;

class KinectConnector
{
public:
	KinectConnector(void);
	~KinectConnector(void);

	//Kinect Initialize
	//set KinectSource_Color bit or
	HRESULT KinectInitialize(char enabledFrameSourceTypes);

	//Get Color Opencv Image. (1920*1080)
	//Image must allocated.(size: 1920*1080, CV_8UC4)
	HRESULT GetColorImage(Mat *src);

	//Get Depth Opencv Image. (512*424)
	//Image must allocated. (size: 512*424, CV_8UC4)
	void GetDepthImage(Mat *src);

	//Get Body joint position. (joint count : 25)
	//and Draw Opencv Image.
	//if mode = 1, Draw DepthScale. so src = DepthImage
	//else if mode = 0, Draw ColorScale. so src = ColorImage.
	HRESULT GetSkeletonPos(SkeletonInfo *m_SkeletonInfo, Mat *src, int mode);

	//Get Face Information & draw Face (RGBD Space)
	HRESULT FaceDetection(SkeletonInfo *m_SkeletonInfo, Mat *src);

	//Kinect sensor close.
	void KinectDestroy();

	//Return Kinect Unique ID
	void GetKinectID(WCHAR *KinectID);

	//Set intrinsic & extrinsic mat
	void SetRTmat(Mat src);

private:
	IKinectSensor*			m_pKinectSensor;
	IColorFrameReader*		m_pColorFrameReader;
	IDepthFrameReader*		m_pDepthFrameReader;
	IBodyFrameReader*		m_pBodyFrameReader;
	ICoordinateMapper*		m_pCoordinateMapper;

	RGBQUAD*				m_pColorRGBX;
	RGBQUAD*				m_pDepthRGBX;
	IBody*					ppBodies[BODY_COUNT];

	// HD Face Sources
	IHighDefinitionFaceFrameSource* m_pHDFaceFrameSources[BODY_COUNT];

	// HDFace readers
	IHighDefinitionFaceFrameReader*	m_pHDFaceFrameReaders[BODY_COUNT];

	bool bHaveBodyData;
	int MapDepthToByte;
	int SkeletonCount;
	WCHAR UniqueID[256];

	void ConvertOpencvColorImage(cv::Mat *src, RGBQUAD* pBuffer, int nSizeBuffer);
	void ConvertOpencvGrayImage(cv::Mat *src, UINT16* pBuffer, int nHeight, int nWidth, int nDepthMinReliableDistance, int nDepthMaxDistance);
	void ProcessSkel(SkeletonInfo* m_SkeletonInfo, int nBodyCount, IBody** ppBodies, Mat *src, int mode);

	void DrawSkelToMat(Mat *src, Point2d *JointPoints, Joint* pJoints, int mode, int t_id);
	void DrawSkelBone(Mat *src, Joint* pJoints, Point2d* pJointPoints, JointType joint0, JointType joint1, Scalar t_Color);
	void DrawFaceinfo(Mat *src, int iFace, const RectI* pFaceBox, const PointF* pFacePoints, const Vector4* pFaceRotation, const DetectionResult* pFaceProperties, const CameraSpacePoint* pHeadPivot, faceinfo *faceinfo);
	bool ValidateFaceBoxAndPoints(const RectI* pFaceBox, const PointF* pFacePoints);
	void ExtractFaceRotationInDegrees(const Vector4* pQuaternion, int* pPitch, int* pYaw, int* pRoll);

	//Draw Hand state. - but not implemented.
	void DrawHand(Mat *src, HandState handState, Point2d& handposition);

	//Change CameraSpace coordinate to DepthCoordinate / ColorCoorinate.
	Point2d BodyToScreen(const CameraSpacePoint& bodyPoint, int mode);

	//Calculate euclidean dist ( cv::point2d );
	float EuclideanDist(Point2d src1, Point2d src2);

	//Rotate Basis
	void BasisCalibration(SkeletonInfo* m_SkeletonInfo);

	//intrinsic & extrinsic mat
	Mat RTMat;

	bool bDepth;
	bool bColor;
	bool bFace;
	bool bBody;

	//////////////////////////
	faceinfo histFace[BODY_COUNT];
};

