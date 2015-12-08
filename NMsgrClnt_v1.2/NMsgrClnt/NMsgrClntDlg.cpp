
// NMsgrClntDlg.cpp : 구현 파일
//

#include "stdafx.h"
#include "NMsgrClnt.h"
#include "NMsgrClntDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// 대화 상자 데이터입니다.
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	// 구현입니다.
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CNMsgrClntDlg 대화 상자




CNMsgrClntDlg::CNMsgrClntDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CNMsgrClntDlg::IDD, pParent)
	, m_strStatus(_T(""))
	, m_strIP(_T(""))
	, m_strID(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	CONNECT_STATUS = FALSE;
	m_nListCount = 0;
	m_hThread = NULL;

	tFaceCheck = cvCreateImage(cvSize(64, 64*BODY_COUNT), IPL_DEPTH_8U, 1);
}

void CNMsgrClntDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_STATUS, m_Edit_Status);
	DDX_Control(pDX, IDC_EDIT_IP, m_Edit_IP);
	DDX_Control(pDX, IDC_EDIT_ID, m_Edit_ID);
	DDX_Text(pDX, IDC_EDIT_STATUS, m_strStatus);
	DDX_Text(pDX, IDC_EDIT_IP, m_strIP);
	DDX_Text(pDX, IDC_EDIT_ID, m_strID);
	DDX_Control(pDX, ID_BUTTON_CONNECT, m_Btn_Connect);
}

