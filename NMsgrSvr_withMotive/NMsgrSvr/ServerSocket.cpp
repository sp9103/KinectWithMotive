// ServerSocket.cpp: implementation of the CServerSocket class.
//
//////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "NMsgrSvr.h"
#include "ServerSocket.h"
#include <process.h> 
#include "NMsgrSvrDlg.h"
#include "userevent.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CServerSocket::CServerSocket()
{
	m_Conn = 0;
	InitializeCriticalSection(&m_cs);
	child_socket_count = 0;
}

CServerSocket::~CServerSocket()
{
	DeleteCriticalSection(&m_cs);
}

//////////////////////////////////////////////////////////////////////////////////////////
//
//	InitListen
//
//	Desc : ���� ������ �ʱ�ȭ �Ѵ� ��Ʈ��ȣ�� PORT_NUM���� define ��  
//
bool CServerSocket::InitListen()
{
	WSADATA wd;

	struct addrinfo hints;
	struct addrinfo *result = NULL;

	int iResult = WSAStartup(MAKEWORD(2,2), &wd);
    if (iResult != 0) {
        AfxMessageBox("WSAStartup failed with error: %d\n", iResult);
        return false;
    }

	ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;    // TCP connection!!!
    hints.ai_flags = AI_PASSIVE;

	    // Resolve the server address and port
    iResult = getaddrinfo(NULL, PORT_NUM, &hints, &result);

    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        exit(1);
    }

	g_ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (g_ListenSocket == INVALID_SOCKET)
	{
		AfxMessageBox("Socket�� �ʱ�ȭ�� �� �����ϴ�..\n");
		WSACleanup();
		return false;
	}

    // Set the mode of the socket to be nonblocking
    u_long iMode = 1;
    iResult = ioctlsocket(g_ListenSocket, FIONBIO, &iMode);

    if (iResult == SOCKET_ERROR) {
        printf("ioctlsocket failed with error: %d\n", WSAGetLastError());
        closesocket(g_ListenSocket);
        WSACleanup();
        return false;
    }

    // Setup the TCP listening socket
    iResult = bind( g_ListenSocket, result->ai_addr, (int)result->ai_addrlen);

    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(g_ListenSocket);
        WSACleanup();
        return false;
    }

    // no longer need address information
    freeaddrinfo(result);

    // start listening for new clients attempting to connect
    iResult = listen(g_ListenSocket, SOMAXCONN);

    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(g_ListenSocket);
        WSACleanup();
        return false;
    }

/*
	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons((unsigned short)PORT_NUM);
	addr.sin_addr.S_un.S_addr = INADDR_ANY;
	if (bind(g_ListenSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		AfxMessageBox("Socket�� bind�� �� �� �����ϴ�..\n");
		closesocket(g_ListenSocket);
		WSACleanup();
		return false;
	}
	if (listen(g_ListenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		AfxMessageBox("Socket�� listen�� �� �� �����ϴ�..\n");
		closesocket(g_ListenSocket);
		WSACleanup();
		return false;
	}

	int nAddrSize = sizeof(addr);

*/
/*	while(1)
	{
		SOCKET hUser = accept(g_ListenSocket,NULL,NULL);

		//EnterCriticalSection(&cs);

//		SendMessage(pSvrSock->m_pHwnd, UM_ACCEPT, 0, 0);
	}
	*/
	_beginthreadex(NULL, 0, AcceptThread, this, 0, NULL);
	return true;
//	AfxMessageBox("������ Ŭ���̾�Ʈ ���� listen�� �����մϴ�..\n");
}


