
// NMsgrSvrDlg.cpp : 구현 파일
//

#include "stdafx.h"
#include "NMsgrSvr.h"
#include "NMsgrSvrDlg.h"
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


// CNMsgrSvrDlg 대화 상자




CNMsgrSvrDlg::CNMsgrSvrDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CNMsgrSvrDlg::IDD, pParent)
	, m_strStatus(_T(""))
	, m_strFileName(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_bStartSync = false;
	m_bFirstSync = true;
	m_pPrimeChildSocket = NULL;

	//ref body arr initialize
	for(int i = 0; i < BODY_COUNT; i++)	m_upperIdxarr[i] = -1;

	FaceDetected = cvCreateImage(cvSize(64*KINECT_COUNT,64*BODY_COUNT), IPL_DEPTH_8U, 1);
	t_Ref = cvCreateImage(cvSize(64*KINECT_COUNT, 64*BODY_COUNT), IPL_DEPTH_8U, 1);
}

void CNMsgrSvrDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_KINECT, m_Edit_Kinect);
	DDX_Control(pDX, IDC_EDIT_STATUS, m_Edit_Status);
	DDX_Text(pDX, IDC_EDIT_STATUS, m_strStatus);
	DDX_Control(pDX, IDC_EDIT_FILE_NAME, m_fileName);
	DDX_Text(pDX, IDC_EDIT_FILE_NAME, m_strFileName);
}


BEGIN_MESSAGE_MAP(CNMsgrSvrDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BT_GET_IP, &CNMsgrSvrDlg::OnBnClickedBtGetIp)
	ON_BN_CLICKED(IDOK, &CNMsgrSvrDlg::OnBnClickedOk)

	ON_MESSAGE(UM_GET_KINECT_DATA, OnGetKinectData)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BTN_WRITE_START, &CNMsgrSvrDlg::OnBnClickedBtnWriteStart)
	ON_BN_CLICKED(IDC_BTN_FILE_CLOSE, &CNMsgrSvrDlg::OnBnClickedBtnFileClose)
END_MESSAGE_MAP()


// CNMsgrSvrDlg 메시지 처리기

BOOL CNMsgrSvrDlg::OnInitDialog()
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

	InitializeCriticalSection(&m_cs);

	int i;
	//	for (i=0; i < KINECT_COUNT+1; i++)
	//		m_whiteImage[i].create(KINECT_DEPTH_HEIGHT, KINECT_DEPTH_WIDTH, CV_8UC3);

	m_ReadBody.OpenFile(NULL, "skeletons/Test3", 'r');
	//	m_RecordBody.OpenFile(NULL, "skeletons/Test", 'w');

#ifdef KINECTALL_MODE
	m_BodyRenderer.InitializeRenderer(NUM_KINECTS, "Kinect All", 5, 30);
	m_BodyRenderer.WaitUntilThreadInit();
#endif
	m_RefineRenderer.InitializeRenderer(1, "Refined Skeleton Tracking", 660, 550);
	m_RefineRenderer.WaitUntilThreadInit();
#ifdef AVERAGE_MODE
	m_AverRenderer.InitializeRenderer(1, "Average Kinect", 5, 550);
	m_AverRenderer.WaitUntilThreadInit();
#endif

#ifdef MAINCAM_MODE
	m_CenterRenderer.InitializeRenderer(1, "ID:3 Kinect", 660, 30);
	m_CenterRenderer.WaitUntilThreadInit();
#endif


	//	double* vel = new double[57*20];
	//	memset(vel, 0x00, sizeof(double)*57*20);
	//	m_filteredNoisyVel = m_MATLABConn.LPFiteringNoisyVel(vel, double(20), double(57));
	/*
	m_noisyVel = NULL;
	m_filteredNoisyVel = NULL;
	m_noisyVel = m_MATLABConn.ReadMatFile("matlab.mat");


	GetSystemTime(&m_st_curr);
	memcpy(&m_st_prev, &m_st_curr, sizeof(SYSTEMTIME));


	*/
	//Window 위치 수정
	SetWindowPos(NULL, 1308, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	prevsync.numObservedBody = -1;
	for(int i = 0; i < BODY_COUNT; i++){
		prevsync.bObsevedBody[i] = false;
		prevsync.syncSkels[i].numObservingKinect = 0;

		for(int j = 0; j < KINECT_COUNT; j++){
			prevsync.syncSkels[i].bObsevingKinect[j] = false;
		}
	}

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

double* CNMsgrSvrDlg::calcFilteredVel(double* vel, int dataLen, int dim)
{

	GetSystemTime(&m_st_curr);
	memcpy(&m_st_prev, &m_st_curr, sizeof(SYSTEMTIME));

	//	m_filteredNoisyVel = m_MATLABConn.LPFiteringNoisyVel(vel, double(dataLen), double(dim));

	GetSystemTime(&m_st_curr);
	long tdiff = abs (time_differnece(m_st_prev, m_st_curr) / 10000);

	CString str;
	str.Format("tdiff=%d",tdiff);
	//	this->PrintStatus(str);

	//	delete m_noisyVel;

	return m_filteredNoisyVel;

}

void CNMsgrSvrDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CNMsgrSvrDlg::OnPaint()
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
HCURSOR CNMsgrSvrDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CNMsgrSvrDlg::OnBnClickedBtGetIp()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	int i;
	CString str;
	m_ServerSocket.SetMainDlg(this);
	bool success = m_ServerSocket.InitListen();

	if(success) {
		str = "server socket initialization success";		
	}else{
		str = "server socket initialization failure";
	}

	PrintStatus(str);

	CString ip = "";
	char szHostName[256];
	struct hostent *pHostInfo;

	if( gethostname(szHostName,sizeof(szHostName)) ==0)
	{
		if((pHostInfo = gethostbyname(szHostName)) != NULL)
		{
			for(int i = 0; i < 1; i++) {
				ip.Format("%u.%u.%u.%u", 
					0xff & pHostInfo->h_addr_list[i][0], 
					0xff & pHostInfo->h_addr_list[i][1], 
					0xff & pHostInfo->h_addr_list[i][2],
					0xff & pHostInfo->h_addr_list[i][3]);
			}
		}
	}
	ip = "Server IP : " + ip;
	PrintStatus(ip);

	//m_bStartSync = true;
	//m_bStartSync = true;		// pyb
	//	m_bStartSync = true;

	for(i=0; i < BODY_COUNT; i++)
		m_KF[i].Initialize(this);

	m_hThread = (HANDLE)_beginthreadex(NULL, 0, SyncSekeletonThread, this, 0, (unsigned*)&m_dwThreadID);
	if(m_hThread == 0)
	{
		str = "_beginthreadex() error";
		PrintStatus(str);
	}
}