BEGIN_MESSAGE_MAP(CNMsgrClntDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(ID_BUTTON_CONNECT, &CNMsgrClntDlg::OnBnClickedButtonConnect)
	ON_WM_DESTROY()

	ON_MESSAGE(UM_GET_KINECT_DATA, OnGetKinectData)
END_MESSAGE_MAP()


// CNMsgrClntDlg 메시지 처리기

BOOL CNMsgrClntDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 이 대화 상자의 아이콘을 설정합니다. 응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	ShowWindow(SW_SHOW);

	// TODO: 여기에 추가 초기화 작업을 추가합니다.
	m_ClntSock.SetWnd(this);

	m_Edit_IP.SetWindowTextA("192.168.0.10");
	//	m_Edit_IP.SetWindowTextA("127.0.0.1");
	m_Edit_ID.SetWindowTextA("3");

	HRESULT ret = m_Kinect.KinectInitialize(KinectSource_Color | KinectSource_Depth| KinectSource_Body | KinectSource_Face);

	if (ret == S_OK) {
		PrintStatus("Kinect Connection Success");
	}else{
		PrintStatus("Kinect Connection Failure");
	}

	m_bReceiveSkeletonMsg = true;

	SetWindowPos(NULL, 1308, 390, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CNMsgrClntDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다. 문서/뷰 모델을 사용하는 MFC 응용 프로그램의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CNMsgrClntDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CNMsgrClntDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CNMsgrClntDlg::OnBnClickedButtonConnect()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	UpdateData(TRUE);
	if (m_strIP=="") 
	{
		PrintStatus("IP를 입력해주세요!");
		m_Edit_IP.SetFocus();
	}
	else if (m_strID=="") 
	{
		PrintStatus("ID를 입력해주세요!");
		m_Edit_ID.SetFocus();
	}
	else 
	{		
		BOOL bSuccess = m_ClntSock.ConnectTo( m_strIP, PORT_NUM, AF_INET, SOCK_STREAM); // 소켓 생성 서버와 연결 

		if (bSuccess) {// 접속이 시도되고 통신상태 감시가 완료되면 
			bSuccess = m_ClntSock.WatchConn();									// 서버로부터 데이터를 전달 받을 수 있는 쓰레드 생성

			if(!bSuccess)
			{
				m_ClntSock.StopConn();
				PrintStatus("서버와의 연결에 실패했습니다");
				return;
			}
		}else {
			m_ClntSock.StopConn();
			PrintStatus("서버와의 연결에 실패했습니다");
			return;
		}
		m_Btn_Connect.EnableWindow(FALSE);
		m_Edit_IP.EnableWindow(FALSE);
		m_Edit_ID.EnableWindow(FALSE);

		namedWindow("KinectColorFrame"/*, CV_WINDOW_KEEPRATIO*/);
		moveWindow("KinectColorFrame", 10, 515);
		namedWindow("KinectDepthFrame"/*, CV_WINDOW_KEEPRATIO*/);
		moveWindow("KinectDepthFrame", 650, 515);

		OnConnect();														// 로긴 요청
	}	
}

/*-----------------------------------------------------------------------
| OnConnect : 서버에 정상적으로 로그인 되었을 때 
+----------------------------------------------------------------------*/
void CNMsgrClntDlg::OnConnect()
{
	UpdateData(TRUE);

	WORD wTotal=0, wData=4;
	BYTE byTotal[PACKET_SIZE], byId[4], *bySend=NULL, *nAddress=NULL;  //byPass[12],
	int nTotal=0;
	memset(byId, 0x00, 4);
	//memset(byPass, 0x00, 12);

	CONNECT_STATUS = TRUE;
	//	m_cmbServer.SetCurSel(0);

	CString strMsg = "♣♣♣  서버에 접속하였습니다.  ♣♣♣";
	PrintStatus(strMsg);

	//	m_strIP.TrimLeft();
	//	m_strIP.TrimRight();
	m_strID.TrimLeft();
	m_strID.TrimRight();	

	memcpy(byId, (VOID*)LPCTSTR(m_strID), m_strID.GetLength());
	//memcpy(byPass, m_strID.GetBuffer(m_strID.GetLength()), m_strID.GetLength());

	wTotal = HEAD_SIZE + wData + EFCD_SIZE;
	//	wTotal = (WORD)nTotal;
	//	itoa(nTotal, (char*)byTotal, 10);

	bySend = new BYTE[wTotal];
	memset(bySend, 0x00, wTotal);

	memcpy(bySend, &wTotal, sizeof(WORD));
	nAddress=bySend+sizeof(WORD);	*nAddress = CLIENT_INFO;
	nAddress++;						memset(nAddress, 0x00, sizeof(WORD));
	nAddress+=sizeof(WORD);			memcpy(nAddress, &wData, sizeof(WORD));
	nAddress+=sizeof(WORD);			memcpy(nAddress, byId, 4);
	nAddress+=4;					*nAddress = 0xef;
	nAddress++;						*nAddress = 0xcd;

	m_ClntSock.sendData(bySend, wTotal, 0);

	/*	m_btnClose.EnableWindow(TRUE);
	m_btnConn.EnableWindow(FALSE);
	m_editIP.EnableWindow(FALSE);
	m_editID.EnableWindow(FALSE);
	m_btnMkdir.EnableWindow(TRUE);
	m_btnDel.EnableWindow(TRUE);
	m_btnRename.EnableWindow(TRUE);
	m_btnUp.EnableWindow(TRUE);
	m_btnDown.EnableWindow(TRUE);
	m_cmbServer.EnableWindow(TRUE);
	*/
	delete bySend;
}

/*--------------------------------------------------------------
| PrintStatus : 클라이언트 정보 출력 
| argument	  : str - 정보 내용 
+-------------------------------------------------------------*/
void CNMsgrClntDlg::PrintStatus(CString str)
{
	if (str == "") return;

	CString prevStr, curStr;

	if(m_Edit_Status.GetLineCount() >= 100)
	{
		int nLength = 0;

		for(int i = 0; i < 100; i++)
			nLength += m_Edit_Status.LineLength(i);

		m_Edit_Status.SetSel( 0, nLength, FALSE);
		m_Edit_Status.ReplaceSel("", FALSE); 
	}

	m_Edit_Status.GetWindowTextA(prevStr);

	str += "\r\n";
	curStr = prevStr + str;

	m_Edit_Status.SetWindowTextA(curStr);

	m_Edit_Status.LineScroll(m_Edit_Status.GetLineCount());
}

void CNMsgrClntDlg::Receive_SysTime(SYSTEMTIME st)
{
	CString str;
	str.Format("System Time Change : %d/%d/%d/%d/%d/%d/%d)",st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond,st.wMilliseconds);
	PrintStatus(str);

	HANDLE hThread;
	hThread = (HANDLE)_beginthreadex(NULL,	// Security attributes
		0,	// stack
		Skeleton_Tracking,			// Thread proc
		this,	// Thread param
		0,	// creation mode
		NULL);	// Thread ID

	if ( NULL != hThread)
	{
		//SetThreadPriority(hThread, THREAD_PRIORITY_HIGHEST);
		ResumeThread( hThread );
		m_hThread = hThread;
	}
}

unsigned __stdcall CNMsgrClntDlg::Skeleton_Tracking(void* pArg)
{
	CNMsgrClntDlg* pDlg = (CNMsgrClntDlg*)pArg;
	bool ret;
	int i, j, tcount, inter_count, cx, cy;
	//Point2d* D2Joints;
	Joint* D3Joints;
	Rect r[BODY_COUNT];
	float aveDepth[BODY_COUNT];
	bool occludedBodyIdies[BODY_COUNT];
	int Index1[2], Index2[2];
	float dist1, dist2;
	ofVec2f p1, p2;
	float d1, d2, depthDiff, depth1, depth2;
	cv::Point point1, point2;
	int offset = 10;
	//	Scalar t_Color = Scalar(0, 0, 255); 

	DWORD waitdelay;
	DWORD dwFames = 0;
	DWORD dwCurrentTime = 0;
	DWORD dwLastUpdateTime = 0;
	DWORD dwElapsedTime = 0;

	HRESULT hr;
	faceinfo prevFace[BODY_COUNT] = {0};

	cv::Mat tKinectColorImage, tKinectDepthImage, tKinectColorresize;
	tKinectColorImage.create(KINECT_COLOR_HEIGHT, KINECT_COLOR_WIDTH, CV_8UC4);			//Kinect Depth Image format BGRA 4 Channel image
	tKinectDepthImage.create(KINECT_DEPTH_HEIGHT, KINECT_DEPTH_WIDTH, CV_8UC4);
	tKinectColorresize.create(KINECT_COLOR_HEIGHT/3, KINECT_COLOR_WIDTH/3, CV_8UC4); 

	while(true){
		ret = false;
		tcount = 0;
		waitdelay = GetTickCount();
		pDlg->m_Kinect.GetDepthImage(&tKinectDepthImage);
		hr = pDlg->m_Kinect.GetColorImage(&tKinectColorImage);

		if(SUCCEEDED (hr)){
			//Color Frame 받아오기 성공시
			hr = pDlg->m_Kinect.GetSkeletonPos(&pDlg->m_SkeletonInfo, &tKinectColorImage, 0);
		}
		if(SUCCEEDED (hr)){
			//Skeleton tracking 성공시
			hr = pDlg->m_Kinect.FaceDetection(&pDlg->m_SkeletonInfo, &tKinectColorImage);
			GetSystemTime(&pDlg->m_st);

			//FaceDetection 실패시 history 반영
			if(FAILED(hr)){
				for(int i = 0; i < BODY_COUNT; i++){
					memcpy(&pDlg->m_SkeletonInfo.InfoBody[i].Face, &prevFace[i], sizeof(faceinfo));
				}
			}else{
				for(int i = 0; i < BODY_COUNT; i++){
					memcpy(&prevFace[i], &pDlg->m_SkeletonInfo.InfoBody[i].Face, sizeof(faceinfo));
				}
			}

#ifdef FACEDETECT_DEBUG
			cvZero(pDlg->tFaceCheck);
			IplImage *temp = cvCreateImage(cvSize(64, 64), IPL_DEPTH_8U, 1);
			for(int i = 0; i < pDlg->m_SkeletonInfo.Count; i++){
				if(pDlg->m_SkeletonInfo.InfoBody[i].Face.bDetect)
					cvSet(temp, cvScalarAll(255));
				else
					cvZero(temp);

				cvSetImageROI(pDlg->tFaceCheck, cvRect(0, 64*i, 64, 64));
				cvCopy(temp, pDlg->tFaceCheck);
				cvResetImageROI(pDlg->tFaceCheck);
			}
			cvShowImage("Clnt Face", pDlg->tFaceCheck);
			waitKey(1);
			cvReleaseImage(&temp);
#endif

			tcount = pDlg->m_SkeletonInfo.Count;

			if (tcount >= 0) {
				pDlg->m_bReceiveSkeletonMsg = false;


				pDlg->OnReceiveKinectData();

				waitdelay = GetTickCount() - waitdelay;
				waitdelay = 33 - waitdelay/1000;
				waitKey(1);

				dwFames++;
				dwCurrentTime = GetTickCount();
				dwElapsedTime = dwCurrentTime - dwLastUpdateTime;
				CString str;
				if(dwElapsedTime >= 1000){
					str.Format("FPS = %u", (UINT)(dwFames * 1000.0 / dwElapsedTime));
					pDlg->PrintStatus(str);

					dwFames = 0;
					dwLastUpdateTime = dwCurrentTime;
				}

			}
		}

		imshow("KinectDepthFrame", tKinectDepthImage);
		cv::resize(tKinectColorImage, tKinectColorresize, cv::Size(KINECT_COLOR_WIDTH/3, KINECT_COLOR_HEIGHT/3), 0, 0, INTER_AREA);
		imshow("KinectColorFrame", tKinectColorresize);
	}
	return 0;
}

afx_msg LRESULT CNMsgrClntDlg::OnGetKinectData(WPARAM wParam, LPARAM lParam)
{
	WORD wTotal=0, wData=SKELETON_DATA_SIZE;
	BYTE bySkeleton[SKELETON_DATA_SIZE], *bySend=NULL, *nAddress=NULL;  //byPass[12],
	memset(bySkeleton, 0x00, SKELETON_DATA_SIZE);
	//memset(byPass, 0x00, 12);

	//	int a = sizeof(SkeletonInfo);
	memcpy(&m_SkeletonInfo.st, &m_st, sizeof(SYSTEMTIME));
	m_SkeletonInfo.Kinect_ID = atoi(m_strID);

	memcpy(bySkeleton, (BYTE*)&m_SkeletonInfo, sizeof(SkeletonInfo));
	//memcpy(byPass, m_strID.GetBuffer(m_strID.GetLength()), m_strID.GetLength());

	wTotal = HEAD_SIZE + wData + EFCD_SIZE;

	bySend = new BYTE[wTotal];
	memset(bySend, 0x00, wTotal);

	memcpy(bySend, &wTotal, sizeof(WORD));
	nAddress=bySend+sizeof(WORD);	*nAddress = SKELETON_DATA;
	nAddress++;						memset(nAddress, 0x00, sizeof(WORD));
	nAddress+=sizeof(WORD);			memcpy(nAddress, &wData, sizeof(WORD));
	nAddress+=sizeof(WORD);			memcpy(nAddress, bySkeleton, SKELETON_DATA_SIZE);
	nAddress+=wData;					*nAddress = 0xef;
	nAddress++;						*nAddress = 0xcd;

	m_ClntSock.sendData(bySend, wTotal, 0);

	return 1; 	
}

void CNMsgrClntDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO: Add your message handler code here
	m_Kinect.KinectDestroy();
}

