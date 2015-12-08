#include "stdafx.h"
#include "KinectConnector.h"


KinectConnector::KinectConnector(void)
{
	bDepth = bColor = bFace = bBody = false;

	m_pColorRGBX = new RGBQUAD[KINECT_COLOR_HEIGHT * KINECT_COLOR_WIDTH];
	m_pDepthRGBX = new RGBQUAD[KINECT_DEPTH_HEIGHT * KINECT_DEPTH_WIDTH];

	MapDepthToByte = 8000 / 256;

	for (int i = 0; i < BODY_COUNT; i++)
	{
		m_pHDFaceFrameSources[i] = nullptr;
		m_pHDFaceFrameReaders[i] = nullptr;

		histFace[i].bDetect = false;
	}

	//Identity Matrix
	RTMat.create(4,4, CV_32FC1);
	for(int i = 0; i < 4; i++){
		for(int j = 0; j < 4; j++){
			if(i == j )
				RTMat.at<float>(i,j) = 1.0f;
			else
				RTMat.at<float>(i,j) = 0.0f;
		}
	}

	//RTMat = (Mat_<float>(4,4) << 0.556736, 0.011479, -0.836429, 2.757066, 0.034158, 1.014769, -0.003559, -0.025543, 0.842073, -0.019536, 0.552613, 0.740417, 0.0, 0.0, 0.0, 1.0);
}

KinectConnector::~KinectConnector(void)
{
	if(m_pColorRGBX){
		delete [] m_pColorRGBX;
		m_pColorRGBX = NULL;
	}
	if(m_pDepthRGBX){
		delete [] m_pDepthRGBX;
		m_pDepthRGBX = NULL;
	}

	SafeRelease(m_pBodyFrameReader);
	SafeRelease(m_pCoordinateMapper);

	RTMat.release();

	for(int i = 0; i < _countof(ppBodies); i++){
		SafeRelease(ppBodies[i]);
	}
}

HRESULT KinectConnector::KinectInitialize(char enabledFrameSourceTypes){
	printf("Start Kinect Initialize...\n");
	HRESULT hr;				//Kinect Color Frame;
	hr = GetDefaultKinectSensor(&m_pKinectSensor);

	if(FAILED(hr)){
		printf("KinectSeonsor Open fail. Check Kinect Sensor connected.\n");
		return hr;
	}

	printf("Start Kinect Initialize...\n");
	//Bit Decompose and set stream flag
	if(0x1 & enabledFrameSourceTypes)
		bColor = true;
	if(0x2 & enabledFrameSourceTypes)
		bDepth = true;
	if(0x4 & enabledFrameSourceTypes)
		bBody = true;
	if(0x8 & enabledFrameSourceTypes)
		bFace = true;

	if(m_pKinectSensor){
		// Initialize the Kinect and get the color reader
		IColorFrameSource*	pColorFrameSource = NULL;
		IDepthFrameSource*	pDepthFrameSource = NULL;
		IBodyFrameSource*	pBodyFrameSource = NULL;

		hr = m_pKinectSensor->Open();
		printf("Kinect Sensor open Complte!\n");

		//General
		if(SUCCEEDED(hr)){
			hr = m_pKinectSensor->get_CoordinateMapper(&m_pCoordinateMapper);
		}

		if(SUCCEEDED(hr)){
			m_pKinectSensor->get_UniqueKinectId(_countof(UniqueID), UniqueID);
			printf("Kinect ID : %s\n", UniqueID);
		}

		//Color Frame source open
		if(bColor){
			printf("Color Stream Init start\n");
			if(SUCCEEDED(hr))
				hr = m_pKinectSensor->get_ColorFrameSource(&pColorFrameSource);

			if(SUCCEEDED(hr))
				hr = pColorFrameSource->OpenReader(&m_pColorFrameReader);

			printf("Color Stream open complete\n");
			SafeRelease(pColorFrameSource);
		}

		//Depth Frame source open
		if(bDepth){
			printf("Depth Stream Init start\n");
			if(SUCCEEDED(hr))
				hr = m_pKinectSensor->get_DepthFrameSource(&pDepthFrameSource);

			if(SUCCEEDED(hr))
				hr = pDepthFrameSource->OpenReader(&m_pDepthFrameReader);

			printf("Depth Stream open complete\n");
			SafeRelease(pDepthFrameSource);
		}

		//Body Frame Sourc open
		if(bBody){
			printf("Body Stream Init start\n");
			if(SUCCEEDED(hr))
				hr = m_pKinectSensor->get_BodyFrameSource(&pBodyFrameSource);

			if(SUCCEEDED(hr))
				hr = pBodyFrameSource->OpenReader(&m_pBodyFrameReader);

			printf("Body Stream open complete\n");
			SafeRelease(pBodyFrameSource);
		}

		//Face Detection source open
		if(bFace){
			printf("Face Stream Init start\n");
			if (SUCCEEDED(hr))
			{
				// create a face frame source + reader to track each body in the fov
				for (int i = 0; i < BODY_COUNT; i++)
				{
					if (SUCCEEDED(hr))
					{
						// create the face frame source by specifying the required face frame features
						//hr = CreateFaceFrameSource(m_pKinectSensor, 0, c_FaceFrameFeatures, &m_pFaceFrameSources[i]);
						hr = CreateHighDefinitionFaceFrameSource(m_pKinectSensor, &m_pHDFaceFrameSources[i]);

					}
					if (SUCCEEDED(hr))
					{
						// open the corresponding reader
						//             hr = m_pFaceFrameSources[i]->OpenReader(&m_pFaceFrameReaders[i]);
						hr = m_pHDFaceFrameSources[i]->OpenReader(&m_pHDFaceFrameReaders[i]);
					}
				}
			}
			printf("Face Stream open complete\n");
		}
	}

	if(!m_pKinectSensor || FAILED(hr)){
		printf("No ready Kinect found!\n");
		return E_FAIL;
	}
	printf("Kinect initialzie Complete\n");

	return hr;
}