unsigned __stdcall CNMsgrSvrDlg::SyncSekeletonThread(void* pArg)
{
	CNMsgrSvrDlg *pDlg = (CNMsgrSvrDlg*)pArg;
	Point2d* D2Joints;
	Joint* D3Joints;
	bool bResult =true, bResult2 = false;
	CString str;
	int count=0, count2=0, i, j, k;
	POSITION pos;
	synconizedSkeletonsAll *syncSkelAll, *prevSyncSkelAll;
	_int64 tdiff;
	double tTick = (double)getTickCount();;

	//	GetSystemTime(&pDlg->m_st_curr);
	//	memcpy(&pDlg->m_st_prev, &pDlg->m_st_curr, sizeof(SYSTEMTIME));
	//	memset(pDlg->m_SkeletonsInfo, 0x00, sizeof(SkeletonInfo)*KINECT_COUNT);

	//	bool success = pDlg->m_MATLABConn.initialize();

	float diffX;
	float diffY;
	float diffZ;
	int c=0;
	int upperid, lowerid;

	DWORD dwFrames = 0;  
	DWORD dwCurrentTime = 0;  
	DWORD dwLastUpdateTime = 0;  
	DWORD dwElapsedTime = 0;  
	int index[19];
	pDlg->m_KF[0].GetActualJointIdx(index);
	memset(pDlg->m_checkNumBodyExist, 0x00, sizeof(int)*BODY_COUNT);

	velVector *StateVec;
	float alpha = 0.7;
	bool ret;

	SkeletonInfo *pSkeletonInfo, tmpSkeletonInfo;

	while(1) {
		if (pDlg->m_bStartSync == true) {
			//	if( count < 100) continue;
			EnterCriticalSection(&pDlg->m_cs);
			memset(pDlg->m_SkeletonsInfo, 0x00, sizeof(SkeletonInfo)*KINECT_COUNT);
//			pDlg->m_ReadBody.ReadBodyInfo(pDlg->m_SkeletonsInfo);  
//			bResult = true;
						bResult = pDlg->syncTimeSkeletonData(pArg);   //pyb  
			GetSystemTime(&pDlg->m_st_curr);

			memcpy(&pDlg->m_st_prev, &pDlg->m_st_curr, sizeof(SYSTEMTIME));
			dwFrames++;

//			if(dwFrames < 1500) continue;

//			if (dwFrames == 3483) //2031)
//				int aaa = 0;

			if(!bResult)  {
				//tTick = -1.0f;
				LeaveCriticalSection(&pDlg->m_cs);
				continue;
			}else{
				//				pDlg->m_RecordBody.WriteBodyInfo(pDlg->m_SkeletonsInfo);
				//TRACE("WriteBodyInfo\n");
			}

			if(bResult) {
				if (pDlg->m_bFirstSync) {
					bResult2 = pDlg->firstSyncBodySkeletonData(pArg);	
					if(bResult2)
						pDlg->m_bFirstSync = false;
					else
						continue;
				}else {
					//					TRACE("before syncBodySkeletonData\n");
					memset(pDlg->m_BodyDisappear, 0x00, sizeof(int)*BODY_COUNT);
					bResult2 = pDlg->syncBodySkeletonData(pArg);
					if(!bResult2) {
						pDlg->m_bFirstSync = true;
						memset(pDlg->m_BodyDisappear, 0x00, sizeof(int)*BODY_COUNT);
						memset(pDlg->m_checkNumBodyExist, 0x00, sizeof(int)*BODY_COUNT);
						for(j=0; j<BODY_COUNT; j++) {
							pDlg->m_KF[j].frameNum = 0;
							memset(pDlg->m_KF[j].m_boneLength, 0x00, sizeof(float)*Actual_BoneType_Count);
						}
					}
					//					TRACE("after syncBodySkeletonData\n");

				}
			}
			LeaveCriticalSection(&pDlg->m_cs);

			if ( bResult2 == false) {
				str.Format("Kinect fusion failure");
			}else{
				str.Format("Kinect fusion success");	

				for(i = 0; i < BODY_COUNT; i++) {
					if(pDlg->m_BodyDisappear[i] == 0  || pDlg->m_BodyDisappear[i] == 4)
						pDlg->m_checkNumBodyExist[i] = 0;
					if(pDlg->m_BodyDisappear[i] == 1)
						pDlg->m_checkNumBodyExist[i]++;
					if(pDlg->m_BodyDisappear[i] == 2 || pDlg->m_BodyDisappear[i] == 3) {
						pDlg->m_checkNumBodyExist[i] = 0;;
						pDlg->m_KF[i].frameNum = 0;
						memset(pDlg->m_KF[i].m_boneLength, 0x00, sizeof(float)*Actual_BoneType_Count);
					}
				}

				pos = pDlg->m_syncSkelList.GetTailPosition();
				syncSkelAll = pDlg->m_syncSkelList.GetPrev(pos);

				memset(pDlg->m_SkeletonTrackingAll, 0x00, sizeof(StateVector)*4);

				_int64 tdiff_prev, tdiff;
				pos = pDlg->m_ServerSocket.m_motive_socket->m_skeletonList.GetTailPosition();

				tdiff_prev = 3000;
				bool bfind = false;
				
				while(pos) {	
					pSkeletonInfo = (SkeletonInfo*)pDlg->m_ServerSocket.m_motive_socket->m_skeletonList.GetPrev(pos);
					//memcpy(&tmpSkeletonInfo, pSkeletonInfo, sizeof(SkeletonInfo));
					tdiff = abs (pDlg->time_differnece(syncSkelAll->st, pSkeletonInfo->st) / 10000.0);

					if (tdiff >= tdiff_prev) break;

					if (tdiff < 200 && tdiff < tdiff_prev) {
						bfind = true;
						tdiff_prev = tdiff;
						memcpy(&tmpSkeletonInfo, pSkeletonInfo, sizeof(SkeletonInfo));

					}
				}
				if(bfind) {
					for (k=0; k<Actual_JointType_Count; k++){
						pDlg->m_SkeletonTrackingAll[0].joints[k].X = tmpSkeletonInfo.InfoBody[0].JointPos[index[k]].Position.X;
						pDlg->m_SkeletonTrackingAll[0].joints[k].Y = tmpSkeletonInfo.InfoBody[0].JointPos[index[k]].Position.Y;
						pDlg->m_SkeletonTrackingAll[0].joints[k].Z = tmpSkeletonInfo.InfoBody[0].JointPos[index[k]].Position.Z;
						pDlg->m_SkeletonsInfo[1].InfoBody[0].JointPos[index[k]].Position.X = tmpSkeletonInfo.InfoBody[0].JointPos[index[k]].Position.X;
						pDlg->m_SkeletonsInfo[1].InfoBody[0].JointPos[index[k]].Position.Y = tmpSkeletonInfo.InfoBody[0].JointPos[index[k]].Position.Y;
						pDlg->m_SkeletonsInfo[1].InfoBody[0].JointPos[index[k]].Position.Z = tmpSkeletonInfo.InfoBody[0].JointPos[index[k]].Position.Z;
					}
					pDlg->m_SkeletonsInfo[1].Count = 1;
					pDlg->m_SkeletonsInfo[1].InfoBody[0].BodyID = 1;
				}
				/*Body rotation*/
				//				pDlg->m_RefineRenderer.SetBodyInfo(pDlg->m_SkeletonsInfo);							//before rotation
				int t = GetTickCount();
				pDlg->refineBodyDir(syncSkelAll);
				t = GetTickCount() - t;

				/*memset(pDlg->m_SkeletonsInfo, 0x00, sizeof(SkeletonInfo)*KINECT_COUNT);

				int idx1, idx2;
				idx1 = 0;
				for(i = 0; i < BODY_COUNT; i++) {
				idx2 = 0;
				if(syncSkelAll->bObsevedBody[i]) {
				pDlg->m_SkeletonsInfo[idx1].Count = syncSkelAll->syncSkels[i].numObservingKinect;
				for(j = 0; j < BODY_COUNT; j++) {
				if(syncSkelAll->syncSkels[i].bObsevingKinect[j])
				memcpy(&pDlg->m_SkeletonsInfo[idx1].InfoBody[idx2++],  &syncSkelAll->syncSkels[i].InfoBody[j], sizeof(BodyInfo));
				}
				}
				idx1++;
				}*/

				//pDlg->m_BodyRenderer.SetBodyInfo(pDlg->m_SkeletonsInfo);
				//pDlg->m_RefineRenderer.SetBodyInfo(&pDlg->m_SkeletonsInfo[2]);

#ifdef MAINCAM_MODE
				SkeletonInfo tCenterskelInfo;
				pDlg->FindsyncSkel(*syncSkelAll, &tCenterskelInfo, 3);
				pDlg->m_CenterRenderer.SetBodyInfo(&tCenterskelInfo);
				pDlg->stateRecord.WriteCenter(tCenterskelInfo);

				for (k=0; k<Actual_JointType_Count; k++){
					pDlg->m_SkeletonTrackingAll[1].joints[k].X = tCenterskelInfo.InfoBody[0].JointPos[index[k]].Position.X;
					pDlg->m_SkeletonTrackingAll[1].joints[k].Y = tCenterskelInfo.InfoBody[0].JointPos[index[k]].Position.Y;
					pDlg->m_SkeletonTrackingAll[1].joints[k].Z = tCenterskelInfo.InfoBody[0].JointPos[index[k]].Position.Z;
				}
#endif

#ifdef AVERAGE_MODE
				//Body Average All
				SkeletonInfo tAverskelInfo;
				/*pDlg->FindsyncSkel(*syncSkelAll, &tAverskelInfo, 3);*/
				pDlg->AverBody(*syncSkelAll, &tAverskelInfo);
				//pDlg->FindsyncSkel(*syncSkelAll, &tAverskelInfo, 3);
				pDlg->m_AverRenderer.SetBodyInfo(&tAverskelInfo);
				pDlg->stateRecord.WriteAver(tAverskelInfo);

				for (k=0; k<Actual_JointType_Count; k++){
					pDlg->m_SkeletonTrackingAll[2].joints[k].X = tAverskelInfo.InfoBody[0].JointPos[index[k]].Position.X;
					pDlg->m_SkeletonTrackingAll[2].joints[k].Y = tAverskelInfo.InfoBody[0].JointPos[index[k]].Position.Y;
					pDlg->m_SkeletonTrackingAll[2].joints[k].Z = tAverskelInfo.InfoBody[0].JointPos[index[k]].Position.Z;
				}

#endif

#ifdef KINECTALL_MODE
				pDlg->m_BodyRenderer.SetBodyInfo(pDlg->m_SkeletonsInfo);
				CString str;
				str.Format("[%d] frame, refID : %d", dwFrames, pDlg->m_upperIdxarr[0]);
				pDlg->PrintStatus(str);
#endif
				int a = pDlg->m_syncSkelList.GetCount();
				pos = pDlg->m_syncSkelList.GetTailPosition();
				syncSkelAll = pDlg->m_syncSkelList.GetPrev(pos);
				if(pDlg->m_syncSkelList.GetCount() > 1)
					prevSyncSkelAll = pDlg->m_syncSkelList.GetPrev(pos);
				else
					prevSyncSkelAll = syncSkelAll;

				tdiff = abs (pDlg->time_differnece(prevSyncSkelAll->st, syncSkelAll->st) / 10000);
				//			pDlg->m_RecordBody.WriteSyncSkelsAllInfo(syncSkelAll);

				if(tdiff == 0) continue;
/*
				float dist1, dist2, dist3, dist4, dist5;

				if (syncSkelAll->syncSkels[0].bObsevingKinect[0]) {
					dist1 = pow(syncSkelAll->syncSkels[0].InfoBody[0].JointPos[5].Position.X-syncSkelAll->syncSkels[0].InfoBody[0].JointPos[6].Position.X,2);
					dist1 = dist1 + pow(syncSkelAll->syncSkels[0].InfoBody[0].JointPos[5].Position.Y-syncSkelAll->syncSkels[0].InfoBody[0].JointPos[6].Position.Y,2);
					dist1 = dist1 + pow(syncSkelAll->syncSkels[0].InfoBody[0].JointPos[5].Position.Z-syncSkelAll->syncSkels[0].InfoBody[0].JointPos[6].Position.Z,2);
					dist1 = sqrt(dist1);
				}
				if (syncSkelAll->syncSkels[0].bObsevingKinect[1]) {
					dist2 = pow(syncSkelAll->syncSkels[0].InfoBody[1].JointPos[5].Position.X-syncSkelAll->syncSkels[0].InfoBody[1].JointPos[6].Position.X,2);
					dist2 = dist2 + pow(syncSkelAll->syncSkels[0].InfoBody[1].JointPos[5].Position.Y-syncSkelAll->syncSkels[0].InfoBody[1].JointPos[6].Position.Y,2);
					dist2 = dist2 + pow(syncSkelAll->syncSkels[0].InfoBody[1].JointPos[5].Position.Z-syncSkelAll->syncSkels[0].InfoBody[1].JointPos[6].Position.Z,2);
					dist2 = sqrt(dist2);
				}
				if (syncSkelAll->syncSkels[0].bObsevingKinect[2]) {
					dist3 = pow(syncSkelAll->syncSkels[0].InfoBody[2].JointPos[5].Position.X-syncSkelAll->syncSkels[0].InfoBody[2].JointPos[6].Position.X,2);
					dist3 = dist3 + pow(syncSkelAll->syncSkels[0].InfoBody[2].JointPos[5].Position.Y-syncSkelAll->syncSkels[0].InfoBody[2].JointPos[6].Position.Y,2);
					dist3 = dist3 + pow(syncSkelAll->syncSkels[0].InfoBody[2].JointPos[5].Position.Z-syncSkelAll->syncSkels[0].InfoBody[2].JointPos[6].Position.Z,2);
					dist3 = sqrt(dist3);
				}
				if (syncSkelAll->syncSkels[0].bObsevingKinect[3]) {
					dist4 = pow(syncSkelAll->syncSkels[0].InfoBody[3].JointPos[5].Position.X-syncSkelAll->syncSkels[0].InfoBody[3].JointPos[6].Position.X,2);
					dist4 = dist4 + pow(syncSkelAll->syncSkels[0].InfoBody[3].JointPos[5].Position.Y-syncSkelAll->syncSkels[0].InfoBody[3].JointPos[6].Position.Y,2);
					dist4 = dist4 + pow(syncSkelAll->syncSkels[0].InfoBody[3].JointPos[5].Position.Z-syncSkelAll->syncSkels[0].InfoBody[3].JointPos[6].Position.Z,2);
					dist4 = sqrt(dist4);
				}
				if (syncSkelAll->syncSkels[0].bObsevingKinect[4]) {
					dist5 = pow(syncSkelAll->syncSkels[0].InfoBody[4].JointPos[5].Position.X-syncSkelAll->syncSkels[0].InfoBody[4].JointPos[6].Position.X,2);
					dist5 = dist5 + pow(syncSkelAll->syncSkels[0].InfoBody[4].JointPos[5].Position.Y-syncSkelAll->syncSkels[0].InfoBody[4].JointPos[6].Position.Y,2);
					dist5 = dist5 + pow(syncSkelAll->syncSkels[0].InfoBody[4].JointPos[5].Position.Z-syncSkelAll->syncSkels[0].InfoBody[4].JointPos[6].Position.Z,2);
					dist5 = sqrt(dist5);
				}

				str.Format("%d,1=%f,2=%f,3=%f,4=%f,5=%f", pDlg->m_upperIdxarr[0]+1, dist1, dist2, dist3, dist4, dist5);
				pDlg->PrintStatus(str);
*/

				SkeletonInfo SystemOutput;
				//memset(pDlg->m_SkeletonsInfo, 0x00, sizeof(SkeletonInfo)*KINECT_COUNT);
				SystemOutput.Count = 0;
				count=0;
				for(j=0; j < BODY_COUNT; j++) {
					if(pDlg->m_checkNumBodyExist[j] > 100) {
						pDlg->m_KF[j].KFExec(&syncSkelAll->syncSkels[j], tdiff, j);

						SystemOutput.Count++;
						SystemOutput.InfoBody[j].BodyID = j+1;

						StateVec = new velVector;
						for (i=0; i<Actual_JointType_Count;i++){
							StateVec->Jointvels[i].dx = pDlg->m_KF[j].m_StateVector.joints[i].X ;
							StateVec->Jointvels[i].dy = pDlg->m_KF[j].m_StateVector.joints[i].Y ;
							StateVec->Jointvels[i].dz = pDlg->m_KF[j].m_StateVector.joints[i].Z ;
						}
						pDlg->m_KF[j].m_StateHytrVectors.push_back(StateVec);

						if (pDlg->m_KF[j].m_StateHytrVectors.size() > 1) {

							for (k=0; k<Actual_JointType_Count; k++){
								SystemOutput.InfoBody[count].JointPos[index[k]].Position.X = alpha * StateVec->Jointvels[k].dx + (1-alpha)*pDlg->m_KF[j].m_lastStateVector.joints[k].X;
								SystemOutput.InfoBody[count].JointPos[index[k]].Position.Y = alpha * StateVec->Jointvels[k].dy + (1-alpha)*pDlg->m_KF[j].m_lastStateVector.joints[k].Y;
								SystemOutput.InfoBody[count].JointPos[index[k]].Position.Z = alpha * StateVec->Jointvels[k].dz + (1-alpha)*pDlg->m_KF[j].m_lastStateVector.joints[k].Z;
							}

							StateVec = pDlg->m_KF[j].m_StateHytrVectors.front();
							delete StateVec;
							pDlg->m_KF[j].m_StateHytrVectors.erase(pDlg->m_KF[j].m_StateHytrVectors.begin());

							for (k=0; k<Actual_JointType_Count; k++){
								pDlg->m_KF[j].m_lastStateVector.joints[k].X = pDlg->m_KF[j].m_StateVector.joints[k].X;
								pDlg->m_KF[j].m_lastStateVector.joints[k].Y = pDlg->m_KF[j].m_StateVector.joints[k].Y;
								pDlg->m_KF[j].m_lastStateVector.joints[k].Z = pDlg->m_KF[j].m_StateVector.joints[k].Z;
							}
							for (k=0; k<Actual_JointType_Count; k++){
								pDlg->m_SkeletonTrackingAll[3].joints[k].X = pDlg->m_KF[j].m_lastStateVector.joints[k].X;
								pDlg->m_SkeletonTrackingAll[3].joints[k].Y = pDlg->m_KF[j].m_lastStateVector.joints[k].Y;
								pDlg->m_SkeletonTrackingAll[3].joints[k].Z = pDlg->m_KF[j].m_lastStateVector.joints[k].Z;
							}

							if(j == 0)
								pDlg->stateRecord.WriteState(pDlg->m_KF[j].m_lastStateVector);

							count++;
//							GetSystemTime(&pDlg->m_st_curr);
//							tdiff = abs (pDlg->time_differnece(pDlg->m_st_prev, pDlg->m_st_curr) / 10000);

						}else {
							for (int k=0; k<Actual_JointType_Count; k++){
								pDlg->m_KF[j].m_lastStateVector.joints[k].X = StateVec->Jointvels[k].dx;
								pDlg->m_KF[j].m_lastStateVector.joints[k].Y = StateVec->Jointvels[k].dy;
								pDlg->m_KF[j].m_lastStateVector.joints[k].Z = StateVec->Jointvels[k].dz;
							}

						}
/*						if (pDlg->m_KF[j].frameNum > 5) {
							ret = pDlg->checkValidate(&pDlg->m_KF[j].m_lastStateVector, j);

							if (!ret) {
								pDlg->m_KF[j].frameNum = 0;
								memset(pDlg->m_KF[j].m_boneLength, 0x00, sizeof(float)*Actual_BoneType_Count);
							}
						}*/
					}
				}
/*				CString str;
				diffX = pDlg->m_KF[0].m_StateVector.joints[10].X;
				diffY = pDlg->m_KF[0].m_StateVector.joints[10].Y;
				diffZ = pDlg->m_KF[0].m_StateVector.joints[10].Z;

				str.Format("KFX=%f, KFY=%f, KFZ=%f",diffX, diffY, diffZ);
				//				str.Format("tdiff=%d",dwFrames);
				pDlg->PrintStatus(str);*/
//				SystemOutput.InfoBody[0].JointPos[0].Position.X = 0.5;
				pDlg->stateRecord.WriteAll(pDlg->m_SkeletonTrackingAll);
				pDlg->m_RefineRenderer.SetBodyInfo(&SystemOutput);
			}
		}
	}
}