void CNMsgrClntDlg::OnReceiveKinectData()
{
	/*	int i;
	CString str;
	for (i = 0; i < m_SkeletonInfo.Count; i++) {
	str.Format("Body_ID:%d\r\n", m_SkeletonInfo.InfoBody[i].BodyID);
	PrintStatus(str);
	}	
	*/
	//	m_bReceiveSkeletonMsg = false;
	WORD wTotal=0, wData=SKELETON_DATA_SIZE;
	BYTE bySkeleton[SKELETON_DATA_SIZE], *bySend=NULL, *nAddress=NULL;  //byPass[12],
	memset(bySkeleton, 0x00, SKELETON_DATA_SIZE);
	//memset(byPass, 0x00, 12);

	//	int a = sizeof(SkeletonInfo);
	memcpy(&m_SkeletonInfo.st, &m_st, sizeof(SYSTEMTIME));
	m_SkeletonInfo.Kinect_ID = atoi(m_strID);

	memcpy(bySkeleton, (BYTE*)&m_SkeletonInfo, sizeof(SkeletonInfo));
	//memcpy(byPass, m_strID.GetBuffer(m_strID.GetLength()), m_strID.GetLength());

	wTotal = HEAD_SIZE + wData + EFCD_SIZE;

	bySend = new BYTE[wTotal];
	memset(bySend, 0x00, wTotal);

	memcpy(bySend, &wTotal, sizeof(WORD));
	nAddress=bySend+sizeof(WORD);	*nAddress = SKELETON_DATA;
	nAddress++;						memset(nAddress, 0x00, sizeof(WORD));
	nAddress+=sizeof(WORD);			memcpy(nAddress, &wData, sizeof(WORD));
	nAddress+=sizeof(WORD);			memcpy(nAddress, bySkeleton, SKELETON_DATA_SIZE);
	nAddress+=wData;					*nAddress = 0xef;
	nAddress++;						*nAddress = 0xcd;

	m_ClntSock.sendData(bySend, wTotal, 0);
}