//////////////////////////////////////////////////////////////////////////////////////////
//
//	AcceptThread
//
//	Desc : Ŭ���̾�Ʈ�� ���������� ����ϴ� ������  
//
unsigned __stdcall CServerSocket::AcceptThread(void* pArg)
{
//	CRITICAL_SECTION cs;
//	InitializeCriticalSection(&cs);
	SOCKADDR_IN addr;
	ZeroMemory(&addr, sizeof(SOCKADDR_IN));
	int nAddrSize = sizeof(addr);
	CUserEvent* userevent = NULL;
	BOOL bNewEvent = TRUE;
	std::map<CUserEvent*, CUserEvent*>::iterator it;
	HANDLE hThread = NULL;										
	int cnt = 0;

	CServerSocket *pSvrSock = (CServerSocket*)pArg;

	while(1)
	{
		SOCKET hUser = accept(pSvrSock->g_ListenSocket,NULL,NULL);

		if (hUser != INVALID_SOCKET) 
		{
			EnterCriticalSection(&pSvrSock->m_cs);
			pSvrSock->m_Conn++;
//			SendMessage(pSvrSock->m_pHwnd, UM_ACCEPT, 0, 0);

	#ifdef _MY_DEBUG
	//		printf("\nŬ���̾�Ʈ ���� ����.. ����� Ŀ�ؼ��� ���� [%d]\n", pSvrSock->m_Conn);
	#endif
			WSAEVENT evt = (WSAEVENT)WSACreateEvent();
			WSAEventSelect(hUser, evt, FD_READ | FD_CLOSE);		// ���ϰ� �̺�Ʈ ��ü ����

			CChildSocket* pChildSock = new CChildSocket;			// �ڽ� ������ ����� 
			pSvrSock->g_users[hUser] = pChildSock;					// g_users�� ���ϰ� �ڽļ����� �����͸� ���� �Ѵ�
			pChildSock->setSocket(hUser);
			pChildSock->SetMainDlg(pSvrSock->m_pDlg);

			if(pSvrSock->child_socket_count > 0){
				pSvrSock->m_childSocket_list.AddHead(pChildSock);
			}else{
				pSvrSock->m_motive_socket = pChildSock;
				pSvrSock->child_socket_count++;
				pChildSock->m_bMotiveSocket = true;
				((CNMsgrSvrDlg*)pSvrSock->m_pDlg)->PrintStatus("motive socket accept");
			}
			
			
			bNewEvent = TRUE;
			it = pSvrSock->g_eventlist.begin();
			while(it != pSvrSock->g_eventlist.end())				// g_eventlist ���� CUserEvent ��ü �����Ͱ� ������ �ִ�
			{														// CUserEvent�� �ִ� �������� MAX_USER_COUNT���� ���� Ŭ������ ������
				userevent = (CUserEvent*)(*it).first;				// �� Ŭ������ ���ο� �̺�Ʈ�� �Ҵ��Ѵ�
				if (userevent->GetEventCount() < MAX_USER_COUNT)
				{
					bNewEvent = FALSE;
//					hThread = userevent->GetThreadHandle();
					break;
				}
				it++;
			}

			if ((int)pSvrSock->g_users.size() == 1 || bNewEvent)	// ���ο� CUserEvent�� ������ �Ѵ� 
			{
				userevent = new CUserEvent;
				pSvrSock->g_eventlist[userevent] = userevent;
				userevent->AddEvent(evt);
				pSvrSock->m_pCurEvent = userevent;

				hThread = (HANDLE)::_beginthreadex(NULL, 0, RecvThread, pSvrSock, 0, NULL);		// CUserEvent�� �����尡 �����ȴ�

				userevent->SetThreadHandle(hThread);

	#ifdef _MY_DEBUG
				printf("�� Receive ������ ����ϴ�. [%d]\n", (int)hThread);
	#endif
			}
			else
				userevent->AddEvent(evt);							// ������ CUserEvent�� ������ �̺�Ʈ�� �߰��Ѵ�

			LeaveCriticalSection(&pSvrSock->m_cs);
		}
	}
//	DeleteCriticalSection(&cs);
	_endthreadex(0);
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
//	RecvThread
//
//	Desc : Ŭ���̾�Ʈ�� recv�� ����ϴ� ������(CUserEvent�� �ϳ��̱� ������ �������� �����Ҽ� �ִ�)  
//
unsigned __stdcall CServerSocket::RecvThread(void* pArg)
{
//	CRITICAL_SECTION cs;
//	InitializeCriticalSection(&cs);
	WSAEVENT* events;
	WSANETWORKEVENTS evt;
	std::map<SOCKET, CChildSocket*>::iterator it;
	DWORD dwRet = 0;
	SOCKET hReqSock;
	CChildSocket* pChildSock = NULL;

	CServerSocket *pSvrSock = (CServerSocket*)pArg;
	CUserEvent* userevent = pSvrSock->m_pCurEvent;

	int Kinect_id;

	while(1)
	{
		if (userevent->GetEventCount() == 0)				// CUserEvent�� �������� 0�̸� �����尡 ����ȴ� 
			break;
		events = userevent->GetEventsHandle();			
		dwRet = WSAWaitForMultipleEvents(userevent->GetEventCount(), events, FALSE, 1, FALSE);	// �̺�Ʈ�� �ִ� WSAEVENT�� ����Ѵ�
		if (dwRet == WSA_WAIT_FAILED)
			continue;
		else if (dwRet == WSA_WAIT_TIMEOUT)
			continue;
		else if (dwRet >= WSA_WAIT_EVENT_0)
		{
			WSAResetEvent(events[dwRet]);					// ������ �̺�Ʈ ��ü�� ��ȣ���� ���� ���·� ���� 
//			EnterCriticalSection(&pSvrSock->m_cs);
			it = pSvrSock->g_users.begin();
			while(it != pSvrSock->g_users.end())
			{
				hReqSock = (SOCKET)(*it).first;				// g_users�� ��ϵ� �����ڵ��� �����´�
				pChildSock = (CChildSocket*)(*it).second;
				if (WSAEnumNetworkEvents(hReqSock, events[dwRet], &evt) == 0)	// �̺�Ʈ�� �� ���Ͽ� � Ÿ���� �̺�Ʈ�� �Դ��� Ȯ���Ѵ�
				{
					if ((evt.lNetworkEvents & FD_READ) == FD_READ)			// ������ ���ۿ�û
					{
						// receive
						pChildSock->OnReceive();								// ���ϰ� ����� �ڽ� ������ OnReceive�Լ� ȣ�� 
						it++;

					}
					else if ((evt.lNetworkEvents & FD_CLOSE) == FD_CLOSE)		// disconnect ��û 
					{
						// close
//						EnterCriticalSection(&pSvrSock->m_cs);
						pSvrSock->m_Conn--;
						POSITION pos = pSvrSock->m_childSocket_list.GetHeadPosition();
						while (pos)
						{
							CChildSocket* pCurChildSock;
							pCurChildSock = pSvrSock->m_childSocket_list.GetNext(pos);
							if (pChildSock == pCurChildSock) {
								pSvrSock->m_childSocket_list.RemoveAt(pos);
								break;
							}
						}
						Kinect_id = pChildSock->m_Kinect_id;
						userevent->DeleteEvent(dwRet);
						pSvrSock->g_users.erase((SOCKET)(*it++).first);
						closesocket(hReqSock);
						delete pChildSock;
						((CNMsgrSvrDlg*)pSvrSock->m_pDlg)->OnDisconnect(Kinect_id);
//						LeaveCriticalSection(&pSvrSock->m_cs);

					}else
						it++;
//					::WSACloseEvent((HANDLE)event.lNetworkEvents);
				}

			}
//			LeaveCriticalSection(&pSvrSock->m_cs);
		}
	}
	EnterCriticalSection(&pSvrSock->m_cs);
	pSvrSock->g_eventlist.erase(userevent);
	delete userevent;
	userevent = NULL;
	LeaveCriticalSection(&pSvrSock->m_cs);
//	DeleteCriticalSection(&cs);
	_endthreadex(0);
	return 0;
}