bool CNMsgrSvrDlg::checkValidate(StateVector* pStateVector, int bodyIdx) 
{

	float dist;
	float thresh = 0.1;

	dist = calculateABoneDist(pStateVector, bodyIdx, 0, 0, 1);
	if( dist > thresh) 
		return false;
	dist = calculateABoneDist(pStateVector, bodyIdx, 1, 1, 2);
	if( dist > thresh) 
		return false;
	dist = calculateABoneDist(pStateVector, bodyIdx, 2, 2, 3);
	if( dist > thresh) 
		return false;
	dist = calculateABoneDist(pStateVector, bodyIdx, 3, 18, 4);
	if( dist > thresh) 
		return false;
	dist = calculateABoneDist(pStateVector, bodyIdx, 4, 4, 5);
	if( dist > thresh) 
		return false;
	dist = calculateABoneDist(pStateVector, bodyIdx, 5, 5, 6);
	if( dist > thresh) 
		return false;
	dist = calculateABoneDist(pStateVector, bodyIdx, 6, 6, 7);
	if( dist > thresh) 
		return false;
	dist = calculateABoneDist(pStateVector, bodyIdx, 7, 18, 8);
	if( dist > thresh) 
		return false;
	dist = calculateABoneDist(pStateVector, bodyIdx, 8, 8, 9);
	if( dist > thresh) 
		return false;
	dist = calculateABoneDist(pStateVector, bodyIdx, 9, 9, 10);
	if( dist > thresh) 
		return false;
	dist = calculateABoneDist(pStateVector, bodyIdx, 10, 10, 11);
	if( dist > thresh) 
		return false;
	dist = calculateABoneDist(pStateVector, bodyIdx, 11, 0, 12);
	if( dist > thresh) 
		return false;
	dist = calculateABoneDist(pStateVector, bodyIdx, 12, 12, 13);
	if( dist > thresh) 
		return false;
	dist = calculateABoneDist(pStateVector, bodyIdx, 13, 13, 14);
	if( dist > thresh) 
		return false;
	dist = calculateABoneDist(pStateVector, bodyIdx, 14, 0, 15);
	if( dist > thresh) 
		return false;
	dist = calculateABoneDist(pStateVector, bodyIdx, 15, 15, 16);
	if( dist > thresh) 
		return false;
	dist = calculateABoneDist(pStateVector, bodyIdx, 16, 16, 17);
	if( dist > thresh) 
		return false;

/*	dist = calcDist(pStateVector, pBodyInfo, 4, 5);
	if( dist > thresh) return false;
	dist = calcDist(pStateVector, pBodyInfo, 5, 6);
	if( dist > thresh) return false;
	dist = calcDist(pStateVector, pBodyInfo, 6, 7);
	if( dist > thresh) return false;
	dist = calcDist(pStateVector, pBodyInfo, 18, 4);
	if( dist > thresh) return false;
	dist = calcDist(pStateVector, pBodyInfo, 8, 9);
	if( dist > thresh) return false;
	dist = calcDist(pStateVector, pBodyInfo, 9, 10);
	if( dist > thresh) return false;
	dist = calcDist(pStateVector, pBodyInfo, 10, 11);
	if( dist > thresh) return false;
	dist = calcDist(pStateVector, pBodyInfo, 18, 8);
	if( dist > thresh) return false;

	dist = calcDist(pStateVector, pBodyInfo, 12, 13);
	if( dist > thresh) return false;
	dist = calcDist(pStateVector, pBodyInfo, 13, 14);
	if( dist > thresh) return false;
	dist = calcDist(pStateVector, pBodyInfo, 16, 17);
	if( dist > thresh) return false;
	dist = calcDist(pStateVector, pBodyInfo, 17, 18);
	if( dist > thresh) return false;
*/
	return true;
}

float CNMsgrSvrDlg::calculateABoneDist(StateVector* pStateVector, int bodyIdx, int targetBoneIdx, int fromJointIdx, int toJintIdx)
{
	float dist = 0.0;
	float length = 0.0;

	length = pow(pStateVector->joints[fromJointIdx].X-pStateVector->joints[toJintIdx].X,2);
	length = length + pow(pStateVector->joints[fromJointIdx].Y-pStateVector->joints[toJintIdx].Y,2);
	length = length + pow(pStateVector->joints[fromJointIdx].Z-pStateVector->joints[toJintIdx].Z,2);
	length = sqrt(length);

	dist = fabs(m_KF[bodyIdx].m_boneLength[targetBoneIdx] - length);

	return dist;
}

float CNMsgrSvrDlg::calcDist(StateVector* pStateVector, BodyInfo* pBodyInfo, int fromIdx, int toIdx) 
{
	int index[19];
	m_KF[0].GetActualJointIdx(index);
	float distRef, distComp, dist;

	distRef = pow(pBodyInfo->JointPos[index[fromIdx]].Position.X - pBodyInfo->JointPos[index[toIdx]].Position.X, 2);
	distRef = distRef + pow(pBodyInfo->JointPos[index[fromIdx]].Position.Y - pBodyInfo->JointPos[index[toIdx]].Position.Y, 2);
	distRef = distRef + pow(pBodyInfo->JointPos[index[fromIdx]].Position.Z - pBodyInfo->JointPos[index[toIdx]].Position.Z, 2);
	distRef = sqrt(distRef);

	distComp = pow(pStateVector->joints[fromIdx].X - pStateVector->joints[toIdx].X, 2);
	distComp = distComp + pow(pStateVector->joints[fromIdx].Y - pStateVector->joints[toIdx].Y, 2);
	distComp = distComp + pow(pStateVector->joints[fromIdx].Z - pStateVector->joints[toIdx].Z, 2);
	distComp = sqrt(distComp);

	dist = fabs(distRef-distComp);

	return dist;
}

