// ChildSocket.cpp: implementation of the CChildSocket class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NMsgrSvr.h"
#include "ChildSocket.h"
#include "NMsgrSvrDlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CChildSocket::CChildSocket()
{
	//	m_pHwnd = hWnd;

	m_pThread = NULL;
	m_dwThID=0;
	m_dwServerID=0;
	m_strID = "";
	m_lstQueue.RemoveAll();
	m_strGlobal = "D:\\Global";
	m_strPrivate = "D:\\FTPUser";
	m_hFile = 0;
	m_nRest = 0;
	m_byRest = NULL;
	m_strName = "";
	m_Kinect_id = 0;
	m_skel_data = new BYTE[SKELETON_DATA_SIZE];
	m_skelDataCount = 0;
	m_bReceiveNewData = false;
	m_bMotiveSocket = false;

	Begin_Thread();
}

CChildSocket::~CChildSocket()
{
	if(m_hSocket != INVALID_SOCKET)
		CloseSocket();
}

/*-------------------------------------------------------------/
소켓 닫고 처리 작업 수행  
/-------------------------------------------------------------*/
void CChildSocket::CloseSocket()
{
	m_nRest = 0;

	m_lstQueue.RemoveAll();

	if(m_dwThID != 0)
	{
		PostThreadMessage(m_dwThID, TH_END_THREAD, 0, (DWORD)this);
		::WaitForSingleObject(m_pThread->m_hThread, 1000);
	}

	if(m_pThread != NULL)
	{
		delete m_pThread;
		m_pThread=NULL;
	}

	//	Close();
	m_hSocket=INVALID_SOCKET;

	if(m_dwServerID != 0)
		PostThreadMessage(m_dwServerID, TH_CLEAR_PDATA, 0, (LPARAM)this);
}

void CChildSocket::Begin_Thread()
{
	m_pThread = ::AfxBeginThread(ReceiveThread, (LPVOID)this, THREAD_PRIORITY_NORMAL);
	m_dwThID = m_pThread->m_nThreadID;
}

void CChildSocket::OnReceive()
{
	//	CString str;
	//	str = "enter OnReceive";
	//	((CNMsgrSvrDlg*)m_pDlg)->PrintStatus(str);

	WORD wSize=0, wTotal=0;
	int i=0, nSum=0, nAdd=0, nReceived=0;
	ULONG nReceive=0;
	int nRec=0, nPrevRec=0, nRemain=0;
	char *byReceive=NULL, *byTmpReceive=NULL;
	bool bFirst = true;
	BYTE byTotal[PACKET_SIZE];
	memset(byTotal, 0x00, PACKET_SIZE);

	nRec = recv(m_hSocket, (char*)byTotal, PACKET_SIZE, 0);
	TRACE("header nRec=%d\n",nRec);
	memcpy(&wTotal, byTotal, PACKET_SIZE);
	byReceive = new char[wTotal];
	memset(byReceive, 0x00, wTotal);
	memcpy(byReceive, byTotal, PACKET_SIZE);
	nReceived = PACKET_SIZE;
	nRemain = wTotal-PACKET_SIZE;
	CString str;

	byTmpReceive = new char[10240];

	//	IOCtl(FIONREAD, &nReceive);
	while(nRemain > 0) {
		//		ioctlsocket(m_hSocket, FIONREAD, &nReceive);			// 소켓 버퍼에 들어온 데이터의 사이즈를 확인한다 
		//byTmpReceive = new char[nRemain];
		nRec = recv(m_hSocket, byTmpReceive, nRemain, 0);
		if (nRec < 1) {
			continue;
		}

		memcpy(byReceive+nReceived, byTmpReceive, nRec);
		nRemain -= nRec;
		nReceived += nRec;
		//		LeaveCriticalSection(&((CNMsgrSvrDlg*)m_pDlg)->m_cs);
	}

	TRACE("before m_lstQueue\n");

	BYTE *pData = new BYTE[wTotal];
	memcpy(pData, byReceive, wTotal);
	if (m_lstQueue.IsEmpty()) {
		m_lstQueue.AddHead(pData);

		if ( m_lstQueue.GetCount() > 1)
			m_lstQueue.RemoveTail();
	}else 
		m_lstQueue.AddTail(pData);

	//	str = "leave OnReceive";
	//	((CNMsgrSvrDlg*)m_pDlg)->PrintStatus(str);

	delete byTmpReceive;

	PostThreadMessage(m_dwThID, 0, 0, 0);
	delete byReceive;
}

