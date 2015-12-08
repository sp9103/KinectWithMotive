#pragma once

#include <stdio.h>
#include <opencv.hpp>
#include "Kinect.h"
#include "Kinect.Face.h"

#define KINECT_COLOR_WIDTH		1920
#define KINECT_COLOR_HEIGHT		1080
#define KINECT_DEPTH_WIDTH		512
#define KINECT_DEPTH_HEIGHT		424

#define OPENCV_WAIT_DELAY		20

#define BODY_COUNT				6
#define NUM_SEGMENTS			14

//Face information structure
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

//Store sensor out Body information
typedef struct SkeletonInfo{
	int Kinect_ID;
	int Count;												//현재 추적하고 있는 스켈레톤 갯수
	SYSTEMTIME	st;
	BodyInfo InfoBody[BODY_COUNT];
}SkeletonInfo;

#define SKELETON_DATA_SIZE 5496

// Safe release for interfaces
template<class Interface>
inline void SafeRelease(Interface *& pInterfaceToRelease)
{
	if (pInterfaceToRelease != NULL)
	{
		pInterfaceToRelease->Release();
		pInterfaceToRelease = NULL;
	}
}

typedef enum _KinectSource KinectSource;

enum _KinectSource
{
	KinectSource_Color	= 0x1,
	KinectSource_Depth	= 0x2,
	KinectSource_Body	= 0x4,
	KinectSource_Face	= 0xC
} ;

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