bool CNMsgrSvrDlg::syncTimeSkeletonData(void* pArg)
{
	CNMsgrSvrDlg *pDlg = (CNMsgrSvrDlg*)pArg;
	CChildSocket* pChildSock;
	POSITION pos, pos2, maxBodyPos;
	SkeletonInfo PrimeSkeletonInfo, curSkeletonInfo, tmpSkeletonInfo;
	SkeletonInfo *pSkeletonInfo;
	_int64 tdiff_prev, tdiff;
	CString str;
	int count, maxBodyCount = 0;
	int i, j;
	bool bfind = false;

	count = 0;

	memset(pDlg->m_SkeletonsInfo, 0x00, sizeof(SkeletonInfo)*KINECT_COUNT);

	if (pDlg->m_bFirstSync) {
		pos = pDlg->m_ServerSocket.m_childSocket_list.GetHeadPosition();
		while (pos)	{
			pos2 = pos;
			pChildSock = pDlg->m_ServerSocket.m_childSocket_list.GetNext(pos);
			if(pChildSock->m_bReceiveNewData == true && pChildSock->m_skelDataCount > 0){
				if (maxBodyCount <= pChildSock->m_LastSkeletonInfo.Count) {
					maxBodyCount = pChildSock->m_LastSkeletonInfo.Count;
					maxBodyPos = pos2;
					bfind = true;
				}
			}
			count++;
		}
		if (!bfind) 
			return false;

		pChildSock = pDlg->m_ServerSocket.m_childSocket_list.GetNext(maxBodyPos);
		m_pPrimeChildSocket = pChildSock;

	}else{
		if(m_pPrimeChildSocket->m_bReceiveNewData == false)
			return false;
	}

	count = 0;
	m_pPrimeChildSocket->m_bReceiveNewData = false;

	/* time sync first, body sync second*/
	memcpy(&PrimeSkeletonInfo, &m_pPrimeChildSocket->m_LastSkeletonInfo, sizeof(SkeletonInfo));
	memcpy(&pDlg->m_SkeletonsInfo[count], &PrimeSkeletonInfo, sizeof(SkeletonInfo));
	count++;

	pos = pDlg->m_ServerSocket.m_childSocket_list.GetHeadPosition();

	while (pos)
	{	
		pChildSock = pDlg->m_ServerSocket.m_childSocket_list.GetNext(pos);
		if (pChildSock == m_pPrimeChildSocket) continue;

		pos2 = pChildSock->m_skeletonList.GetTailPosition();

		tdiff_prev = 3000;
		bfind = false;
		while(pos2) {	
			int cnt = pChildSock->m_skeletonList.GetCount();
			pSkeletonInfo = (SkeletonInfo*)pChildSock->m_skeletonList.GetPrev(pos2);
			memcpy(&tmpSkeletonInfo, pSkeletonInfo, sizeof(SkeletonInfo));
			tdiff = abs (pDlg->time_differnece(PrimeSkeletonInfo.st, tmpSkeletonInfo.st) / 10000);

			if (tdiff >= tdiff_prev) break;

			if (tdiff < 200 && tdiff < tdiff_prev) {
				bfind = true;
				tdiff_prev = tdiff;
				memcpy(&curSkeletonInfo, &tmpSkeletonInfo, sizeof(SkeletonInfo));

			}

		}
		if(bfind) {						
			memcpy(&pDlg->m_SkeletonsInfo[count], &curSkeletonInfo, sizeof(SkeletonInfo));
			pChildSock->m_bReceiveNewData = false;
			count++;
		}else {
			str.Format("Can't execute syncronization of skeleton time");
			pDlg->PrintStatus(str);
			//return false;
		}
		//		pChildSock->m_bReceiveNewData = false;
	}

	return true;

}

bool CNMsgrSvrDlg::syncBodySkeletonData(void* pArg)
{
	CNMsgrSvrDlg *pDlg = (CNMsgrSvrDlg*)pArg;
	SkeletonInfo *pSkeletonInfo;
	synconizedSkeletonsAll *syncSkelAll, *prevSyncSkelAll;
	synconizedSkeletons		*syncSkel, *tmpSyncSkel;
	BodyInfo *refBody, *compBody, *tCompBody;
	Joint *refJoint, *compJoint;
	int i, j, k, l, m, count, numObservedBody=0, bodyCount[BODY_COUNT], hid;
	int matchedInfo[KINECT_COUNT][BODY_COUNT];     /* #-1 no data, #hid+1 matched , #0 unmatched*/
	double error, error2, minError;
	double errorThreshold = 0.35;
	bool bFindRefBody, bFindBody, bMatchedBody, bFind;
	int refKinectIdx;
	int matchedKinectIdx, matchedBodyIdx;
	POSITION pos;
	UINT64 BodyID;
	CString str;
	int kIdx;

	syncSkelAll = new synconizedSkeletonsAll;
	memset(syncSkelAll, 0x00, sizeof(synconizedSkeletonsAll));

	for (i = 0; i < KINECT_COUNT; i++) {
		if (pDlg->m_SkeletonsInfo[i].Kinect_ID == 3) {
			memcpy(&syncSkelAll->st, &pDlg->m_SkeletonsInfo[i].st,  sizeof(SYSTEMTIME));

		}
	}
	memset(matchedInfo, 0x00, sizeof(int)*KINECT_COUNT*BODY_COUNT);

	for (i = 0; i < KINECT_COUNT; i++) {
		count  = pDlg->m_SkeletonsInfo[i].Count;
		for (j = 0 ; j < BODY_COUNT; j ++) {
			matchedInfo[i][j] = 0;

			if (j >= count) matchedInfo[i][j] = -1; 
		}
	}

	/* uses previous body sync information*/
	pos = pDlg->m_syncSkelList.GetTailPosition();
	prevSyncSkelAll = pDlg->m_syncSkelList.GetPrev(pos);

	bFind = false;
	for (i = 0; i < BODY_COUNT; i ++) {
		syncSkel = new synconizedSkeletons;
		memset(syncSkel, 0x00, sizeof(synconizedSkeletons));
		syncSkel->numObservingKinect = 0;
		syncSkel->Hid = i+1;
		bodyCount[i] = 0; 

		if (prevSyncSkelAll->bObsevedBody[i] == false) {
			syncSkel->numObservingKinect = bodyCount[i];
			memcpy(&syncSkelAll->syncSkels[i], syncSkel, sizeof(synconizedSkeletons));
			syncSkelAll->bObsevedBody[i] = false;
			delete syncSkel;
			syncSkel = NULL;
			continue;
		}
		tmpSyncSkel = &(prevSyncSkelAll->syncSkels[i]);

		bFindBody = false;

		for (j = 0; j < KINECT_COUNT; j ++) {
			syncSkel->bObsevingKinect[j] = false;

			if (tmpSyncSkel->bObsevingKinect[j] == false) continue;
			if( bFindBody) break;
			BodyID = tmpSyncSkel->InfoBody[j].BodyID;

			for (k = 0; k < KINECT_COUNT; k++) {
				for (l = 0; l < BODY_COUNT; l++) {
					if (bFindBody) continue;

					if (pDlg->m_SkeletonsInfo[k].Count < l+1) continue;

					if (pDlg->m_SkeletonsInfo[k].InfoBody[l].BodyID == BodyID) {

						matchedInfo[k][l] = syncSkel->Hid;
						kIdx = pDlg->m_SkeletonsInfo[k].Kinect_ID-1;
						memcpy(&syncSkel->InfoBody[kIdx], &pDlg->m_SkeletonsInfo[k].InfoBody[l], sizeof(BodyInfo));
						syncSkel->bObsevingKinect[kIdx] = true;
						bodyCount[i]++;
						bFindBody = true;
						bFind = true;
					}
				}
			}	
		}

		if(!bFindBody) {
			pDlg->m_BodyDisappear[i] = 2;
		}else {
			pDlg->m_BodyDisappear[i] = 1;
		}
		syncSkel->numObservingKinect = bodyCount[i];
		memcpy(&syncSkelAll->syncSkels[i], syncSkel, sizeof(synconizedSkeletons));
		delete syncSkel;
		syncSkel = NULL;	

		if (bodyCount[i] > 0) {
			//			numObservedBody++;
			syncSkelAll->bObsevedBody[i] = true;
		}
		//		syncSkelAll->numObservedBody = numObservedBody;
	}

	if(!bFind)
		return false;		///  ????????????

	for (i = 0; i < BODY_COUNT; i ++) {
		syncSkel = &syncSkelAll->syncSkels[i];
		refBody = NULL;
		for (k = 0; k < KINECT_COUNT; k++) {
			if(syncSkel->bObsevingKinect[k]) {
				refBody = &syncSkel->InfoBody[k];
				break;
			}
		}

		if (refBody == NULL)
			continue;

		for (j = i+1; j < BODY_COUNT; j++) {
			tmpSyncSkel = &syncSkelAll->syncSkels[j];
			tCompBody = NULL;
			for (l = 0; l < KINECT_COUNT; l++) {
				if(tmpSyncSkel->bObsevingKinect[l]) {
					tCompBody = &tmpSyncSkel->InfoBody[l];
					break;
				}
			}
			if (tCompBody == NULL)
				continue;

			minError = 1000000.0; matchedKinectIdx=-1; matchedBodyIdx=-1;
			error = CalcDistanceTwoBodies(refBody, tCompBody);

			if (error < errorThreshold) {
				if (minError > error) {
					minError = error;
					matchedKinectIdx = k; 
					kIdx = l;
					matchedBodyIdx = i;
					compBody = tCompBody; 
				}
			}
			if (matchedKinectIdx != -1 && matchedBodyIdx != -1) {
				for (m = 0; m < BODY_COUNT; m++) {
					if (matchedInfo[l][m] == j+1)
						matchedInfo[l][m] = syncSkel->Hid;
				}
				memcpy(&syncSkel->InfoBody[kIdx], compBody, sizeof(BodyInfo));
				syncSkel->bObsevingKinect[kIdx] = true;
				bodyCount[i]++;
				syncSkel->numObservingKinect = bodyCount[i];
				memset(&tmpSyncSkel->InfoBody[l], 0x00, sizeof(BodyInfo));
				tmpSyncSkel->bObsevingKinect[l] = false;
				bodyCount[j]--;
				tmpSyncSkel->numObservingKinect = bodyCount[j];
				syncSkelAll->bObsevedBody[j] = false;
			}
		}
	}

	for (i = 0 ; i < BODY_COUNT; i ++) {

		if (!syncSkelAll->bObsevedBody[i])
			continue;

		syncSkel = &syncSkelAll->syncSkels[i];
		refKinectIdx = -1;

		/* find reference body*/
		for (j = 0; j < KINECT_COUNT; j++) {
			if (!syncSkel->bObsevingKinect[j]) continue;

			refBody = &syncSkel->InfoBody[j];
			bFindRefBody = true;
			refKinectIdx = j;
		}

		if(!bFindRefBody) {
			int aaa = 0;  // ???????????????????????
			continue;
		}
		/* find corresponding body*/		
		for (j = 0; j < KINECT_COUNT; j++) {
			minError = 1000000.0; matchedKinectIdx=-1; matchedBodyIdx=-1;

			for (k = 0; k < BODY_COUNT; k++) {
				if (matchedInfo[j][k] != 0) continue;
				if (refKinectIdx == pDlg->m_SkeletonsInfo[j].Kinect_ID-1) continue;

				tCompBody = &(pDlg->m_SkeletonsInfo[j].InfoBody[k]);
				error = CalcDistanceTwoBodies(refBody, tCompBody);

				if (error < errorThreshold) {
					if (minError > error) {
						minError = error;
						matchedKinectIdx = j; 
						kIdx = pDlg->m_SkeletonsInfo[j].Kinect_ID-1;
						matchedBodyIdx = k;
						compBody = tCompBody; 
					}
				}
			}
			if (matchedKinectIdx != -1 && matchedBodyIdx != -1) {
				matchedInfo[matchedKinectIdx][matchedBodyIdx] = 1;
				memcpy(&syncSkel->InfoBody[kIdx], compBody, sizeof(BodyInfo));
				syncSkel->bObsevingKinect[kIdx] = true;
				bodyCount[i]++;
			}
		}
		syncSkel->numObservingKinect = bodyCount[i];
		//		memcpy(&syncSkelAll->syncSkels[i], syncSkel, sizeof(synconizedSkeletons));
		//		delete syncSkel;
		//		syncSkel = NULL;

		if (bodyCount[i] > 0) {
			numObservedBody++;
			syncSkelAll->bObsevedBody[i] = true;
		}
	}

	int indexOfBodyTable;
	int indexOfKinect;
	bFindRefBody = false;
	/* make new body sync */
	for (i = 0; i < KINECT_COUNT; i++) {
		for (j = 0; j < BODY_COUNT; j++) {
			if (matchedInfo[i][j] == 0) {

				//				int cc = pDlg->m_ServerSocket.m_childSocket_list.GetCount();

				refBody = &(pDlg->m_SkeletonsInfo[i].InfoBody[j]);
				for (k = 0; k < BODY_COUNT; k++) {
					if(syncSkelAll->bObsevedBody[k] == false) {
						syncSkel = &syncSkelAll->syncSkels[k];
						kIdx = pDlg->m_SkeletonsInfo[i].Kinect_ID-1;
						memcpy(&syncSkel->InfoBody[kIdx], refBody, sizeof(BodyInfo));
						bodyCount[k]++;

						if(pDlg->m_BodyDisappear[k] == 2)
							pDlg->m_BodyDisappear[k] = 3;
						else if(pDlg->m_BodyDisappear[k] == 0)
							pDlg->m_BodyDisappear[k] = 4;

						syncSkel->numObservingKinect = bodyCount[k];
						syncSkel->bObsevingKinect[kIdx] = true;
						matchedInfo[i][j] = syncSkel->Hid;
						indexOfBodyTable = k;		// syncSkelAll 의 index
						indexOfKinect = i;			// m_SkeletonsInfo 의 index 
						bFindRefBody = true;
						break;
					}
				}

				minError = 1000000.0; matchedKinectIdx=-1; matchedBodyIdx=-1;
				for (k = 0; k < KINECT_COUNT ; k++) {
					for (l = 0; l < BODY_COUNT; l++) {
						if (matchedInfo[k][l] != 0) continue;					
						if (indexOfKinect == k) continue;

						tCompBody = &(pDlg->m_SkeletonsInfo[k].InfoBody[l]);
						error = CalcDistanceTwoBodies(refBody, tCompBody);

						if (error < errorThreshold) {
							if (minError > error) {
								minError = error;
								kIdx = pDlg->m_SkeletonsInfo[k].Kinect_ID-1;
								matchedKinectIdx = k; 
								matchedBodyIdx = l;	
								compBody = tCompBody;
							}
						}
					}
				}
				if (matchedKinectIdx != -1 && matchedBodyIdx != -1) {
					matchedInfo[matchedKinectIdx][matchedBodyIdx] = syncSkel->Hid;
					syncSkel->bObsevingKinect[kIdx] = true;
					memcpy(&syncSkel->InfoBody[kIdx], compBody, sizeof(BodyInfo));
					bodyCount[indexOfBodyTable]++;
					syncSkel->numObservingKinect = bodyCount[indexOfBodyTable];
				}
				if (bodyCount[indexOfBodyTable] > 0) {
					numObservedBody++;
					syncSkelAll->bObsevedBody[indexOfBodyTable] = true;
				}
			}
		}
	}

	syncSkelAll->numObservedBody = numObservedBody;
	//	memcpy(&syncSkelAll->st, &pDlg->m_SkeletonsInfo[0].st,  sizeof(SYSTEMTIME));

	pDlg->m_syncSkelList.AddTail(syncSkelAll);

	/*	str.Format("%d/%d/%d/%d/%d/%d\n", matchedInfo[0][0],matchedInfo[0][1],matchedInfo[0][2],matchedInfo[0][3],matchedInfo[0][4],matchedInfo[0][5]);
	pDlg->PrintStatus(str);
	str.Format("%d/%d/%d/%d/%d/%d\n", matchedInfo[1][0],matchedInfo[1][1],matchedInfo[1][2],matchedInfo[1][3],matchedInfo[1][4],matchedInfo[1][5]);
	pDlg->PrintStatus(str);
	*/
	synconizedSkeletonsAll *psyncSkelAll;
	synconizedSkeletons *psyncSkel;
	if (pDlg->m_syncSkelList.GetCount() > SYNC_SKELS_BUF_SIZE) {
		psyncSkelAll = pDlg->m_syncSkelList.GetHead();
		delete psyncSkelAll;
		pDlg->m_syncSkelList.RemoveHead();
	}
	return true;
}