#include <ctime>

void CNMsgrClntDlg::saveSegments(int vIndex, vector<ofPolyline> &segments, vector<SegmentInfo> &segmentInfos, ofVec2f sp, ofVec2f ep, JointType Joint1, JointType Joint2, bool bLeft)
{
	float random = 0.f;
	float offset1 = 0.001;
	float offset2 = 0.0005;
	SegmentInfo segmentInfo[NUM_SEGMENTS]; 
	ofPolyline polyline[NUM_SEGMENTS];

	random = ((float)rand() / (float)(RAND_MAX + 1));

	if (bLeft == true)
		sp.x = sp.x -offset1;
	else
		sp.x = sp.x +offset1;

	if (sp.x == ep.x) sp.x = sp.x - offset2*random;
	polyline[vIndex].addVertex(sp); polyline[vIndex].addVertex(ep);
	segments.push_back(polyline[vIndex]);
	segmentInfo[vIndex].segmentIdx = vIndex; segmentInfo[vIndex].jointIdx1 = Joint1; segmentInfo[vIndex].jointIdx2 = Joint2;
	segmentInfos.push_back(segmentInfo[vIndex]);
}

bool CNMsgrClntDlg::IsFinitePoints(ofVec2f sp, ofVec2f ep) 
{
	if (!IsFiniteNumber(sp.x)) return false;
	if (!IsFiniteNumber(sp.y)) return false;
	if (!IsFiniteNumber(ep.x)) return false;
	if (!IsFiniteNumber(ep.y)) return false;

	return true;
}