void KinectConnector::KinectDestroy(){
	printf("Start Kinect Destroy...\n");
	if(m_pKinectSensor){
		m_pKinectSensor->Close();
	}

	// done with face sources and readers
	for (int i = 0; i < BODY_COUNT; i++)
	{	
		SafeRelease(m_pHDFaceFrameSources[i]);
		SafeRelease(m_pHDFaceFrameReaders[i]);
	}

	SafeRelease(m_pKinectSensor);
	printf("Kinect Destroy Complete\n");
}

HRESULT KinectConnector::GetColorImage(Mat *src){

	if(!m_pColorFrameReader){
		return E_FAIL;
	}

	IColorFrame* pColorFrame = NULL;

	HRESULT hr = m_pColorFrameReader->AcquireLatestFrame(&pColorFrame);

	if(SUCCEEDED(hr)){
		INT64 nTime = 0;
		IFrameDescription* pFrameDescription = NULL;
		int nWidth	= 0;
		int nHeight = 0;
		ColorImageFormat ImageFormat = ColorImageFormat_None;
		UINT nBUfferSize = 0;
		RGBQUAD *pBuffer = NULL;

		hr = pColorFrame->get_RelativeTime(&nTime);

		if(SUCCEEDED(hr)){
			hr = pColorFrame->get_FrameDescription(&pFrameDescription);
		}

		//Check Color Image Width & Height
		if(SUCCEEDED(hr)){
			pFrameDescription->get_Width(&nWidth);
			pFrameDescription->get_Height(&nHeight);
			hr = pColorFrame->get_RawColorImageFormat(&ImageFormat);
		}

		if(SUCCEEDED(hr)){
			if(ImageFormat == ColorImageFormat_Bgra){
				//if Image format is BGRA -> copy image direct.
				hr = pColorFrame->AccessRawUnderlyingBuffer(&nBUfferSize, reinterpret_cast<BYTE**>(&pBuffer));
			}
			else if(m_pColorRGBX){
				//Default Image format Yuy2
				pBuffer = m_pColorRGBX;
				nBUfferSize = KINECT_COLOR_HEIGHT * KINECT_COLOR_WIDTH * sizeof(RGBQUAD);
				hr = pColorFrame->CopyConvertedFrameDataToArray(nBUfferSize, reinterpret_cast<BYTE*>(pBuffer), ColorImageFormat_Bgra);
			}
			else
				hr = E_FAIL;
		}

		//Image Format Convert
		if(SUCCEEDED(hr)){
			ConvertOpencvColorImage(src, pBuffer, nBUfferSize);
		}

		SafeRelease(pFrameDescription);
	}

	SafeRelease(pColorFrame);

	return hr;
}