bool CNMsgrSvrDlg::firstSyncBodySkeletonData(void* pArg)
{
	CNMsgrSvrDlg *pDlg = (CNMsgrSvrDlg*)pArg;
	SkeletonInfo *pSkeletonInfo;
	synconizedSkeletonsAll *syncSkelAll;
	synconizedSkeletons		*syncSkel;
	BodyInfo *refBody, *compBody, *tCompBody;
	Joint *refJoint, *compJoint;
	int i, j, k, l, m, count, numObservedBody=0, bodyCount[BODY_COUNT];
	int matchedInfo[KINECT_COUNT][BODY_COUNT];     /* #-1 no data, #1 matched, #0 unmatched*/
	double error, error2, minError;
	double errorThreshold = 0.35;
	bool bFindRefBody;
	int refKinectIdx;
	int matchedKinectIdx, matchedBodyIdx;
	int kIdx;
	bool bFind = false;

	syncSkelAll = new synconizedSkeletonsAll;
	memset(syncSkelAll, 0x00, sizeof(synconizedSkeletonsAll));

	pSkeletonInfo = &pDlg->m_SkeletonsInfo[0];
	memcpy(&syncSkelAll->st, &pSkeletonInfo->st, sizeof(SYSTEMTIME));
	memset(matchedInfo, 0x00, sizeof(int)*KINECT_COUNT*BODY_COUNT);
	//	memset(bodyCount, 0x00, sizeof(int)*BODY_COUNT);

	for (i = 0; i < KINECT_COUNT; i++) {
		count  = pDlg->m_SkeletonsInfo[i].Count;

		for (j = 0 ; j < BODY_COUNT; j ++) {
			if (j >= count) matchedInfo[i][j] = -1; 
		}
	}

	for (i = 0 ; i < BODY_COUNT; i ++) {
		syncSkelAll->bObsevedBody[i] = false;
		syncSkel = new synconizedSkeletons;
		memset(syncSkel, 0x00, sizeof(synconizedSkeletons));
		bodyCount[i] = 0; 
		bFindRefBody = false;
		refKinectIdx = -1;
		syncSkel->Hid = i+1;

		/* find reference body*/
		for (j = 0; j < KINECT_COUNT; j++) {
			for (k = 0; k < BODY_COUNT; k++) {			
				if (matchedInfo[j][k] == 0 && bFindRefBody == false) {
					refBody = &(pDlg->m_SkeletonsInfo[j].InfoBody[k]);
					kIdx = pDlg->m_SkeletonsInfo[j].Kinect_ID-1;
					memcpy(&syncSkel->InfoBody[kIdx], refBody, sizeof(BodyInfo));
					matchedInfo[j][k] = 1;
					syncSkel->bObsevingKinect[kIdx] = true;
					bFindRefBody = true;
					refKinectIdx = j;
					bodyCount[i]++;
					bFind = true;
					break;
				}
			}
		}

		if(!bFindRefBody) {
			syncSkel->numObservingKinect = bodyCount[i];
			memcpy(&syncSkelAll->syncSkels[i], syncSkel, sizeof(synconizedSkeletons));
			delete syncSkel;
			syncSkel = NULL;
			continue;
		}
		/* find corresponding body*/		
		for (j = 0; j < KINECT_COUNT; j++) {
			minError = 1000000.0; matchedKinectIdx=-1; matchedBodyIdx=-1;

			for (k = 0; k < BODY_COUNT; k++) {
				if (refKinectIdx == j) continue;
				if (matchedInfo[j][k] == -1) continue;

				tCompBody = &(pDlg->m_SkeletonsInfo[j].InfoBody[k]);

				error = CalcDistanceTwoBodies(refBody, tCompBody);
				/*
				if (pDlg->m_SkeletonsInfo[j].Kinect_ID != 3) 
				error2 = ConvertLeftRight(refBody, tCompBody, error);

				if (error > error2) error = error2;
				*/
				if (error < errorThreshold) {
					if (minError > error) {
						minError = error;
						matchedKinectIdx = j; 
						kIdx = pDlg->m_SkeletonsInfo[j].Kinect_ID-1;
						matchedBodyIdx = k;
						compBody = tCompBody; 
					}
				}

			}
			if (matchedKinectIdx != -1 && matchedBodyIdx != -1) {
				matchedInfo[matchedKinectIdx][matchedBodyIdx] = 1;
				memcpy(&syncSkel->InfoBody[kIdx], compBody, sizeof(BodyInfo));
				syncSkel->bObsevingKinect[kIdx] = true;
				bodyCount[i]++;
			}
		}
		syncSkel->numObservingKinect = bodyCount[i];
		memcpy(&syncSkelAll->syncSkels[i], syncSkel, sizeof(synconizedSkeletons));
		delete syncSkel;
		syncSkel = NULL;

		if (bodyCount[i] > 0) {
			numObservedBody++;
			syncSkelAll->bObsevedBody[i] = true;
		}
	}

	syncSkelAll->numObservedBody = numObservedBody;

	for (i = 0; i < KINECT_COUNT; i++) {
		if (pDlg->m_SkeletonsInfo[i].Kinect_ID == 3) {
			memcpy(&syncSkelAll->st, &pDlg->m_SkeletonsInfo[i].st,  sizeof(SYSTEMTIME));

		}
	}

	pDlg->m_syncSkelList.AddTail(syncSkelAll);

	if (pDlg->m_syncSkelList.GetCount() > SYNC_SKELS_BUF_SIZE)
		pDlg->m_syncSkelList.RemoveHead();

	if(!bFind) return false;

	return true;
}