/*-----------------------------------------------------------------------
| Thread Function
+------------------------------------------------------------------------
| ReceiveThread : 받은 Data의 Command 별 구분 
| argument      : 데이터 소켓의 포인터 
+----------------------------------------------------------------------*/
UINT CChildSocket::ReceiveThread(LPVOID pParam)
{
	MSG			msg;
	UINT		nRec=0;
	BYTE		bySave=0, byList=0, byOld=0, *nAddress=NULL;
	WORD		wDir=0, wName=0, wTotal=0;
	CString		str;
	int			nTotal=0;
	BYTE		byTotal[PACKET_SIZE];
	memset(byTotal, 0x00, PACKET_SIZE);

	CChildSocket *pData = (CChildSocket*)pParam;

	while(::GetMessage(&msg, NULL, 0, 0))
	{
		if(msg.message == TH_END_THREAD)
		{
			DWORD dwExitCode = 0;
			ExitThread(dwExitCode);
			break;
		}
		if(msg.hwnd == NULL)
		{
			while (pData->m_lstQueue.GetCount()!=0)
			{				
				BYTE *byStore=NULL;
				byStore = (BYTE*)pData->m_lstQueue.RemoveTail();
				memcpy(byTotal, byStore, PACKET_SIZE);		// size : 4
				memcpy(&wTotal, byTotal, PACKET_SIZE);
				BYTE* byReceive = new BYTE[wTotal];				// size : 34
				memcpy(byReceive, byStore, wTotal);				
				delete byStore;

				byList = byReceive[3];

				TRACE("before switch\n");

				switch(byReceive[2])
				{
				case CLIENT_INFO:
					{
						BYTE *id_data = new BYTE[4];
						nAddress = byReceive;
						nAddress+=HEAD_SIZE;
						memcpy(id_data, nAddress, 4);
						//str.Format("%d", (int)data);
						//int id = atoi( str.GetString());
						int id = atoi((char*)id_data);
						pData->m_Kinect_id = id;

						((CNMsgrSvrDlg*)pData->m_pDlg)->OnAccept(id);
						pData->SendSystemTime();
						//	((CRobotControllerSrvDlg*)pData->m_pDlg)->control_robot(byList);
						break;
					}

				case SKELETON_DATA:
					{
						nAddress = byReceive;
						nAddress+=HEAD_SIZE;
						memcpy(pData->m_skel_data, nAddress, SKELETON_DATA_SIZE);

						TRACE("before receive_skeleton\n");
						pData->receive_skeleton(pData->m_skel_data);
						TRACE("after receive_skeleton\n");

						if (((CNMsgrSvrDlg*)pData->m_pDlg)->m_bStartSync == false) {
							if(!pData->m_bMotiveSocket)
								((CNMsgrSvrDlg*)pData->m_pDlg)->m_bStartSync = true;
						}				
						break;
					}
				}
				delete byReceive;
			}
		}
	}


	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// send
///////////////////////////////////////////////////////////////////////////////
// DESCRIPTION:
//		Writes data to the Socket Communication
// PARAMETERS:
//		const LPBYTE lpBuffer: data to write
//		DWORD dwCount: maximum characters to write
//		DWORD dwTimeout: timeout to use in millisecond
///////////////////////////////////////////////////////////////////////////////
DWORD CChildSocket::Send(const LPBYTE lpBuffer, DWORD dwCount, DWORD dwTimeout)
{
	_ASSERTE( NULL != lpBuffer );

	fd_set	fdWrite  = { 0 };
	TIMEVAL	stTime;
	TIMEVAL	*pstTime = NULL;

	if ( INFINITE != dwTimeout ) {
		stTime.tv_sec = 0;
		stTime.tv_usec = dwTimeout*1000;
		pstTime = &stTime;
	}

	SOCKET s = (SOCKET) m_hSocket;
	// Set Descriptor
	if ( !FD_ISSET( s, &fdWrite ) )
		FD_SET( s, &fdWrite );

	// Select function set write timeout
	DWORD dwBytesWritten = 0L;
	int res = select( s+1, NULL, &fdWrite, NULL, pstTime );
	if ( res > 0)
	{
		res = ::send( s, (LPCSTR)lpBuffer, dwCount, 0);

		dwBytesWritten = (DWORD)((res >= 0)?(res) : (-1));
	}

	return dwBytesWritten;
}

void CChildSocket::OnConnect()
{
	//	SendMessage(m_pHwnd, UM_CONNECT, 0, 0);
}

void CChildSocket::SetMainDlg(CDialog* pDlg)
{
	m_pDlg = pDlg;
}

void CChildSocket::SendSystemTime()
{
	WORD wTotal, wData;
	wData = 16;

	BYTE byTime[16], *bySend=NULL, *nAddress=NULL;  //byPass[12],
	memset(byTime, 0x00, 16);

	wTotal = HEAD_SIZE + wData + EFCD_SIZE;

	bySend = new BYTE[wTotal];
	memset(bySend, 0x00, wTotal);

	SYSTEMTIME st;
	GetSystemTime(&st);

	memcpy(byTime, &st, sizeof(st));

	memcpy(bySend, &wTotal, sizeof(WORD));
	nAddress=bySend+sizeof(WORD);	*nAddress = TIME_INFO;
	nAddress++;						memset(nAddress, 0x00, sizeof(WORD));
	nAddress+=sizeof(WORD);			memcpy(nAddress, &wData, sizeof(WORD));
	nAddress+=sizeof(WORD);			memcpy(nAddress, byTime, 16);
	nAddress+=16;					*nAddress = 0xef;
	nAddress++;						*nAddress = 0xcd;

	Send(bySend, wTotal, 0);

}

void CChildSocket::receive_skeleton(BYTE* skel_data)
{
	int count, i, j;
	Point2d* D2Joints;
	Joint* D3Joints;
	static float tTick = -1.0f;
	CString str;
	SkeletonInfo* pSkeletonInfo;

	//memcpy에서 skel_data만 다루니까 여기서만 묶어줌(?)
	EnterCriticalSection(&m_cs);
	memcpy(&m_LastSkeletonInfo, skel_data, SKELETON_DATA_SIZE);
	pSkeletonInfo = new SkeletonInfo;
	memcpy(pSkeletonInfo, skel_data, SKELETON_DATA_SIZE);

	str.Format("sec=%d,milsec=%d", m_LastSkeletonInfo.st.wSecond, m_LastSkeletonInfo.st.wMilliseconds);
	((CNMsgrSvrDlg*)m_pDlg)->PrintStatus(str);

//	if (m_LastSkeletonInfo.Count > 0) {
		m_skeletonList.AddTail(pSkeletonInfo);
		count = m_skeletonList.GetCount();
		
		if (count > 10) {
			pSkeletonInfo = m_skeletonList.GetHead();
			delete pSkeletonInfo;
			m_skeletonList.RemoveHead();
		}
		m_bReceiveNewData = true;
		m_skelDataCount = m_skeletonList.GetCount();
//	}
	LeaveCriticalSection(&m_cs);

	/*	WORD wTotal, wData;
	wData = 0;

	BYTE *bySend=NULL, *nAddress=NULL;  //byPass[12],

	wTotal = HEAD_SIZE + wData + EFCD_SIZE;

	bySend = new BYTE[wTotal];

	memcpy(bySend, &wTotal, sizeof(WORD));
	nAddress=bySend+sizeof(WORD);	*nAddress = SKELETON_DATA_RECEIVED;
	nAddress++;						memset(nAddress, 0x00, sizeof(WORD));
	nAddress+=sizeof(WORD);			*nAddress = 0xef;
	nAddress++;						*nAddress = 0xcd;

	Send(bySend, wTotal, 0);*/

}