void KinectConnector::GetDepthImage(Mat *src){
	if( !m_pDepthFrameReader){
		return;
	}

	IDepthFrame* pDepthFrame = NULL;

	HRESULT hr = m_pDepthFrameReader->AcquireLatestFrame(&pDepthFrame);

	if(SUCCEEDED(hr)){
		INT64 nTime = 0;
		IFrameDescription* pFrameDescription = NULL;
		int nWidth = 0;
		int nHeight = 0;
		USHORT nDepthMinReliableDistance	= 0;
		USHORT nDepthMaxDistance			= 0;
		UINT nBufferSize = 0;
		UINT16 *pBuffer	= NULL;

		hr = pDepthFrame->get_RelativeTime(&nTime);

		if(SUCCEEDED(hr)){
			hr = pDepthFrame->get_FrameDescription(&pFrameDescription);
		}

		if(SUCCEEDED(hr)){
			pFrameDescription->get_Width(&nWidth);
			pFrameDescription->get_Height(&nHeight);
			hr = pDepthFrame->get_DepthMinReliableDistance(&nDepthMinReliableDistance);

			// In order to see the full range of depth (including the less reliable far field depth)
			// we are setting nDepthMaxDistance to the extreme potential depth threshold
			nDepthMaxDistance = USHRT_MAX;

			//// 각 어플리케이션에서 최장거리 제한이 필요한 경우 아래 코드 수정..
			// Note:  If you wish to filter by reliable depth distance, uncomment the following line.
			//// hr = pDepthFrame->get_DepthMaxReliableDistance(&nDepthMaxDistance);
		}

		if(SUCCEEDED(hr)){
			hr = pDepthFrame->AccessUnderlyingBuffer(&nBufferSize, &pBuffer);
		}

		if(SUCCEEDED(hr)){
			ConvertOpencvGrayImage(src, pBuffer, nHeight, nWidth, nDepthMinReliableDistance, nDepthMaxDistance);
		}

		SafeRelease(pFrameDescription);
	}

	SafeRelease(pDepthFrame);
}

void KinectConnector::ConvertOpencvColorImage(cv::Mat *src, RGBQUAD* pBuffer, int nBufferSize){
	memcpy(src->data, pBuffer, nBufferSize);
}

void KinectConnector::ConvertOpencvGrayImage(cv::Mat *src, UINT16* pBuffer, int nHeight, int nWidth, int nMinDepth, int nMaxDepth){
	src->setTo(Scalar::all(0));
	if(m_pDepthRGBX && pBuffer){
		//RGBQUAD* pRGBX = m_pDepthRGBX;

		const UINT16* pBufferEnd = pBuffer + (nWidth * nHeight);

		for(int i = 0; i < nWidth*nHeight; i++){
			USHORT depth = *pBuffer;

			for(int j = 0; j < 4; j++)
				src->data[4*i+j] = (byte)(depth >= nMinDepth && depth <= nMaxDepth ? (depth / MapDepthToByte) : 0);

			pBuffer++;
		}
	}
}

HRESULT KinectConnector::GetSkeletonPos(SkeletonInfo* m_SkeletonInfo, Mat *src, int mode){
	if( !m_pBodyFrameReader){
		return E_PENDING;
	}

	//	memset(m_SkeletonInfo, 0x00, sizeof(SkeletonInfo));

	IBodyFrame* pBodyFrame = NULL;

	HRESULT hr = m_pBodyFrameReader->AcquireLatestFrame(&pBodyFrame);
	bHaveBodyData = false;
	//IBody* ppBodies[BODY_COUNT] = {0};

	if(SUCCEEDED(hr)){
		INT64 nTime = 0;

		hr = pBodyFrame->get_RelativeTime(&nTime);

		for(int i = 0; i < BODY_COUNT; i++)
			ppBodies[i] = 0;

		if(SUCCEEDED(hr)){
			hr = pBodyFrame->GetAndRefreshBodyData(_countof(ppBodies), ppBodies);
		}

		//Process raw skeleton data
		if(SUCCEEDED(hr)){
			//bHaveBodyData = true;
			ProcessSkel(m_SkeletonInfo, BODY_COUNT, ppBodies, src, mode);	
			////process & draw body
			BasisCalibration(m_SkeletonInfo);		
		}
	}

	SafeRelease(pBodyFrame);

	return hr;
}