void CNMsgrSvrDlg::AppendText(CEdit* pEdit, CString addStr)
{
	CString str, prevStr;
	pEdit->GetWindowTextA(prevStr);
	str = prevStr  + addStr;
	pEdit->SetWindowTextA(str);
}

void CNMsgrSvrDlg::OnAccept(int id)
{
	CString str;

	str.Format("A new Kinect [id:%d] is accepted",id);
	PrintStatus(str);

	//	char currentTime[84] = "";
	//	sprintf(currentTime,"%d/%d/%d  %d:%d:%d %d",st.wDay,st.wMonth,st.wYear, st.wHour, st.wMinute, st.wSecond , st.wMilliseconds);

	//	CString strTime(currentTime);
	//AppendText(&m_Edit_Status, str);
}


void CNMsgrSvrDlg::OnDisconnect(int id)
{
	CString str;

	str.Format("A Kinect [id:%d] is disconnected",id);
	PrintStatus(str);
	//	AppendText(&m_Edit_Status, str);
}

void CNMsgrSvrDlg::PrintStatus(CString str)
{
	CString prevStr, curStr;

	if (str == "") return;

	if(m_Edit_Status.GetLineCount() >= 100)
	{
		int nLength = 0;

		for(int i = 0; i < 100; i++)
			nLength += m_Edit_Status.LineLength(i);

		m_Edit_Status.SetSel( 0, nLength, FALSE);
		m_Edit_Status.ReplaceSel("", FALSE); 
	}

	//	UpdateData(TRUE);
	m_Edit_Status.GetWindowTextA(prevStr);

	str += "\r\n";
	curStr = prevStr + str;

	//	UpdateData(FALSE);
	m_Edit_Status.SetWindowTextA(curStr);
	m_Edit_Status.LineScroll(m_Edit_Status.GetLineCount());
}

afx_msg LRESULT CNMsgrSvrDlg::OnGetKinectData(WPARAM wParam, LPARAM lParam)
{
	return 1; 	
}

void CNMsgrSvrDlg::OnBnClickedOk()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	m_ReadBody.CloseFile();
	//	m_RecordBody.CloseFile();
	CloseHandle(m_hThread);
	DeleteCriticalSection(&m_cs);
#ifdef KINECTALL_MODE
	m_BodyRenderer.DeInitializeRenderer();
	m_BodyRenderer.WaitUntilThreadDead();
#endif
	m_RefineRenderer.DeInitializeRenderer();
	m_RefineRenderer.WaitUntilThreadDead();
#ifdef AVERAGE_MODE
	m_AverRenderer.DeInitializeRenderer();
	m_AverRenderer.WaitUntilThreadDead();
#endif

#ifdef MAINCAM_MODE
	m_CenterRenderer.DeInitializeRenderer();
	m_CenterRenderer.WaitUntilThreadDead();
#endif
	//	CDialogEx::OnOK();
}

_int64 CNMsgrSvrDlg::time_differnece(const SYSTEMTIME st1, const SYSTEMTIME st2)
{
	union timeunion {
		FILETIME fileTime;
		ULARGE_INTEGER ul;
	} ;

	timeunion ft1;
	timeunion ft2;

	SystemTimeToFileTime(&st1, &ft1.fileTime);
	SystemTimeToFileTime(&st2, &ft2.fileTime);

	return ft2.ul.QuadPart - ft1.ul.QuadPart;
}

void CNMsgrSvrDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO: Add your message handler code here
}

float CNMsgrSvrDlg::CalcDistanceTwoBodies(BodyInfo *refBody, BodyInfo *compBody)
{
	float dist, distsum;
	int count, i;
	Joint *refJoint, *compJoint;
	float MAX_ERROR = 100.0;

	count = 0; distsum = 0.0;
	for (i = 0; i < 4; i++) {
		dist = 0.0;
		refJoint = &refBody->JointPos[i];
		compJoint = &compBody->JointPos[i];
		//		if (refJoint->TrackingState == TrackingState_Tracked && compJoint->TrackingState == TrackingState_Tracked) {
		dist = pow( (refJoint->Position.X - compJoint->Position.X), 2) ;
		dist = dist + pow( (refJoint->Position.Y - compJoint->Position.Y), 2);
		dist = dist + pow( (refJoint->Position.Z - compJoint->Position.Z), 2);
		dist = sqrt(dist);
		distsum = distsum + dist;
		count++;
		//		}
	}
	dist = distsum / double(count);

	if (count == 0)
		dist = MAX_ERROR;

	return dist;
}

float CNMsgrSvrDlg::CalcDistanceTwoBodies2(BodyInfo *refBody, BodyInfo *compBody)
{
	float dist;
	int count, i;
	Joint *refJoint, *compJoint;
	float MAX_ERROR = 100.0;

	int index[19];
	m_KF[0].GetActualJointIdx(index);

	dist = 0.0; count = 0;
	for (i = 0; i < ACTUAL_JOINT_COUNT; i++) {
		refJoint = &refBody->JointPos[index[i]];
		compJoint = &compBody->JointPos[index[i]];
		//		if (refJoint->TrackingState == TrackingState_Tracked && compJoint->TrackingState == TrackingState_Tracked) {
		dist = dist + pow( (refJoint->Position.X - compJoint->Position.X), 2);
		dist = dist + pow( (refJoint->Position.Y - compJoint->Position.Y), 2);
		dist = dist + pow( (refJoint->Position.Z - compJoint->Position.Z), 2);
		count++;
	}
	dist = dist / double(count);

	if (count == 0)
		dist = MAX_ERROR;

	return dist;
}

int CNMsgrSvrDlg::CalcRefID(SkeletonInfo *Input){
	float LengthRatio = 9999.0f;
	int tid = 0;

	float spineDepth;
	float shoulderLength;

	for(int i = 0; i < NUM_KINECTS; i++){
		spineDepth = Input[i].InfoBody[0].spinedepth;
		shoulderLength = Input[i].InfoBody[0].upperbodylen;

		float tRatio = shoulderLength / spineDepth;

		if(LengthRatio > tRatio){
			LengthRatio = tRatio;
			tid = i;
		}
	}

	return tid;
}

