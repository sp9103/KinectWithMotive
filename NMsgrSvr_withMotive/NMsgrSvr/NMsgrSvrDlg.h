
// NMsgrSvrDlg.h : ��� ����
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

// CNMsgrSvrDlg ��ȭ ����
class CNMsgrSvrDlg : public CDialogEx
{
	// �����Դϴ�.
public:
	CNMsgrSvrDlg(CWnd* pParent = NULL);	// ǥ�� �������Դϴ�.

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
	int						m_PrimeKinectID;				// ����ȭ �� ������ �Ǵ� Kinect ID, �Ź� ��ȭ��
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
	int						m_BodyDisappear[BODY_COUNT];		// #0 ���� ����, #1 ��� ����, #2 �����, #3 ������� ���ο� ���, # ���ο� ��� �߰�
	int						m_checkNumBodyExist[BODY_COUNT];
	int						m_upperIdxarr[BODY_COUNT];

	// ��ȭ ���� �������Դϴ�.
	enum { IDD = IDD_NMSGRSVR_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV �����Դϴ�.
	void AppendText(CEdit* pEdit, CString addStr);

	// �����Դϴ�.
protected:
	HICON m_hIcon;

	// ������ �޽��� �� �Լ�
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
	//�� skelInfo struct�� �־���. 5��¥�� �迭�� �ƴ� �Ѱ�¥�� ����ü
	void AverBody(synconizedSkeletonsAll syncBody, SkeletonInfo *dst);

private:
	//Find Body direction, refine direction
	synconizedSkeletonsAll prevsync;									//������ ���
	void RefineBodyDirection(synconizedSkeletons *src, int upperRefID, int lowerRefID);		//upperRefID : ��ü ���� Ű��Ʈ ID, lowerRefID : ��ü ���� Ű��Ʈ ID ( ����ü ���� ���� �ٲ��� ) 
	int CalcRefID(SkeletonInfo *Input);
	int CalcUpperRefID(synconizedSkeletons input, synconizedSkeletons prev, int t);
	int CalcLowerRefID(synconizedSkeletons input, synconizedSkeletons prev);
	bool sidefacecheck(synconizedSkeletons input, int idx);

	//Ư�� ���̵� ���̷����� ã�Ƴ�
	void FindsyncSkel(synconizedSkeletonsAll syncBody, SkeletonInfo *dst, int KinectID);

	//������ ������
#ifdef AVERAGE_MODE					//��հ� �׸��� ���
	glBodyRenderer			m_AverRenderer;
#endif

	//3�� ī�޶�� ������
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