void KinectConnector::DrawSkelToMat(cv::Mat *src, Point2d* JointPoints, Joint* pJoint, int mode, int t_id){
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
	//	DrawSkelBone(src, pJoint, JointPoints, JointType_HandLeft, JointType_HandTipLeft, t_Color1);
	//	DrawSkelBone(src, pJoint, JointPoints, JointType_WristLeft, JointType_ThumbLeft, t_Color1);

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

void KinectConnector::DrawSkelBone(Mat *src, Joint* pJoints, Point2d* pJointPoints, JointType joint0, JointType joint1, Scalar t_Color){
	TrackingState joint0State = pJoints[joint0].TrackingState;
	TrackingState joint1State = pJoints[joint1].TrackingState;

	if ((joint0State == TrackingState_Tracked) && (joint1State == TrackingState_Tracked))   // occluded 
	{
		line(*src, pJointPoints[joint0], pJointPoints[joint1], t_Color, c_TrackedBoneThickness);
	}
	else if ((joint0State == TrackingState_Inferred) && (joint1State == TrackingState_Inferred))   // occluding
	{
		line(*src, pJointPoints[joint0], pJointPoints[joint1], t_Color, c_TrackedBoneThickness);

	}else 
	{
		line(*src, pJointPoints[joint0], pJointPoints[joint1], t_Color, c_TrackedBoneThickness);
	}
}

//Not Implemented. - not work in SDK 2.0 preview version.. (나중에 사용해야할듯 handState가 항상 unknown.)
void KinectConnector::DrawHand(Mat *src, HandState handState, Point2d& handposition){
	switch(handState){
	case HandState_Closed:
		break;
	case HandState_Open:
		break;
	case HandState_Lasso:
		break;
	}
}

Point2d KinectConnector::BodyToScreen(const CameraSpacePoint& bodyPoint, int mode){
	Point2d Return_val;

	if(mode == 0){
		ColorSpacePoint colorPoint = {0};
		m_pCoordinateMapper->MapCameraPointToColorSpace(bodyPoint, &colorPoint);

		Return_val.x = static_cast<double>(colorPoint.X);
		Return_val.y = static_cast<double>(colorPoint.Y);
	}
	if(mode == 1){
		DepthSpacePoint depthPoint = {0};
		m_pCoordinateMapper->MapCameraPointToDepthSpace(bodyPoint, &depthPoint);

		Return_val.x = static_cast<double>(depthPoint.X);
		Return_val.y = static_cast<double>(depthPoint.Y );
	}

	return Return_val;
}

void KinectConnector::ProcessSkel(SkeletonInfo* m_SkeletonInfo, int nBodyCount, IBody** ppBodies, Mat *src, int mode){
	SkeletonCount = 0;
	HRESULT hr;
	CameraSpacePoint zero;
	zero.X = zero.Y = zero.Z = 0.0f;

	if(m_pCoordinateMapper){
		for(int i = 0; i < nBodyCount; i++){
			IBody* pBody = ppBodies[i];

			if(pBody){				
				BOOLEAN bTracked = false;
				hr = pBody->get_IsTracked(&bTracked);

				if(SUCCEEDED(hr) && bTracked){
					bHaveBodyData = true;
					Joint joints[JointType_Count];
					Point2d jointPoints[JointType_Count];
					HandState leftHandState = HandState_Unknown;
					HandState rightHandState = HandState_Unknown;

					pBody->get_HandLeftState(&leftHandState);
					pBody->get_HandRightState(&rightHandState);

					hr = pBody->GetJoints(_countof(joints), joints);

					//Get Tracking Body ID;
					UINT64 t_ID;
					hr = pBody->get_TrackingId(&t_ID);

					m_SkeletonInfo->InfoBody[SkeletonCount].BodyID = t_ID;

					if(SUCCEEDED(hr)){
						for(int j = 0; j < _countof(joints); j++){
							m_SkeletonInfo->InfoBody[SkeletonCount].JointPos[j] = joints[j];
							jointPoints[j] = BodyToScreen(joints[j].Position, mode);
							//m_SkeletonInfo->InfoBody[SkeletonCount].jointPoints[j] = jointPoints[j];
						}

						//상하체 길이 return
						m_SkeletonInfo->InfoBody[SkeletonCount].upperbodylen = EuclideanDist(jointPoints[JointType_ShoulderLeft], jointPoints[JointType_ShoulderRight]);
						m_SkeletonInfo->InfoBody[SkeletonCount].lowerbodylen = EuclideanDist(jointPoints[JointType_HipLeft], jointPoints[JointType_HipRight]);
						m_SkeletonInfo->InfoBody[SkeletonCount].spinedepth = (sqrt(pow(joints[JointType_SpineBase].Position.X, 2) + pow(joints[JointType_SpineBase].Position.Y, 2) + pow(joints[JointType_SpineBase].Position.Z, 2))
							+ sqrt(pow(joints[JointType_SpineMid].Position.X, 2) + pow(joints[JointType_SpineMid].Position.Y, 2) + pow(joints[JointType_SpineMid].Position.Z, 2))
							+ sqrt(pow(joints[JointType_SpineShoulder].Position.X, 2) + pow(joints[JointType_SpineShoulder].Position.Y, 2) + pow(joints[JointType_SpineShoulder].Position.Z, 2))) / 3.0f;

						SkeletonCount++;
						DrawSkelToMat(src, jointPoints, joints, mode, t_ID);
					}

					//					DrawHand(src, leftHandState, jointPoints[JointType_HandLeft]);
					//					DrawHand(src, rightHandState, jointPoints[JointType_HandRight]); 
				}
			}
		}
	}
	m_SkeletonInfo->Count = SkeletonCount;
}

void KinectConnector::GetKinectID(WCHAR *KinectID){
	memcpy(KinectID, UniqueID, _countof(UniqueID)*sizeof(WCHAR));
}

bool KinectConnector::ValidateFaceBoxAndPoints(const RectI* pFaceBox, const PointF* pFacePoints)
{
	bool isFaceValid = false;

	if (pFaceBox != nullptr)
	{
		INT32 screenWidth = KINECT_COLOR_WIDTH;
		INT32 screenHeight = KINECT_COLOR_HEIGHT;

		INT32 width = pFaceBox->Right - pFaceBox->Left;
		INT32 height = pFaceBox->Bottom - pFaceBox->Top;

		// check if we have a valid rectangle within the bounds of the screen space
		isFaceValid = width > 0 && 
			height > 0 && 
			pFaceBox->Right <= screenWidth && 
			pFaceBox->Bottom <= screenHeight;

		//if (isFaceValid)
		//{
		//    for (int i = 0; i < FacePointType::FacePointType_Count; i++)
		//    {
		//        // check if we have a valid face point within the bounds of the screen space                        
		//        bool isFacePointValid = pFacePoints[i].X > 0.0f &&
		//            pFacePoints[i].Y > 0.0f &&
		//            pFacePoints[i].X < m_sourceWidth &&
		//            pFacePoints[i].Y < m_sourceHeight;

		//        if (!isFacePointValid)
		//        {
		//            isFaceValid = false;
		//            break;
		//        }
		//    }
		//}
	}

	return isFaceValid;
}

void KinectConnector::ExtractFaceRotationInDegrees(const Vector4* pQuaternion, int* pPitch, int* pYaw, int* pRoll)
{
	double x = pQuaternion->x;
	double y = pQuaternion->y;
	double z = pQuaternion->z;
	double w = pQuaternion->w;
	const double M_PI = 3.141592;

	// convert face rotation quaternion to Euler angles in degrees		
	double dPitch, dYaw, dRoll;
	dPitch = atan2(2 * (y * z + w * x), w * w - x * x - y * y + z * z) / M_PI * 180.0;
	dYaw = asin(2 * (w * y - x * z)) / M_PI * 180.0;
	dRoll = atan2(2 * (x * y + w * z), w * w + x * x - y * y - z * z) / M_PI * 180.0;

	// clamp rotation values in degrees to a specified range of values to control the refresh rate
	double increment = 5.0;
	*pPitch = static_cast<int>((dPitch + increment/2.0 * (dPitch > 0 ? 1.0 : -1.0)) / increment) * static_cast<int>(increment);
	*pYaw = static_cast<int>((dYaw + increment/2.0 * (dYaw > 0 ? 1.0 : -1.0)) / increment) * static_cast<int>(increment);
	*pRoll = static_cast<int>((dRoll + increment/2.0 * (dRoll > 0 ? 1.0 : -1.0)) / increment) * static_cast<int>(increment);
}

void KinectConnector::DrawFaceinfo(Mat *src, int iFace, const RectI* pFaceBox, const PointF* pFacePoints, const Vector4* pFaceRotation, const DetectionResult* pFaceProperties, const CameraSpacePoint* pHeadPivot, faceinfo *faceinfo){
	if (ValidateFaceBoxAndPoints(pFaceBox, pFacePoints))
	{
		UINT64 tid;
		ppBodies[iFace]->get_TrackingId(&tid);
		Scalar tColor = Scalar((tid*37)%256, (tid*113)%256, (tid*71)%256); 
		Rect faceBound = Rect(pFaceBox->Left, pFaceBox->Top, pFaceBox->Right - pFaceBox->Left, pFaceBox->Bottom - pFaceBox->Top);

		cv::rectangle(*src, faceBound, tColor, 6);

		int TextTerm = 30;
		string faceText;
		Point TextPoint = Point(pFaceBox->Right, pFaceBox->Top);
		faceText = " HeadPivot Coordinates";
		cv::putText(*src, faceText, TextPoint, FONT_HERSHEY_PLAIN, 2.0, tColor, 3);

		faceText = " X-> " + std::to_string( pHeadPivot->X ) + " Y-> " + std::to_string(pHeadPivot->Y) + " Z-> " + std::to_string(pHeadPivot->Z);
		TextPoint.y += TextTerm;
		cv::putText(*src, faceText, TextPoint, FONT_HERSHEY_PLAIN, 2.0, tColor, 3);

		// extract face rotation in degrees as Euler angles
		int pitch, yaw, roll;
		ExtractFaceRotationInDegrees(pFaceRotation, &pitch, &yaw, &roll);

		faceText = " FaceYaw : " + std::to_string( yaw );
		TextPoint.y += TextTerm;
		cv::putText(*src, faceText, TextPoint, FONT_HERSHEY_PLAIN, 2.0, tColor, 3);
		faceText = " FacePitch : " + std::to_string( pitch );
		TextPoint.y += TextTerm;
		cv::putText(*src, faceText, TextPoint, FONT_HERSHEY_PLAIN, 2.0, tColor, 3);
		faceText = " FaceRoll : " + std::to_string( roll );
		TextPoint.y += TextTerm;
		cv::putText(*src, faceText, TextPoint, FONT_HERSHEY_PLAIN, 2.0, tColor, 3);

		faceinfo->pitch		= pitch;
		faceinfo->yaw		= yaw;
		faceinfo->roll		= roll;
	}
}

HRESULT KinectConnector::FaceDetection(SkeletonInfo *m_SkeletonInfo, cv::Mat *src){
	HRESULT hr = E_PENDING;
	HRESULT hRefresh = E_PENDING;
	int validCount = 0, latestCount = 0;
	//bool bHaveBodyData = SUCCEEDED( UpdateBodyData(ppBodies) );

	for( int i = 0; i < BODY_COUNT; i++){
		IHighDefinitionFaceFrame * pHDFaceFrame = nullptr;

		hr = m_pHDFaceFrameReaders[i]->AcquireLatestFrame(&pHDFaceFrame);

		m_SkeletonInfo->InfoBody[i].Face.bDetect = false;
		BOOLEAN bFaceTracked = false;

		if ( SUCCEEDED(hr) )
		{
			hRefresh = hr;
			latestCount++;
			if( nullptr != pHDFaceFrame ){
				// check if a valid face is tracked in this face frame
				hr = pHDFaceFrame->get_IsTrackingIdValid(&bFaceTracked);
			}

			if(SUCCEEDED ( hr )){
				validCount++;
			}
		}

		if (bFaceTracked)
		{
			//맞는 body index find
			int tbodyidx = -1;
			UINT64 faceid = -1;
			pHDFaceFrame->get_TrackingId(&faceid);

			for(int j = 0; j < m_SkeletonInfo->Count; j++){
				if(m_SkeletonInfo->InfoBody[j].BodyID == faceid){
					tbodyidx = j;
					break;
				}
			}

			IFaceAlignment* pFaceAlignment = nullptr;
			CreateFaceAlignment(&pFaceAlignment);
			pHDFaceFrame->GetAndRefreshFaceAlignmentResult(pFaceAlignment);

			float* pAnimationUnits = new float[FaceShapeAnimations_Count];

			pFaceAlignment->GetAnimationUnits(FaceShapeAnimations_Count, pAnimationUnits);

			RectI faceBox = {0};

			//HighDetailFacePoints::
			PointF hdPoints[36];

			PointF facePoints[FacePointType::FacePointType_Count];
			Vector4 faceRotation;

			pFaceAlignment->get_FaceBoundingBox(&faceBox);
			if(faceBox.Right != 0 && faceBox.Left != 0){
				//TRACE(L"Face Box Top: %d\tBottom: %d\tLeft: %d\tRight: %d", faceBox.Top, faceBox.Bottom, faceBox.Left, faceBox.Right);

				pFaceAlignment->get_FaceOrientation(&faceRotation);
				//TRACE(L"Face Rotation X:\t%f\t\tY:\t%f\t\tZ:\t%f\t\tW:\t%f", faceRotation.x, faceRotation.y, faceRotation.z, faceRotation.w);

				CameraSpacePoint headPivot;				
				pFaceAlignment->get_HeadPivotPoint(&headPivot);
				//TRACE(L"Head Pivot X:\t%f\t\tY:\t%f\t\tZ:\t%f", headPivot.X, headPivot.Y, headPivot.Z);

				FaceAlignmentQuality faceQuality;
				pFaceAlignment->get_Quality(&faceQuality);

				DetectionResult faceProperties[FaceProperty::FaceProperty_Count];

				DrawFaceinfo(src, i, &faceBox, facePoints, &faceRotation, faceProperties, &headPivot, &m_SkeletonInfo->InfoBody[tbodyidx].Face);

				m_SkeletonInfo->InfoBody[tbodyidx].Face.bDetect = true;
				m_SkeletonInfo->InfoBody[tbodyidx].Face.Facepos = headPivot;	
			}

			delete [] pAnimationUnits;
		}
		else {
			// face tracking is not valid - attempt to fix the issue
			// a valid body is required to perform this step
			if (bHaveBodyData)
			{
				// check if the corresponding body is tracked 
				// if this is true then update the face frame source to track this body
				IBody* pBody = ppBodies[i];
				if (pBody != nullptr)
				{
					BOOLEAN bTracked = false;
					hr = pBody->get_IsTracked(&bTracked);

					UINT64 bodyTId;
					if (SUCCEEDED(hr) && bTracked)
					{
						// get the tracking ID of this body
						hr = pBody->get_TrackingId(&bodyTId);
						if (SUCCEEDED(hr))
						{
							// update the face frame source with the tracking ID
							m_pHDFaceFrameSources[i]->put_TrackingId(bodyTId);
						}
					}
				}
			}
		}
	}

	if(latestCount != m_SkeletonInfo->Count || validCount != m_SkeletonInfo->Count){
		return E_PENDING;
	}

	return hRefresh;
}

void KinectConnector::BasisCalibration(SkeletonInfo* m_SkeletonInfo){
	BodyInfo tempBody;

	Mat camData;
	camData.create(4,1, CV_32FC1);

	for(int i = 0; i < m_SkeletonInfo->Count; i++){
		tempBody = m_SkeletonInfo->InfoBody[i];

		//Calibration - 각 조인트에 대해서
		for(int j = 0; j < JointType_Count; j++){
			Joint tempJoint = tempBody.JointPos[j];

			camData.at<float>(0,0) = tempJoint.Position.X;
			camData.at<float>(1,0) = tempJoint.Position.Y;
			camData.at<float>(2,0) = tempJoint.Position.Z;
			camData.at<float>(3,0) = 1.0f;

			camData = RTMat * camData;

			tempJoint.Position.X = camData.at<float>(0,0);
			tempJoint.Position.Y = camData.at<float>(1,0);
			tempJoint.Position.Z = camData.at<float>(2,0);

			m_SkeletonInfo->InfoBody[i].JointPos[j] = tempJoint;	
		}
	}
}

float KinectConnector::EuclideanDist(Point2d src1, Point2d src2){
	return sqrt(pow(src1.x - src2.x,2) + pow(src1.y - src2.y,2));
}

void KinectConnector::SetRTmat(Mat src){
	src.copyTo(RTMat);
}