void CNMsgrSvrDlg::RefineBodyDirection(synconizedSkeletons *src, int upperRefID, int lowerRefID){
	//float ELL, ELR, ERL, ERR;
	float EReverse, ENormal;				//EReverse : 역방향 에러, ENormal : 정방향 에러
	CameraSpacePoint BasisTran;
	CameraSpacePoint temp;

	if(upperRefID == -1 || lowerRefID == -1)	return;

	for(int i = 0; i < NUM_KINECTS; i++){
		//ELL = ELR = ERR = ERL = 0.0f;
		EReverse = ENormal = 0.0f;

		if(src->bObsevingKinect[i] == false)	continue;
		// 상체 스왑
		if( i != upperRefID){
			BasisTran.X = src->InfoBody[i].JointPos[JointType_SpineShoulder].Position.X - src->InfoBody[upperRefID].JointPos[JointType_SpineShoulder].Position.X;
			BasisTran.Y = src->InfoBody[i].JointPos[JointType_SpineShoulder].Position.Y - src->InfoBody[upperRefID].JointPos[JointType_SpineShoulder].Position.Y;
			BasisTran.Z = src->InfoBody[i].JointPos[JointType_SpineShoulder].Position.Z - src->InfoBody[upperRefID].JointPos[JointType_SpineShoulder].Position.Z;

			/*for(int j = 0; j < JointType_HipLeft; j++){
			//왼쪽에서 시작비교
			if(JointType_ShoulderLeft == j || JointType_ElbowLeft == j || JointType_WristLeft == j || JointType_HandLeft == j ){
			//좌우 거리 비교
			CameraSpacePoint OriginPos = src->InfoBody[upperRefID].JointPos[j].Position;
			CameraSpacePoint ReversePos = src->InfoBody[upperRefID].JointPos[j+4].Position;

			//Error Left to Left
			ELL = sqrt(pow(OriginPos.X - (src->InfoBody[i].JointPos[j].Position.X+BasisTran.X),2)
			+ pow(OriginPos.Y - (src->InfoBody[i].JointPos[j].Position.Y+BasisTran.Y),2)
			+ pow(OriginPos.Z - (src->InfoBody[i].JointPos[j].Position.Z+BasisTran.Z),2));

			//Error Left to Right
			ELR = sqrt(pow(ReversePos.X - (src->InfoBody[i].JointPos[j].Position.X+BasisTran.X),2)
			+ pow(ReversePos.Y - (src->InfoBody[i].JointPos[j].Position.Y+BasisTran.Y),2)
			+ pow(ReversePos.Z - (src->InfoBody[i].JointPos[j].Position.Z+BasisTran.Z),2));
			}
			//오른쪽에서 시작비교
			if(JointType_ShoulderRight == j || JointType_ElbowRight == j || JointType_WristRight == j || JointType_HandRight == j ){
			CameraSpacePoint OriginPos = src->InfoBody[upperRefID].JointPos[j].Position;
			CameraSpacePoint ReversePos = src->InfoBody[upperRefID].JointPos[j-4].Position;

			//Error Right to right
			ERR = sqrt(pow(OriginPos.X - src->InfoBody[i].JointPos[j].Position.X+BasisTran.X,2)
			+ pow(OriginPos.Y - src->InfoBody[i].JointPos[j].Position.Y+BasisTran.Y,2)
			+ pow(OriginPos.Z - (src->InfoBody[i].JointPos[j].Position.Z+BasisTran.Z),2));

			//Error Right to left
			ERL = sqrt(pow(ReversePos.X - (src->InfoBody[i].JointPos[j].Position.X+BasisTran.X),2)
			+ pow(ReversePos.Y - (src->InfoBody[i].JointPos[j].Position.Y+BasisTran.Y),2)
			+ pow(ReversePos.Z - (src->InfoBody[i].JointPos[j].Position.Z+BasisTran.Z),2));
			}
			}*/

			//왼쪽에서
			for(int j = JointType_ShoulderLeft; j < JointType_ShoulderRight; j++){
				CameraSpacePoint refPoint = src->InfoBody[upperRefID].JointPos[j].Position;

				float tNor = sqrt(pow(refPoint.X - (src->InfoBody[i].JointPos[j].Position.X-BasisTran.X),2)
					+ pow(refPoint.Y - (src->InfoBody[i].JointPos[j].Position.Y-BasisTran.Y),2)
					+ pow(refPoint.Z - (src->InfoBody[i].JointPos[j].Position.Z-BasisTran.Z),2));

				float tRev = sqrt(pow(refPoint.X - (src->InfoBody[i].JointPos[j+4].Position.X-BasisTran.X),2)
					+ pow(refPoint.Y - (src->InfoBody[i].JointPos[j+4].Position.Y-BasisTran.Y),2)
					+ pow(refPoint.Z - (src->InfoBody[i].JointPos[j+4].Position.Z-BasisTran.Z),2));

				EReverse += tRev;
				ENormal += tNor;
			}

			//오른쪽에서
			for(int j = JointType_ShoulderRight; j < JointType_HipLeft; j++){
				CameraSpacePoint refPoint = src->InfoBody[upperRefID].JointPos[j].Position;

				float tNor = sqrt(pow(refPoint.X - (src->InfoBody[i].JointPos[j].Position.X-BasisTran.X),2)
					+ pow(refPoint.Y - (src->InfoBody[i].JointPos[j].Position.Y-BasisTran.Y),2)
					+ pow(refPoint.Z - (src->InfoBody[i].JointPos[j].Position.Z-BasisTran.Z),2));

				float tRev = sqrt(pow(refPoint.X - (src->InfoBody[i].JointPos[j-4].Position.X-BasisTran.X),2)
					+ pow(refPoint.Y - (src->InfoBody[i].JointPos[j-4].Position.Y-BasisTran.Y),2)
					+ pow(refPoint.Z - (src->InfoBody[i].JointPos[j-4].Position.Z-BasisTran.Z),2));

				EReverse += tRev;
				ENormal += tNor; 
			}

			//에러보고 스왑 - 좌우 통째로 돌림
			if(/*(ERL < ERR && ERL < ELL && ERL < ELL) || (ELR < ERR && ELR < ELL && ELR < ERL)*/EReverse < ENormal){
				//상체 스왑
				SWAP(src->InfoBody[i].JointPos[JointType_ShoulderLeft].Position, src->InfoBody[i].JointPos[JointType_ShoulderRight].Position, temp);
				SWAP(src->InfoBody[i].JointPos[JointType_ElbowLeft].Position, src->InfoBody[i].JointPos[JointType_ElbowRight].Position, temp);
				SWAP(src->InfoBody[i].JointPos[JointType_WristLeft].Position, src->InfoBody[i].JointPos[JointType_WristRight].Position, temp);
				SWAP(src->InfoBody[i].JointPos[JointType_HandLeft].Position, src->InfoBody[i].JointPos[JointType_HandRight].Position, temp);

				//하체도 그냥 상체 조건에 부합하면 스왑
				SWAP(src->InfoBody[i].JointPos[JointType_HipLeft].Position, src->InfoBody[i].JointPos[JointType_HipRight].Position, temp);
				SWAP(src->InfoBody[i].JointPos[JointType_KneeLeft].Position, src->InfoBody[i].JointPos[JointType_KneeRight].Position, temp);
				SWAP(src->InfoBody[i].JointPos[JointType_AnkleLeft].Position, src->InfoBody[i].JointPos[JointType_AnkleRight].Position, temp);
			}
		}

		/*//EReverse = ENormal = 0.0f;
		//// 하체 스왑
		//if (i != lowerRefID){
		//	BasisTran.X = src->InfoBody[i].JointPos[JointType_SpineBase].Position.X - src->InfoBody[lowerRefID].JointPos[JointType_SpineBase].Position.X;
		//	BasisTran.Y = src->InfoBody[i].JointPos[JointType_SpineBase].Position.Y - src->InfoBody[lowerRefID].JointPos[JointType_SpineBase].Position.Y;
		//	BasisTran.Z = src->InfoBody[i].JointPos[JointType_SpineBase].Position.Z - src->InfoBody[lowerRefID].JointPos[JointType_SpineBase].Position.Z;

		//	/*for(int j = JointType_HipLeft; j < JointType_SpineShoulder; j++){
		//	//왼쪽에서 시작비교
		//	if(JointType_HipLeft == j || JointType_KneeLeft == j || JointType_AnkleLeft == j ){
		//	//좌우 거리 비교
		//	CameraSpacePoint OriginPos = src->InfoBody[lowerRefID].JointPos[j].Position;
		//	CameraSpacePoint ReversePos = src->InfoBody[lowerRefID].JointPos[j+4].Position;

		//	//Error Left to Left
		//	ELL = sqrt(pow(OriginPos.X - (src->InfoBody[i].JointPos[j].Position.X+BasisTran.X),2)
		//	+ pow(OriginPos.Y - (src->InfoBody[i].JointPos[j].Position.Y+BasisTran.Y),2)
		//	+ pow(OriginPos.Z - (src->InfoBody[i].JointPos[j].Position.Z+BasisTran.Z),2));

		//	//Error Left to Right
		//	ELR = sqrt(pow(ReversePos.X - (src->InfoBody[i].JointPos[j].Position.X+BasisTran.X),2)
		//	+ pow(ReversePos.Y - (src->InfoBody[i].JointPos[j].Position.Y+BasisTran.Y),2)
		//	+ pow(ReversePos.Z - (src->InfoBody[i].JointPos[j].Position.Z+BasisTran.Z),2));
		//	}
		//	//오른쪽에서 시작비교
		//	if(JointType_HipRight == j || JointType_KneeRight == j || JointType_AnkleRight == j ){
		//	CameraSpacePoint OriginPos = src->InfoBody[lowerRefID].JointPos[j].Position;
		//	CameraSpacePoint ReversePos = src->InfoBody[lowerRefID].JointPos[j-4].Position;

		//	//Error Right to right
		//	ERR = sqrt(pow(OriginPos.X - src->InfoBody[i].JointPos[j].Position.X,2)
		//	+ pow(OriginPos.Y - src->InfoBody[i].JointPos[j].Position.Y,2)
		//	+ pow(OriginPos.Z - (src->InfoBody[i].JointPos[j].Position.Z+BasisTran.Z),2));

		//	//Error Right to left
		//	ERL = sqrt(pow(ReversePos.X - (src->InfoBody[i].JointPos[j].Position.X+BasisTran.X),2)
		//	+ pow(ReversePos.Y - (src->InfoBody[i].JointPos[j].Position.Y+BasisTran.Y),2)
		//	+ pow(ReversePos.Z - (src->InfoBody[i].JointPos[j].Position.Z+BasisTran.Z),2));
		//	}
		//	}*/

		//	//왼쪽에서
		//	for(int j = JointType_HipLeft; j < JointType_FootLeft; j++){
		//		CameraSpacePoint refPoint = src->InfoBody[lowerRefID].JointPos[j].Position;

		//		float tNor = sqrt(pow(refPoint.X - (src->InfoBody[i].JointPos[j].Position.X-BasisTran.X),2)
		//			+ pow(refPoint.Y - (src->InfoBody[i].JointPos[j].Position.Y-BasisTran.Y),2)
		//			+ pow(refPoint.Z - (src->InfoBody[i].JointPos[j].Position.Z-BasisTran.Z),2));

		//		float tRev = sqrt(pow(refPoint.X - (src->InfoBody[i].JointPos[j+4].Position.X-BasisTran.X),2)
		//			+ pow(refPoint.Y - (src->InfoBody[i].JointPos[j+4].Position.Y-BasisTran.Y),2)
		//			+ pow(refPoint.Z - (src->InfoBody[i].JointPos[j+4].Position.Z-BasisTran.Z),2));

		//		EReverse += tRev;
		//		ENormal += tNor;
		//	}

		//	//오른쪽에서
		//	for(int j = JointType_HipRight; j < JointType_FootRight; j++){
		//		CameraSpacePoint refPoint = src->InfoBody[upperRefID].JointPos[j].Position;

		//		float tNor = sqrt(pow(refPoint.X - (src->InfoBody[i].JointPos[j].Position.X-BasisTran.X),2)
		//			+ pow(refPoint.Y - (src->InfoBody[i].JointPos[j].Position.Y-BasisTran.Y),2)
		//			+ pow(refPoint.Z - (src->InfoBody[i].JointPos[j].Position.Z-BasisTran.Z),2));

		//		float tRev = sqrt(pow(refPoint.X - (src->InfoBody[i].JointPos[j-4].Position.X-BasisTran.X),2)
		//			+ pow(refPoint.Y - (src->InfoBody[i].JointPos[j-4].Position.Y-BasisTran.Y),2)
		//			+ pow(refPoint.Z - (src->InfoBody[i].JointPos[j-4].Position.Z-BasisTran.Z),2));

		//		EReverse += tRev;
		//		ENormal += tNor; 
		//	}

		//	//에러보고 스왑
		//	if(/*(ERL < ERR && ERL < ELL && ERL < ELL) || (ELR < ERR && ELR < ELL && ELR < ERL)*/EReverse < ENormal){
		//		//하체 스왑
		//		SWAP(src->InfoBody[i].JointPos[JointType_HipLeft].Position, src->InfoBody[i].JointPos[JointType_HipRight].Position, temp);
		//		SWAP(src->InfoBody[i].JointPos[JointType_KneeLeft].Position, src->InfoBody[i].JointPos[JointType_KneeRight].Position, temp);
		//		SWAP(src->InfoBody[i].JointPos[JointType_AnkleLeft].Position, src->InfoBody[i].JointPos[JointType_AnkleRight].Position, temp);
		//	}
		//}*/
	}
}

bool comp(std::pair<float,int> a, std::pair<float,int> b) {
	return (a.first > b.first);
}

