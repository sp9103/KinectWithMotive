#include "Kinect.h"
#include <opencv.hpp>
#include <vector>

#define KINECT_COUNT 5
#define NUM_KINECTS 5
#define BODY_COUNT 6
#define SYNC_SKELS_BUF_SIZE		5		
#define KINECT_DEPTH_WIDTH		512
#define KINECT_DEPTH_HEIGHT		424
#define OPENCV_WAIT_DELAY		1
#define PI						3.141592653589
#define MAX_WEIGHT				999

#define SWAP(a,b,t) ((t)=(a), (a)=(b), (b)=(t))

typedef struct _faceinfo{
	bool bDetect;						//Face Detection 됬는지 안됫는지
	CameraSpacePoint Facepos;			//3d 공간상의 Face 좌표
	//얼굴 회전 각도
	int yaw;
	int pitch;
	int roll;
}faceinfo;

//Single Body Structure;
typedef struct BodyInfo{
	Joint JointPos[JointType_Count];
	//cv::Point2d jointPoints[JointType_Count];
	UINT64 BodyID;
	faceinfo Face;
	float upperbodylen;
	float lowerbodylen;
	float spinedepth;
}BodyInfo;

typedef struct synconizedSkeletons{
	int Hid;
	int numObservingKinect;
	bool bObsevingKinect[KINECT_COUNT];
	BodyInfo InfoBody[KINECT_COUNT];
	int kinectIDarr[KINECT_COUNT];					//키넥트 아이디를 가지고 있어야함 - 
}synconizedSkeletons;


typedef struct synconizedSkeletonsAll{
	
	SYSTEMTIME	st;
	int numObservedBody;
	bool bObsevedBody[BODY_COUNT];
	synconizedSkeletons syncSkels[BODY_COUNT];

}synconizedSkeletonsAll;

//Store sensor out Body information
typedef struct SkeletonInfo{
	int Kinect_ID;
	int Count;												//현재 추적하고 있는 스켈레톤 갯수
	SYSTEMTIME	st;
	BodyInfo InfoBody[BODY_COUNT];
}SkeletonInfo;

typedef struct MyVector{
	float sx1;
	float sx2;
	float ex1;
	float ex2;
	float x1;
	float x2;
}MyVector;


#define SKELETON_DATA_SIZE 3336
/*
typedef struct SkeletonsInfo{
	SkeletonInfo skel[KINECT_COUNT];
}SkeletonsInfo;
*/

#define PORT_NUM			"5001"		// 서버의 포트 넘버
//-- Thread Message --------------------
#define TH_END_THREAD		0x91
#define HOSTNAME_SIZE		1024
#define SERVER_IP			"127.0.0.1"
#define BUFFER_SIZE			8192
#define DEFAULT_TIMEOUT		100L
#define SD_BOTH				0x02

//-- Struct Size -----------------------
#define IPASS_SIZE			0x0c
#define MSG_SIZE			512
#define HEAD_SIZE			0x07	// 헤드 크기 
#define EFCD_SIZE			0x02	// 0xef, 0xcd 플래그 크기 
#define INFO_SIZE			0x63	// ClientInfo 구조체 크기 
#define FILE_SIZE			0x62	// FileList 구조체 크기 
#define SEND_SIZE			0x0403	// SendFile 구조체 크기 

//-- Head ------------------------------
#define CLIENT_INFO			0x01
#define SKELETON_DATA		0x02
#define SKELETON_DATA_RECEIVED	0x03
#define UPLOAD_DIR			0x04
#define UPLOAD_FILE			0x05
#define DOWN_LOAD			0x06
#define CHAT_MSG			0x07
#define FILE_LIST			0x08
#define TIME_INFO			0x09
#define PACKET_SIZE			0x02   // 소켓 데이터 크기를 저장하고 있는 header의 크기


//-- ClientInfo List -------------------
#define CLIENT_INFO_INFO	0x11
#define CLIENT_INFO_OK		0x12
#define CLIENT_INFO_ERR		0x13


//-- FileList List ---------------------
#define FILE_CONFIRM		0x20
#define CREATE_DIR			0x21	// 폴더 생성 
#define REMOVE_DIR			0x22	// 폴더 삭제 
#define REMOVE_FILE			0x23	// 파일 삭제
#define RENAME_DIR			0x24	// 폴더, 파일 이름 변경 
#define CHANGE_DIR			0x25	// 서버 목록 변경
#define CREATE_ERR			0x26	// 폴더 생성 중복 체크 
#define RENAME_ERR			0x27	// 파일 이름바꾸기 중복 체크 

#define START_LIST			0x28	// 보낸 처음 목록
#define ADD_LIST			0x29	// 추가 목록 

//-- FileList : diff -------------------
#define FILE_DIR			0x30	// 디렉토리 
#define FILE_FILE			0x31	// 파일 

//-- SendFile List ---------------------
#define SEND_DIR			0x40	 
#define SEND_FILE			0x41	// 파일 데이터 보내기 
#define SEND_REQUEST		0x42	// 업로드 요청 
#define SEND_CREATE_ERR		0x43	// 업로드 파일 생성 중복 에러 
#define SEND_REWRITE		0x44	// 파일 쓰기 
#define SEND_REWRITE_OK		0x45	// 파일 쓰기 확인 
#define SEND_CREATE_OK		0x46	// 업로드 파일 생성 확인 
#define SEND_OK				0x47	// 데이터 보내기 
#define SEND_ERR			0x48	// 데이터 보내기 에러 
#define SEND_CANCEL			0x49	// 데이터 보내기 취소 
#define SEND_OPEN			0x50	// 파일 열기 확인 

