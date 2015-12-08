
// NMsgrSvrDlg.h : 헤더 파일
//

#pragma once

#include "resource.h" 
#include "ServerSocket.h"
#include "afxcmn.h"
#include "afxwin.h"
#include "DrawSkeletons.h"
#include "RecordBodyInfo.h"
#include "glBodyRenderer.h"
//#include "MATLABConn.h"
#include "KF.h"
#include "StateVecRecord.h"

#define AVERAGE_MODE
#define MAINCAM_MODE
#define FACEDEBUG_MODE
#define REFERENCEDEBUG
#define KINECTALL_MODE

#define OUTLIER_THRESHOLD 0.08

// CNMsgrSvrDlg 대화 상자
class CNMsgrSvrDlg : public CDialogEx
{
	// 생성입니다.
public:
	CNMsgrSvrDlg(CWnd* pParent = NULL);	// 표준 생성자입니다.

	void OnAccept(int id);
	void OnDisconnect(int id);
	void OnReceiveSkeleton();
	void PrintStatus(CString str);
	_int64 time_differnece(const SYSTEMTIME st1, const SYSTEMTIME st2);
	afx_msg LRESULT OnGetKinectData(WPARAM wParam, LPARAM lParam);
	static unsigned __stdcall SyncSekeletonThread(void* pArg);	
	bool syncTimeSkeletonData(void* pArg);
	bool firstSyncBodySkeletonData(void* pArg);
	bool syncBodySkeletonData(void* pArg);
	float CalcDistanceTwoBodies(BodyInfo *refBody, BodyInfo *compBody);
	float CalcDistanceTwoBodies2(BodyInfo *refBody, BodyInfo *compBody);
	double ConvertLeftRight(BodyInfo *refBodyInfo, BodyInfo *compBodyInfo, float dist);
	double* calcFilteredVel(double* vel, int dataLen, int dim); 
	bool checkValidate(StateVector* pStateVector, int bodyIdx);
	float calcDist(StateVector* pStateVector, BodyInfo* pBodyInfo, int fromIdx, int toIdx) ;
	float calculateABoneDist(StateVector* pStateVector, int bodyIdx, int targetBoneIdx, int fromJointIdx, int toJintIdx);

	CRITICAL_SECTION		m_cs;
	CServerSocket			m_ServerSocket;
	SkeletonInfo			m_SkeletonsInfo[KINECT_COUNT];				// for time sync
	//synconizedSkeletonsAll	m_syncSkelAll[SYNC_SKELS_BUF_SIZE];
	CList<synconizedSkeletonsAll*, synconizedSkeletonsAll*> m_syncSkelList;		// for body sync
	CChildSocket*			m_pPrimeChildSocket;
	int						m_PrimeKinectID;				// 동기화 때 기준이 되는 Kinect ID, 매번 변화함
	bool					m_bStartSync;
	bool					m_bFirstSync;
	HANDLE					m_hThread;
	DWORD					m_dwThreadID;
	DrawSkeletons			m_DrawSkeletons;
	//	cv::Mat					m_whiteImage[KINECT_COUNT+1];
	//	RecordBodyInfo			m_RecordBody;
	RecordBodyInfo			m_ReadBody;
	glBodyRenderer			m_RefineRenderer;
	SYSTEMTIME				m_st_prev, m_st_curr;
	//	MATLABConn				m_MATLABConn;
	double*					m_noisyVel;
	double*					m_filteredNoisyVel;
	KF						m_KF[BODY_COUNT];
	int						m_BodyDisappear[BODY_COUNT];		// #0 원래 없음, #1 계속 있음, #2 사라짐, #3 사라지고 새로운 사람, # 새로운 사람 추가
	int						m_checkNumBodyExist[BODY_COUNT];
	int						m_upperIdxarr[BODY_COUNT];

	// 대화 상자 데이터입니다.
	enum { IDD = IDD_NMSGRSVR_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.
	void AppendText(CEdit* pEdit, CString addStr);

	// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtGetIp();

	//	CListCtrl m_ListStatus;
	//	CListCtrl m_List_Kinect;
	CEdit m_Edit_Kinect;
	CEdit m_Edit_Status;
	CString m_strStatus;
	afx_msg void OnBnClickedOk();
	afx_msg void OnDestroy();

	//change body dircetion
	void refineBodyDir(synconizedSkeletonsAll *syncBody);

	//Average body
	//input : synconizedSkeletonesAll struct, output : SkeletonInfo struct
	//빈 skelInfo struct를 넣어줌. 5개짜리 배열이 아닌 한개짜리 구조체
	void AverBody(synconizedSkeletonsAll syncBody, SkeletonInfo *dst);

private:
	//Find Body direction, refine direction
	synconizedSkeletonsAll prevsync;									//이전값 사용
	void RefineBodyDirection(synconizedSkeletons *src, int upperRefID, int lowerRefID);		//upperRefID : 상체 기준 키넥트 ID, lowerRefID : 하체 기준 키넥트 ID ( 상하체 따로 방향 바꿔줌 ) 
	int CalcRefID(SkeletonInfo *Input);
	int CalcUpperRefID(synconizedSkeletons input, synconizedSkeletons prev, int t);
	int CalcLowerRefID(synconizedSkeletons input, synconizedSkeletons prev);
	bool sidefacecheck(synconizedSkeletons input, int idx);

	//특정 아이디 스켈레톤을 찾아냄
	void FindsyncSkel(synconizedSkeletonsAll syncBody, SkeletonInfo *dst, int KinectID);

	//디버깅용 렌더러
#ifdef AVERAGE_MODE					//평균값 그리기 모드
	glBodyRenderer			m_AverRenderer;
#endif

	//3번 카메라용 렌더러
#ifdef MAINCAM_MODE
	glBodyRenderer			m_CenterRenderer;
#endif

#ifdef KINECTALL_MODE
	glBodyRenderer			m_BodyRenderer;
#endif

#ifdef FACEDEBUG_MODE
	IplImage *FaceDetected;
	IplImage *t_Ref;
#endif

	StateVecRecord stateRecord;
	StateVector		m_SkeletonTrackingAll[4];
public:
	afx_msg void OnBnClickedBtnWriteStart();
	afx_msg void OnBnClickedBtnFileClose();
	CEdit m_fileName;
	CString m_strFileName;
};