bool CNMsgrClntDlg::IsFiniteNumber(float x) 
{
	return (x <= DBL_MAX && x >= -DBL_MAX); 
}

bool CNMsgrClntDlg::makeLineSegments(Point2d* D2Joints, vector<ofPolyline> &segments, vector<SegmentInfo> &segmentInfos)
{
	ofVec2f sp, ep;

	srand(time(0));

	//#1
	/*	sp.x = D2Joints[JointType_Head].x; 	sp.y = D2Joints[JointType_Head].y;
	ep.x = D2Joints[JointType_Neck].x; 	ep.y = D2Joints[JointType_Neck].y-offset1;
	if (!IsFinitePoints(sp, ep)) return false;
	saveSegments(0, segments, segmentInfos, sp, ep, JointType_Head, JointType_Neck);

	//#2
	sp.x = D2Joints[JointType_Neck].x; 	sp.y = D2Joints[JointType_Neck].y+offset1;
	ep.x = D2Joints[JointType_SpineShoulder].x; ep.y = D2Joints[JointType_SpineShoulder].y-offset1;
	if (!IsFinitePoints(sp, ep)) return false;
	saveSegments(1, segments, segmentInfos, sp, ep, JointType_Neck, JointType_SpineShoulder);
	*/
	/*	//#3
	sp.x = D2Joints[JointType_SpineShoulder].x; sp.y = D2Joints[JointType_SpineShoulder].y+offset1;
	ep.x = D2Joints[JointType_SpineMid].x; ep.y = D2Joints[JointType_SpineMid].y-offset1;
	if (!IsFinitePoints(sp, ep)) return false;
	saveSegments(2, segments, segmentInfos, sp, ep, JointType_SpineShoulder, JointType_SpineMid);

	//#4
	sp.x = D2Joints[JointType_SpineMid].x; sp.y = D2Joints[JointType_SpineMid].y+offset1;
	ep.x = D2Joints[JointType_SpineBase].x; ep.y = D2Joints[JointType_SpineBase].y-offset1;
	if (!IsFinitePoints(sp, ep)) return false;
	saveSegments(3, segments, segmentInfos, sp, ep, JointType_SpineMid, JointType_SpineBase);
	*/
	/*	//#1
	sp.x = D2Joints[JointType_SpineShoulder].x; sp.y = D2Joints[JointType_SpineShoulder].y;
	ep.x = D2Joints[JointType_ShoulderRight].x; ep.y = D2Joints[JointType_ShoulderRight].y;
	if (!IsFinitePoints(sp, ep)) return false;
	saveSegments(0, segments, segmentInfos, sp, ep, JointType_SpineShoulder, JointType_ShoulderRight, false);

	//#2
	sp.x = D2Joints[JointType_SpineShoulder].x; sp.y = D2Joints[JointType_SpineShoulder].y;
	ep.x = D2Joints[JointType_ShoulderLeft].x; ep.y = D2Joints[JointType_ShoulderLeft].y;
	if (!IsFinitePoints(sp, ep)) return false;
	saveSegments(1, segments, segmentInfos, sp, ep, JointType_SpineShoulder, JointType_ShoulderLeft, true);
	*/
	//#1
	sp.x = D2Joints[JointType_SpineBase].x; sp.y = D2Joints[JointType_SpineBase].y;
	ep.x = D2Joints[JointType_HipRight].x; ep.y = D2Joints[JointType_HipRight].y;
	if (!IsFinitePoints(sp, ep)) return false;
	saveSegments(0, segments, segmentInfos, sp, ep, JointType_SpineBase, JointType_HipRight, false);

	//#2
	sp.x = D2Joints[JointType_SpineBase].x; sp.y = D2Joints[JointType_SpineBase].y;
	ep.x = D2Joints[JointType_HipLeft].x; ep.y = D2Joints[JointType_HipLeft].y;
	if (!IsFinitePoints(sp, ep)) return false;
	saveSegments(1, segments, segmentInfos, sp, ep, JointType_SpineBase, JointType_HipLeft, true);

	//#3
	sp.x = D2Joints[JointType_ShoulderRight].x; sp.y = D2Joints[JointType_ShoulderRight].y;
	ep.x = D2Joints[JointType_ElbowRight].x; ep.y = D2Joints[JointType_ElbowRight].y;
	if (!IsFinitePoints(sp, ep)) return false;
	saveSegments(2, segments, segmentInfos, sp, ep, JointType_ShoulderRight, JointType_ElbowRight, false);

	//#4
	sp.x = D2Joints[JointType_ElbowRight].x; sp.y = D2Joints[JointType_ElbowRight].y;
	ep.x = D2Joints[JointType_WristRight].x; ep.y = D2Joints[JointType_WristRight].y;
	if (!IsFinitePoints(sp, ep)) return false;
	saveSegments(3, segments, segmentInfos, sp, ep, JointType_ElbowRight, JointType_WristRight, false);

	//#5
	sp.x = D2Joints[JointType_WristRight].x; sp.y = D2Joints[JointType_WristRight].y;
	ep.x = D2Joints[JointType_HandRight].x; ep.y = D2Joints[JointType_HandRight].y;
	if (!IsFinitePoints(sp, ep)) return false;
	saveSegments(4, segments, segmentInfos, sp, ep, JointType_WristRight, JointType_HandRight, false);

	/*	//#11
	sp.x = D2Joints[JointType_WristRight].x+offset1; sp.y = D2Joints[JointType_WristRight].y;
	ep.x = D2Joints[JointType_HandRight].x; ep.y = D2Joints[JointType_HandRight].y;
	if (!IsFinitePoints(sp, ep)) return false;

	*/
	/*	//#11
	sp.x = D2Joints[JointType_HandRight].x+offset1; sp.y = D2Joints[JointType_HandRight].y;
	ep.x = D2Joints[JointType_HandTipRight].x+offset2; ep.y = D2Joints[JointType_HandTipRight].y;
	if (!IsFinitePoints(sp, ep)) return false;
	saveSegments(10, segments, segmentInfos, sp, ep, JointType_HandRight, JointType_HandTipRight);
	*/
	//#6
	sp.x = D2Joints[JointType_ShoulderLeft].x; sp.y = D2Joints[JointType_ShoulderLeft].y;
	ep.x = D2Joints[JointType_ElbowLeft].x; ep.y = D2Joints[JointType_ElbowLeft].y;
	if (!IsFinitePoints(sp, ep)) return false;
	saveSegments(5, segments, segmentInfos, sp, ep, JointType_ShoulderLeft, JointType_ElbowLeft, true);

	//#7
	sp.x = D2Joints[JointType_ElbowLeft].x; sp.y = D2Joints[JointType_ElbowLeft].y;
	ep.x = D2Joints[JointType_WristLeft].x; ep.y = D2Joints[JointType_WristLeft].y;
	if (!IsFinitePoints(sp, ep)) return false;
	saveSegments(6, segments, segmentInfos, sp, ep, JointType_ElbowLeft, JointType_WristLeft, true);

	//#8
	sp.x = D2Joints[JointType_WristLeft].x; sp.y = D2Joints[JointType_WristLeft].y;
	ep.x = D2Joints[JointType_HandLeft].x; ep.y = D2Joints[JointType_HandLeft].y;
	if (!IsFinitePoints(sp, ep)) return false;
	saveSegments(7, segments, segmentInfos, sp, ep, JointType_WristLeft, JointType_HandLeft, true);

	/*	//#11
	sp.x = D2Joints[JointType_WristLeft].x-offset1; sp.y = D2Joints[JointType_WristLeft].y;
	ep.x = D2Joints[JointType_HandLeft].x; ep.y = D2Joints[JointType_HandLeft].y;
	if (!IsFinitePoints(sp, ep)) return false;
	saveSegments(12, segments, segmentInfos, sp, ep, JointType_WristLeft, JointType_HandLeft);
	*/
	/*	//#16
	sp.x = D2Joints[JointType_HandLeft].x-offset1; sp.y = D2Joints[JointType_HandLeft].y;
	ep.x = D2Joints[JointType_HandTipLeft].x-offset2; ep.y = D2Joints[JointType_HandTipLeft].y;
	if (!IsFinitePoints(sp, ep)) return false;
	*/
	//#9
	sp.x = D2Joints[JointType_HipRight].x; sp.y = D2Joints[JointType_HipRight].y;
	ep.x = D2Joints[JointType_KneeRight].x; ep.y = D2Joints[JointType_KneeRight].y;
	if (!IsFinitePoints(sp, ep)) return false;
	saveSegments(8, segments, segmentInfos, sp, ep, JointType_HipRight, JointType_KneeRight, false);

	//#10
	sp.x = D2Joints[JointType_KneeRight].x; sp.y = D2Joints[JointType_KneeRight].y;
	ep.x = D2Joints[JointType_AnkleRight].x; ep.y = D2Joints[JointType_AnkleRight].y;
	if (!IsFinitePoints(sp, ep)) return false;
	saveSegments(9, segments, segmentInfos, sp, ep, JointType_KneeRight, JointType_AnkleRight, false);

	//#11
	sp.x = D2Joints[JointType_AnkleRight].x; sp.y = D2Joints[JointType_AnkleRight].y;
	ep.x = D2Joints[JointType_FootRight].x; ep.y = D2Joints[JointType_FootRight].y;
	if (!IsFinitePoints(sp, ep)) return false;
	saveSegments(10, segments, segmentInfos, sp, ep, JointType_AnkleRight, JointType_FootRight, false);

	//#12
	sp.x = D2Joints[JointType_HipLeft].x; sp.y = D2Joints[JointType_HipLeft].y;
	ep.x = D2Joints[JointType_KneeLeft].x; ep.y = D2Joints[JointType_KneeLeft].y;
	if (!IsFinitePoints(sp, ep)) return false;
	saveSegments(11, segments, segmentInfos, sp, ep, JointType_HipLeft, JointType_KneeLeft, true);

	//#13
	sp.x = D2Joints[JointType_KneeLeft].x; sp.y = D2Joints[JointType_KneeLeft].y;
	ep.x = D2Joints[JointType_AnkleLeft].x; ep.y = D2Joints[JointType_AnkleLeft].y;
	if (!IsFinitePoints(sp, ep)) return false;
	saveSegments(12, segments, segmentInfos, sp, ep, JointType_KneeLeft, JointType_AnkleLeft, true);

	//#14
	sp.x = D2Joints[JointType_AnkleLeft].x; sp.y = D2Joints[JointType_AnkleLeft].y;
	ep.x = D2Joints[JointType_FootLeft].x; ep.y = D2Joints[JointType_FootLeft].y;
	if (!IsFinitePoints(sp, ep)) return false;
	saveSegments(13, segments, segmentInfos, sp, ep, JointType_AnkleLeft, JointType_FootLeft, true);

	return true;
}