//Ref upper body or lower body decision
int CNMsgrSvrDlg::CalcUpperRefID(synconizedSkeletons input, synconizedSkeletons prev, int t){
	//얼굴 출현 여부 찾기
	bool bFaceDetect = false;
	int errorIdx = -1;

#ifdef FACEDEBUG_MODE
	IplImage *tROI;
	tROI = cvCreateImage(cvSize(64,64), IPL_DEPTH_8U, 1);

#endif

	float MeanRight = 0.0f, MeanLeft = 0.0f;
	int BodyCount = 0;
	CameraSpacePoint center, rightShoulder, leftShoulder ;
	for(int i = 0; i < KINECT_COUNT; i++){

		//오른쪽 왼쪽 어깨 유클리디안 디스턴스 계산
		if(input.bObsevingKinect[i]){
			center = input.InfoBody[i].JointPos[JointType_SpineShoulder].Position;
			rightShoulder = input.InfoBody[i].JointPos[JointType_ShoulderRight].Position;
			leftShoulder = input.InfoBody[i].JointPos[JointType_ShoulderLeft].Position;
			BodyCount++;
			MeanRight += sqrt(pow(center.X - rightShoulder.X, 2) + pow(center.Y - rightShoulder.Y, 2) + pow(center.Z - rightShoulder.Z, 2));
			MeanLeft += sqrt(pow(center.X - leftShoulder.X, 2) + pow(center.Y - leftShoulder.Y, 2) + pow(center.Z - leftShoulder.Z, 2));
		}
		if(input.InfoBody[i].Face.bDetect){

#ifdef FACEDEBUG_MODE
			cvSet(tROI, cvScalarAll(255));
#endif
			if(input.InfoBody[i].Face.yaw < 40 && input.InfoBody[i].Face.yaw > -40){
				bFaceDetect = true;
			}
			else{
				input.InfoBody[i].Face.bDetect = false;
			}
		}
#ifdef FACEDEBUG_MODE
		else
			cvZero(tROI);

		cvSetImageROI(FaceDetected, cvRect(64*i, 64*t, 64, 64));
		cvCopy(tROI, FaceDetected);
		cvResetImageROI(FaceDetected);
#endif
	}

	MeanRight /= BodyCount;
	MeanLeft /= BodyCount;

#ifdef FACEDEBUG_MODE
	cvReleaseImage(&tROI);
#endif

	//얼굴이 없으면 prev 얼굴 정보 사용
	if(!bFaceDetect){
		//이전 얼굴 정보 검색
		for(int i = 0; i < KINECT_COUNT; i++){
			if(input.bObsevingKinect[i] && prev.bObsevingKinect[i] && prev.InfoBody[i].Face.bDetect){
				if(prev.InfoBody[i].Face.yaw < 40 && prev.InfoBody[i].Face.yaw > -40)
					bFaceDetect = true;
				else
					prev.InfoBody[i].Face.bDetect = false;
			}
		}

		//prev에서 얼굴 출현시 현재 정보에 overwrite
		if(bFaceDetect){
			for(int i = 0; i < KINECT_COUNT; i++){
				if(input.bObsevingKinect[i])
					input.InfoBody[i].Face.bDetect = prev.InfoBody[i].Face.bDetect;
			}
		}
	}

	//비율 순으로 정렬
	float MaxDepth = -1.0f;
	std::vector<std::pair<float,int>> tempvec;			//<ratio, idx>
	std::pair<float, int> temppair;
	float rightLen, leftLen;
	for(int i = 0; i < KINECT_COUNT; i++){
		if(input.bObsevingKinect[i]){
			errorIdx = i;

			center = input.InfoBody[i].JointPos[JointType_SpineShoulder].Position;
			rightShoulder = input.InfoBody[i].JointPos[JointType_ShoulderRight].Position;
			leftShoulder = input.InfoBody[i].JointPos[JointType_ShoulderLeft].Position;
			rightLen = sqrt(pow(center.X - rightShoulder.X, 2) + pow(center.Y - rightShoulder.Y, 2) + pow(center.Z - rightShoulder.Z, 2));
			leftLen = sqrt(pow(center.X - leftShoulder.X, 2) + pow(center.Y - leftShoulder.Y, 2) + pow(center.Z - leftShoulder.Z, 2));

			if(abs(MeanRight - rightLen) > OUTLIER_THRESHOLD || abs(MeanLeft - leftLen) > OUTLIER_THRESHOLD)
				continue;

			temppair.first = input.InfoBody[i].upperbodylen * input.InfoBody[i].spinedepth;
			temppair.second = i;
			tempvec.push_back(temppair);

			if(MaxDepth < input.InfoBody[i].spinedepth)							//Find Max Depth
				MaxDepth = input.InfoBody[i].spinedepth;
		}
	}

	for(int i = 0; i < tempvec.size(); i++){
		tempvec.at(i).first /= MaxDepth;
	}
	std::sort(tempvec.begin(), tempvec.end(), comp);

	//얼굴보고 판정
	if(bFaceDetect){
		for(int i = 0; i < tempvec.size(); i++)
			if(sidefacecheck(input, tempvec.at(i).second))	return tempvec.at(i).second;				//비율이 가장 긴 바디에 얼굴이 디텍션 됬을 경우
	}

	if(tempvec.size() != 0)
		return tempvec.at(0).second;

	return errorIdx;
}

int CNMsgrSvrDlg::CalcLowerRefID(synconizedSkeletons input, synconizedSkeletons prev){

	//얼굴 출현 여부 찾기
	bool bFaceDetect = false;

#ifdef FACEDEBUG_MODE
	IplImage *tROI;
	tROI = cvCreateImage(cvSize(64,64), IPL_DEPTH_8U, 1);

#endif

	for(int i = 0; i < KINECT_COUNT; i++){
		if(input.InfoBody[i].Face.bDetect){
			if(input.InfoBody[i].Face.yaw < 40 && input.InfoBody[i].Face.yaw > -40){
				bFaceDetect = true;
			}
			else{
				input.InfoBody[i].Face.bDetect = false;
			}
		}
	}

	//얼굴이 없으면 prev 얼굴 정보 사용
	if(!bFaceDetect){
		//이전 얼굴 정보 검색
		for(int i = 0; i < KINECT_COUNT; i++){
			if(input.bObsevingKinect[i] && prev.bObsevingKinect[i] && prev.InfoBody[i].Face.bDetect){
				if(prev.InfoBody[i].Face.yaw < 40 && prev.InfoBody[i].Face.yaw > -40)
					bFaceDetect = true;
				else
					prev.InfoBody[i].Face.bDetect = false;
			}
		}

		//prev에서 얼굴 출현시 현재 정보에 overwrite
		if(bFaceDetect){
			for(int i = 0; i < KINECT_COUNT; i++){
				if(input.bObsevingKinect[i])
					input.InfoBody[i].Face.bDetect = prev.InfoBody[i].Face.bDetect;
			}
		}
	}

	//비율 순으로 정렬
	float MaxDepth = -1.0f;
	std::vector<std::pair<float,int>> tempvec;			//<ratio, idx>
	std::pair<float, int> temppair;
	for(int i = 0; i < KINECT_COUNT; i++){
		if(input.bObsevingKinect[i]){
			temppair.first = input.InfoBody[i].lowerbodylen * input.InfoBody[i].spinedepth;
			temppair.second = i;
			tempvec.push_back(temppair);

			if(MaxDepth < input.InfoBody[i].spinedepth)							//Find Max Depth
				MaxDepth = input.InfoBody[i].spinedepth;
		}
	}

	for(int i = 0; i < tempvec.size(); i++){
		tempvec.at(i).first /= MaxDepth;
	}
	std::sort(tempvec.begin(), tempvec.end(), comp);

	//얼굴보고 판정
	if(bFaceDetect){
		for(int i = 0; i < tempvec.size(); i++)
			if(sidefacecheck(input, tempvec.at(i).second))	return tempvec.at(i).second;				//비율이 가장 긴 바디에 얼굴이 디텍션 됬을 경우
	}else
		return tempvec.at(0).second;
}

//이웃 디바이스에서 얼굴이 디텍션됬는지를 확인
bool CNMsgrSvrDlg::sidefacecheck(synconizedSkeletons input, int idx){
	if(input.InfoBody[idx].Face.bDetect)		return true;
	else{
		int leftkinect = idx - 1;
		int rightkinect = idx + 1;

		//int bodyidx;
		if(leftkinect >= 0)
			if(input.InfoBody[leftkinect].Face.bDetect)	return true;

		if(rightkinect <= 4)
			if(input.InfoBody[rightkinect].Face.bDetect)	return true;
	}

	return false;
}

//change body dircetion
void CNMsgrSvrDlg::refineBodyDir(synconizedSkeletonsAll *syncBody){
	CString str;

#ifdef REFERENCEDEBUG
	IplImage *temp;

	temp = cvCreateImage(cvSize(64, 64), IPL_DEPTH_8U, 1);
	cvZero(FaceDetected);
	cvZero(t_Ref);
#endif

	for(int i = 0; i < BODY_COUNT; i++){
		m_upperIdxarr[i] = -1;
		if(syncBody->bObsevedBody[i] == false)
			continue;

		int upperIdx = CalcUpperRefID(syncBody->syncSkels[i], prevsync.syncSkels[i], i);
		//int lowerIdx = CalcLowerRefID(syncBody->syncSkels[i], prevsync.syncSkels[i]);

#ifdef REFERENCEDEBUG
		for(int j = 0; j < KINECT_COUNT; j++){
			if(j == upperIdx)
				cvSet(temp, cvScalarAll(255));
			else
				cvZero(temp);

			cvSetImageROI(t_Ref, cvRect(64*j, 64*i, 64, 64));
			cvCopy(temp, t_Ref);
			cvResetImageROI(t_Ref);
		}
#endif

		m_upperIdxarr[i] = upperIdx;

		RefineBodyDirection(&syncBody->syncSkels[i], upperIdx,/* lowerIdx*/upperIdx);
	}

#ifdef FACEDEBUG_MODE
	cvShowImage("KinectFace", FaceDetected);
	cvWaitKey(1);
#endif

#ifdef REFERENCEDEBUG
	cvShowImage("REFERENCE KINECT", t_Ref);
	cvWaitKey(1);
	cvReleaseImage(&temp);
#endif

	//histroy 기록
	memcpy(&prevsync, syncBody, sizeof(synconizedSkeletonsAll));
}

void CNMsgrSvrDlg::AverBody(synconizedSkeletonsAll syncBody, SkeletonInfo *dst){
	Joint tBody[JointType_Count];
	int skelcount = 0;

	for(int i = 0; i < BODY_COUNT; i++){
		if(!syncBody.bObsevedBody[i])	continue;

		//var Initialize
		for(int j = 0; j < JointType_Count; j++)	tBody[j].Position.X = tBody[j].Position.Y = tBody[j].Position.Z = 0.f;

		//Average
		for(int j = 0; j < KINECT_COUNT; j++){
			if(!syncBody.syncSkels[i].bObsevingKinect[j])	continue;

			for(int k = 0; k < JointType_Count; k++){
				tBody[k].Position.X += (syncBody.syncSkels[i].InfoBody[j].JointPos[k].Position.X / (float)syncBody.syncSkels[i].numObservingKinect);
				tBody[k].Position.Y += (syncBody.syncSkels[i].InfoBody[j].JointPos[k].Position.Y / (float)syncBody.syncSkels[i].numObservingKinect);
				tBody[k].Position.Z += (syncBody.syncSkels[i].InfoBody[j].JointPos[k].Position.Z / (float)syncBody.syncSkels[i].numObservingKinect);
			}
		}

		//write to skelinfo structure
		dst->InfoBody[skelcount].BodyID = syncBody.syncSkels[i].Hid;
		memcpy(dst->InfoBody[skelcount].JointPos, tBody, sizeof(Joint)*JointType_Count);
		skelcount++;
	}

	//
	dst->Count = syncBody.numObservedBody;
	dst->Kinect_ID = 3;
}

void CNMsgrSvrDlg::FindsyncSkel(synconizedSkeletonsAll syncBody, SkeletonInfo *dst, int KinectID){
	int tskelCount = 0;

	dst->Kinect_ID = KinectID;

	for(int i = 0; i < BODY_COUNT; i++){
		if(syncBody.bObsevedBody[i]){
			if(syncBody.syncSkels[i].bObsevingKinect[KinectID-1]){
				memcpy(&dst->InfoBody[tskelCount], &syncBody.syncSkels[i].InfoBody[KinectID-1], sizeof(BodyInfo));
				tskelCount++;
			}
		}
	}

	dst->Count = tskelCount;
}

void CNMsgrSvrDlg::OnBnClickedBtnWriteStart()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	UpdateData(TRUE);
	if (m_strFileName=="") 
	{
		PrintStatus("FileName을 입력해주세요!");
		m_fileName.SetFocus();
	}
	else
		stateRecord.OpenFile(NULL, m_strFileName.GetBuffer(), 'w');
}


void CNMsgrSvrDlg::OnBnClickedBtnFileClose()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	UpdateData(TRUE);
	if (m_strFileName=="") 
	{
		PrintStatus("FileName을 입력해주세요!");
		m_fileName.SetFocus();
	}
	else
		stateRecord.CloseFile();
}