//-- SendFile : diff -------------------
#define SEND_FIRST			0x51
#define SEND_CENTER			0x52
#define SEND_LAST			0x53


//-- chat msg ---------------------

#define CLIENT_MSG			0x54
#define SERVER_MSG			0x55

// -- Kinect msg --
#define UM_GET_KINECT_DATA	0x60


// CProgressDlg dialog
#define END_UP				0x90
#define END_DOWN			0x91
#define CANCEL_UP			0x92
#define CANCEL_DOWN			0x93

// CClientSock command target
#define UM_ACCEPT			WM_USER+100
#define UM_CLEAR_LIST		WM_USER+201
#define UM_PRINT_STATUS		WM_USER+202
#define UM_PRINT_LIST		WM_USER+203 

#define UM_CONNECT			WM_USER+300
#define UM_CONNECT_ERR		WM_USER+301
#define UM_CLOSE			WM_USER+302
#define UM_SHOW_MESSAGE		WM_USER+304
#define UM_LOGIN_ERR		WM_USER+305
#define UM_SERVER_LIST		WM_USER+306
#define UM_REMOVE_LIST		WM_USER+307
#define UM_RENAME_LIST		WM_USER+308
#define UM_MAKE_DIR			WM_USER+309
#define UM_ON_SEND			WM_USER+310
#define UM_END_SEND			WM_USER+311
#define UM_ADD_LIST			WM_USER+312
#define UM_SEND_CANCEL		WM_USER+313
#define UM_CHAT_MSG			WM_USER+314

#define TH_CLEAR_PDATA		0x92

#define MAX_USER_COUNT	20	// 한 쓰레드당 소켓의 개수

struct ClientInfo
{
	BYTE	id[12];		
	BYTE	pass[12];	
	BYTE	name[25];
	BYTE	path[50];		// 자기 경로 
};

struct FileList
{
	BYTE	diff;			// 파일 구별자(디렉토리, 파일)
	BYTE	filesize[4];	// 파일 크기
	BYTE	filename[75];	// 파일 이름
	BYTE	lsttime[18];	// 마지막 수정 날짜
};

struct SendFile
{
	BYTE	diff;			// 파일 처음, 끝 
	BYTE	filesize[2];	// 파일 크기 
	BYTE	content[1024];	// 파일 내용
};

struct SendMsg
{
	BYTE	id[12];		
//	BYTE	name[25];
	BYTE	msg[512];	// 메세지 내용
};


#define ACTUAL_JOINT_COUNT 19
#define STATE_DIM	ACTUAL_JOINT_COUNT*3
#define TIMEWINDOW	20

enum _ActualJointType
    {
        SpineBase	= 0,
        SpineMid	= 1,
        Neck	= 2,
        Head	= 3,
        ShoulderLeft	= 4,
        ElbowLeft	= 5,
        WristLeft	= 6,
        HandLeft	= 7,
        ShoulderRight	= 8,
        ElbowRight	= 9,
        WristRight	= 10,
        HandRight	= 11,
        HipLeft	= 12,
        KneeLeft	= 13,
        AnkleLeft	= 14,
        HipRight	= 16,
        KneeRight	= 17,
        AnkleRight	= 18,
        SpineShoulder	= 20,
        Actual_JointType_Count	= ( 19 ) 
    } ;

enum _ActualBoneType
    {
        SpineBaseToMid	= 0,
        SpineMidToNeck	= 1,
        NeckToHead	= 2,
        ShoulderSpineToLeft	= 3,
        LeftShoulderToElbow	= 4,
        LeftElbowToWrist	= 5,
        LeftWristToHand	= 6,
        ShoulderSpineToRight = 7,
        RightShoulderToElbow = 8,
        RightElbowToWrist	= 9,
        RightWristToHand	= 10,
        SpineBaseToLeftHip	= 11,
        LeftHipToKnee	= 12,
        LeftKneeToAnkle	= 13,
        SpineBaseToRightHip	= 14,
        RightHipToKnee	= 15,
        RightKneeToAnkle	= 16,
        Actual_BoneType_Count	= ( 17 ) 
    } ;


enum _JointStatus
    {
        ConfidenceNormal = 0,
        ConfidenceOccluding	= 1,
        ConfidenceOccluded	= 2,
        NotConfidenceNormal	= 3,
        NotConfidenceOccluding	= 4,
        NotConfidenceOccluded	= 5,
        JointType_Status_Count	= ( NotConfidenceOccluded + 1 ) 
	} ;

typedef struct StateVector{
	CameraSpacePoint joints[Actual_JointType_Count];
}StateVector;

typedef struct JointVelocity{
	double dx;
	double dy;
	double dz;
}JointVelocity;

typedef struct velVector{
	JointVelocity Jointvels[Actual_JointType_Count];
}velVector;

typedef struct JointsConfidennce{
	bool confidence[JointType_Count];
}JointsConfidennce;

typedef struct JointHistory{
	CameraSpacePoint joints[Actual_JointType_Count];
	SYSTEMTIME	st;
}JointHistory;