bool CNMsgrClntDlg::GetBoundingBox(Rect* r, Point2d* D2Joints)
{
	int i;
	float left, top, right, bottom;

	left = 1000; top = 1000;
	right = 0; bottom = 0;

	for (i = 0; i < 24; i++) {
		if (!IsFiniteNumber(D2Joints[i].x)) return false;
		if (!IsFiniteNumber(D2Joints[i].y)) return false;

		if (D2Joints[i].x < left) left = D2Joints[i].x;
		if (D2Joints[i].x > right) right = D2Joints[i].x;
		if (D2Joints[i].y < top) top = D2Joints[i].y;
		if (D2Joints[i].y > bottom) bottom = D2Joints[i].x;
	}
	r->x = left;
	r->y = top;
	r->width = right - left;
	r->height = bottom - top;

	return true;
}

float CNMsgrClntDlg::GetAverageDepth(Joint* D3Joints)
{
	int i;
	float depth;

	depth = 0.0;
	for (i = 0; i < 24; i++) {
		depth = depth + D3Joints[i].Position.Z;
	}

	depth = depth / 24.0;
	return depth;
}

bool CNMsgrClntDlg::valueInRange(int value, int min, int max)
{ 
	return (value >= min) && (value <= max); 
}

bool CNMsgrClntDlg::rectOverlap(Rect A, Rect B)
{
	bool xOverlap = valueInRange(A.x, B.x, B.x + B.width) ||
		valueInRange(B.x, A.x, A.x + A.width);

	bool yOverlap = valueInRange(A.y, B.y, B.y + B.height) ||
		valueInRange(B.y, A.y, A.y + A.height);

	return xOverlap && yOverlap;
}