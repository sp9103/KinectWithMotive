
// NMsgrClntDlg.h : 헤더 파일
//

#pragma once
#include "afxwin.h"
#include "KinectConnector.h"
#include "ClntSocket.h"
#include "ofxSweepLine.h"

//#define FACEDETECT_DEBUG
// CNMsgrClntDlg 대화 상자
class CNMsgrClntDlg : public CDialogEx
{
// 생성입니다.
public:
	CNMsgrClntDlg(CWnd* pParent = NULL);	// 표준 생성자입니다.

	void OnConnect();
	void PrintStatus(CString str);
	void Receive_SysTime(SYSTEMTIME st);
	static unsigned __stdcall Skeleton_Tracking(void* pArg);
	
	afx_msg LRESULT OnGetKinectData(WPARAM wParam, LPARAM lParam);
	void OnReceiveKinectData();
	bool makeLineSegments(Point2d* D2Joints, vector<ofPolyline> &segments, vector<SegmentInfo> &segmentInfos);
	void saveSegments(int vIndex, vector<ofPolyline> &segments, vector<SegmentInfo> &segmentInfos, ofVec2f sp, ofVec2f ep, JointType Joint1, JointType Joint2, bool bLeft);
	bool IsFiniteNumber(float x);
	bool IsFinitePoints(ofVec2f sp, ofVec2f ep);
	bool GetBoundingBox(Rect* r, Point2d* D2Joints);
	float GetAverageDepth(Joint* D3Joints);
	bool valueInRange(int value, int min, int max);
	bool rectOverlap(Rect A, Rect B);

	KinectConnector m_Kinect;
	SkeletonInfo	m_SkeletonInfo;
	SYSTEMTIME		m_st;

	CClntSocket		m_ClntSock;
	int				m_nListCount;
	BOOL			CONNECT_STATUS;
	HANDLE			m_hThread;
	bool			m_bReceiveSkeletonMsg;
	ofxSweepLine	m_SweepLine;
	vector<ofVec2f> m_intersections;
	vector<IntersectInfo> m_intersectInfos;
	vector<ofPolyline> m_segments;
	vector<SegmentInfo> m_segmentInfos;

	// 대화 상자 데이터입니다.
	enum { IDD = IDD_NMSGRCLNT_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.

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
	CEdit m_Edit_Status;
	afx_msg void OnBnClickedButtonConnect();
	CEdit m_Edit_IP;
	CEdit m_Edit_ID;
	CString m_strStatus;
	CString m_strIP;
	CString m_strID;
	CButton m_Btn_Connect;
	afx_msg void OnDestroy();

	IplImage *tFaceCheck;
};
