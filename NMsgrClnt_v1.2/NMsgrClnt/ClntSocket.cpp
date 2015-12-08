// ClntSocket.cpp: implementation of the CClntSocket class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NMsgrClntDlg.h"
#include "ClntSocket.h"
//#include <process.h>
//#include "FTFile.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CClntSocket::CClntSocket()
{
	m_hSock = INVALID_HANDLE_VALUE;
	m_strPath = "";
	m_strGlobal = "";
	m_nRest = 0;
	m_byRest = NULL;
	m_dwFileSize = 0;
}

CClntSocket::~CClntSocket()
{

}

bool CClntSocket::IsOpen() const
{
	return ( INVALID_HANDLE_VALUE != m_hSock );
}

void CClntSocket::SetWnd(CDialog* pDlg) 
{
	m_pDlg = pDlg;
}

///////////////////////////////////////////////////////////////////////////////
// ConnectTo
///////////////////////////////////////////////////////////////////////////////
// DESCRIPTION:
//				Establish connection with a server service or port
// PARAMETERS:
//	LPCTSTR strDestination: hostname or address to connect (in .dot format)
//	LPCTSTR strServiceName: Service name or port number
//	int nProtocol: protocol to use (set to AF_INET)
//	int nType: type of socket to create (SOCK_STREAM, SOCK_DGRAM)
///////////////////////////////////////////////////////////////////////////////
bool CClntSocket::ConnectTo(LPCTSTR strDestination, LPCTSTR strServiceName, int nProtocol, int nType)
{
	// Socket is already opened
	if ( IsOpen() )
		return false;


	int client_len;
    int client_sockfd;

    struct sockaddr_in clientaddr;

    client_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    clientaddr.sin_family = AF_INET;
//    clientaddr.sin_addr.s_addr = inet_addr("192.168.1.2");
	clientaddr.sin_addr.s_addr = inet_addr(strDestination);
    clientaddr.sin_port = htons(GetPortNumber( strServiceName ));

    client_len = sizeof(clientaddr);

    if (connect(client_sockfd, (struct sockaddr *)&clientaddr, client_len) < 0)
    {
//        perror("Connect error: ");
		closesocket( client_sockfd );
        return false;
    }else {
		m_hSock = (HANDLE)client_sockfd;
		return true;
	}
/*
	SOCKADDR_IN sockAddr = { 0 };

	// Create a Socket that is bound to a specific service provide
	// nProtocol: (AF_INET)
	// nType: (SOCK_STREAM, SOCK_DGRAM)
	SOCKET sock = socket(nProtocol, nType, 0);
	if (INVALID_SOCKET != sock)
	{
		// Associate a local address with the socket
		TCHAR strHost[HOSTNAME_SIZE] = { 0 };

		sockAddr.sin_family = nProtocol;

		// Now get destination address or continue with local address
		if ( strDestination[0]) {
			sockAddr.sin_addr.s_addr = htonl(CClntSocket::GetIPAddress( strDestination ) );
		}

		if ( SOCKET_ERROR == bind(sock, (LPSOCKADDR)&sockAddr, sizeof(SOCKADDR_IN)))
		{
			closesocket( sock );
			return false;
		}

		// Get port number based on service name or port string
		sockAddr.sin_port = htons( GetPortNumber( strServiceName ) );
		if ( 0 != sockAddr.sin_port )
		{
			// try to connect - if fail, server not ready
			if (SOCKET_ERROR == connect( sock, (LPSOCKADDR)&sockAddr, sizeof(SOCKADDR_IN)))
			{
				closesocket( sock );
				return false;
			}

			// Success, now we may save this socket
			m_hSock = (HANDLE)sock;
			return true;
		}
	}*/
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// StopComm
///////////////////////////////////////////////////////////////////////////////
// DESCRIPTION:
//		Close Socket and Stop Communication thread
// PARAMETERS:
//		None
///////////////////////////////////////////////////////////////////////////////
void CClntSocket::StopConn()
{
	// Close Socket
	if (IsOpen())
	{
		CloseConn();
		Sleep(50);

		if (WaitForSingleObject(m_hThread, 5000L) == WAIT_TIMEOUT)
			TerminateThread(m_hThread, 1L);
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////
// CloseConn
///////////////////////////////////////////////////////////////////////////////
// DESCRIPTION:
//		Close Socket Communication
// PARAMETERS:
//		None
///////////////////////////////////////////////////////////////////////////////
void CClntSocket::CloseConn()
{
	if (IsOpen())
	{
		shutdown((SOCKET)m_hSock, SD_BOTH);
		closesocket( (SOCKET)m_hSock );
		m_hSock = INVALID_HANDLE_VALUE;
	}
}

///////////////////////////////////////////////////////////////////////////////
// GetLocalName
///////////////////////////////////////////////////////////////////////////////
// DESCRIPTION:
//				Get local computer name.  Something like: "mycomputer.myserver.net"
// PARAMETERS:
//	LPTSTR strName: name of the computer is returned here
//	UINT nSize: size max of buffer "strName"
///////////////////////////////////////////////////////////////////////////////
bool CClntSocket::GetLocalName(LPTSTR strName, UINT nSize)
{
	if (strName != NULL && nSize > 0)
	{
		char strHost[HOSTNAME_SIZE] = { 0 };

		// get host name, if fail, SetLastError is set
		if (SOCKET_ERROR != gethostname(strHost, sizeof(strHost)))
		{
			struct hostent* hp;
			hp = gethostbyname(strHost);
			if (hp != NULL)	{
				strcpy(strHost, hp->h_name);
			}

			// check if user provide enough buffer
			if (strlen(strHost) > nSize)
			{
				SetLastError(ERROR_INSUFFICIENT_BUFFER);
				return false;
			}

			// Unicode conversion
#ifdef _UNICODE
			return (0 != MultiByteToWideChar(CP_ACP, 0, strHost, -1, strName, nSize, NULL, NULL ));
#else
			_tcscpy(strName, strHost);
			return true;
#endif
		}
	}
	else
		SetLastError(ERROR_INVALID_PARAMETER);
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// GetIPAddress
///////////////////////////////////////////////////////////////////////////////
// DESCRIPTION:
//		Returns an IP address.
//			- It tries to convert the string directly
//			- If that fails, it tries to resolve it as a hostname
// PARAMETERS:
//	LPCTSTR strHostName: host name to get IP address
///////////////////////////////////////////////////////////////////////////////
ULONG CClntSocket::GetIPAddress( LPCTSTR strHostName )
{
	LPHOSTENT	lphostent;
	ULONG		uAddr = INADDR_NONE;
	
	if ( NULL != strHostName )
	{
#ifdef _UNICODE
		char strHost[HOSTNAME_SIZE] = { 0 };
		WideCharToMultiByte(CP_ACP, 0, strHostName, -1, strHost, sizeof(strHost), NULL, NULL );
#else
		LPCTSTR strHost = strHostName;
#endif
		// Check for an Internet Protocol dotted address string
		uAddr = inet_addr( strHost );

		if ( (INADDR_NONE == uAddr) && (strcmp( strHost, "255.255.255.255" )) )
		{
			// It's not an address, then try to resolve it as a hostname
			if ( lphostent = gethostbyname( strHost ) )
				uAddr = *((ULONG *) lphostent->h_addr_list[0]);
		}
	}
	
	return ntohl( uAddr );
}

///////////////////////////////////////////////////////////////////////////////
// GetPortNumber
///////////////////////////////////////////////////////////////////////////////
// DESCRIPTION:
//				Returns a port number based on service name or port number string
// PARAMETERS:
//	LPCTSTR strServiceName: Service name or port string
///////////////////////////////////////////////////////////////////////////////
USHORT CClntSocket::GetPortNumber( LPCTSTR strServiceName )
{
	LPSERVENT	lpservent;
	USHORT		nPortNumber = 0;

	if ( _istdigit( strServiceName[0] ) ) {
		nPortNumber = (USHORT) _ttoi( strServiceName );
	}
	else {
#ifdef _UNICODE
		char pstrService[HOSTNAME_SIZE];
		WideCharToMultiByte(CP_ACP, 0, pstrService, -1, strServiceName, sizeof(pstrService), NULL, NULL );
#else
		LPCTSTR pstrDevice = strServiceName;
#endif
		// Convert network byte order to host byte order
		if ( (lpservent = getservbyname( pstrDevice, NULL )) != NULL )
			nPortNumber = ntohs( lpservent->s_port );
	}

	return nPortNumber;
}

///////////////////////////////////////////////////////////////////////////////
// WatchComm	
///////////////////////////////////////////////////////////////////////////////
// DESCRIPTION:
//		Starts Socket Communication Working thread
// PARAMETERS:
//		None
///////////////////////////////////////////////////////////////////////////////
bool CClntSocket::WatchConn()
{
	if (IsOpen())
	{
		HANDLE hThread;
		UINT uiThreadId = 0;
		hThread = (HANDLE)_beginthreadex(NULL,	// Security attributes
					 0,	// stack
					OnReceive,			// Thread proc
					this,	// Thread param
					0,	// creation mode
					NULL);	// Thread ID

		if ( NULL != hThread)
		{
			//SetThreadPriority(hThread, THREAD_PRIORITY_HIGHEST);
			ResumeThread( hThread );
			m_hThread = hThread;
			return true;
		}
	}

	return false;
}

unsigned __stdcall CClntSocket::OnReceive(void* pArg)
{
	BYTE	buffer[BUFFER_SIZE];
	DWORD	dwBytes  = 0L;

	DWORD	dwTimeout = DEFAULT_TIMEOUT;
	CClntSocket* pClntSocket = (CClntSocket*)pArg;

	while( pClntSocket->IsOpen() )
	{
		// Blocking mode: Wait for event
		dwBytes = pClntSocket->ReadData(buffer, sizeof(buffer), dwTimeout);

		// Error? - need to signal error
		if (dwBytes == (DWORD)-1)
		{
		}

		while (pClntSocket->m_lstQueue.GetCount() !=0 ) {
			pClntSocket->HandleReadData(pArg);
		}

		Sleep(0);

	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// ReadData
///////////////////////////////////////////////////////////////////////////////
// DESCRIPTION:
//		Reads the Socket Communication
// PARAMETERS:
//		LPBYTE lpBuffer: buffer to place new data
//		DWORD dwSize: maximum size of buffer
//		DWORD dwTimeout: timeout to use in millisecond
///////////////////////////////////////////////////////////////////////////////
DWORD CClntSocket::ReadData(LPBYTE lpBuffer, DWORD dwSize, DWORD dwTimeout)
{
	_ASSERTE( IsOpen() );
	_ASSERTE( lpBuffer != NULL );

	// TODO: Add your specialized code here and/or call the base class
	WORD wSize=0, wTotal=0;
	int i=0, nSum=0, nAdd=0;
	ULONG nReceive=0;
	UINT nRec=0;
	char *byReceive=NULL;
	DWORD dwBytesRead = 0L;

    ioctlsocket((SOCKET)m_hSock, FIONREAD, &nReceive);

	byReceive = new char[nReceive];
	memset(byReceive, 0x00, nReceive);
	nRec = recv((SOCKET)m_hSock, byReceive, nReceive, 0);
	
	memcpy(&wSize, byReceive, sizeof(WORD));

	if (wSize==nRec)
	{
		BYTE *pData = new BYTE[wSize];
		memcpy(pData, byReceive, nRec);
		if (m_lstQueue.IsEmpty())
			m_lstQueue.AddHead(pData);
		else 
			m_lstQueue.AddTail(pData);
	}
	else
	{
		while (nAdd < (int)nRec)
		{		
			BYTE *pData = new BYTE[wSize];
			memset(pData, 0x00, wSize);

			for (int i=0; i<wSize; i++)
			{
				pData[i] = byReceive[nAdd];
				if (i==wSize-1)
				{	// 루프가 패킷의 전체 크기 만큼 돌았다면 
					if (pData[i-1]==0xef && pData[i]==0xcd)
					{
						if (m_lstQueue.IsEmpty())
							m_lstQueue.AddHead(pData);
						else 
							m_lstQueue.AddTail(pData);
					}
				}
				nAdd++;
			}
			nSum += wSize; 
			memcpy(&wSize, &byReceive[nAdd], sizeof(WORD));					
		}
	}
	return dwBytesRead;
}

/*-----------------------------------------------------------------------
| Thread Function
+------------------------------------------------------------------------
| ReceiveThread : 받은 Data의 Command 별 구분 
| argument      : 클라이언트 소켓의 포인터 
+----------------------------------------------------------------------*/

UINT CClntSocket::HandleReadData(void* pArg)
{
	CClntSocket* pClntSocket = (CClntSocket*)pArg;
	BYTE *nAddress, byList;
	//FileList filelist;
	WORD wData, wName, wTotal;	// wData, wName 여러가지 타입의 데이터 사이즈를 저장하는 용도로 쓰임
	//CFTFile	ftFile;						// wTotal 받는 데이터의 총 사이즈

	BYTE *byReceive=NULL, *byStore=NULL;
	byStore = (BYTE*)m_lstQueue.RemoveHead();
	memcpy(&wTotal, byStore, sizeof(WORD));
	byReceive = new BYTE[wTotal];
	memset(byReceive, 0x00, wTotal);
	memcpy(byReceive, byStore, wTotal);
	delete byStore;
								
	byList = byReceive[3];
	switch(byReceive[2])
	{
		case TIME_INFO:
		{
			BYTE byTime[16];
			nAddress=byReceive+5;					memcpy(&wData, nAddress, sizeof(WORD));
			nAddress+=sizeof(WORD);					memcpy(&byTime, nAddress, wData);	

			CString str;
			SYSTEMTIME systime;
			GetSystemTime(&systime);
			str.Format("local min : %d, sec : %d, milsec: %d", systime.wMinute, systime.wSecond, systime.wMilliseconds);
			((CNMsgrClntDlg*)pClntSocket->m_pDlg)->PrintStatus(str);

			memcpy(&systime, byTime, sizeof(SYSTEMTIME));
			((CNMsgrClntDlg*)pClntSocket->m_pDlg)->Receive_SysTime(systime);
			memcpy(&systime, byTime, sizeof(SYSTEMTIME));

			SetSystemTime(&systime);
//			str.Format("server min : %d, sec : %d, milsec: %d", systime.wMinute, systime.wSecond, systime.wMilliseconds);
//			((CNMsgrClntDlg*)pClntSocket->m_pDlg)->PrintStatus(str);
/*
A possible solution is turning on UAC in the Linker Options:
Open the project-properties dialog, go to Linker->Manifest and set UAC to "requireAdministrator".
The downside is you then have to run Visual Studio as Administrator to debug your program
*/

			GetSystemTime(&systime);
			str.Format("local min : %d, sec : %d, milsec: %d", systime.wMinute, systime.wSecond, systime.wMilliseconds);
			((CNMsgrClntDlg*)pClntSocket->m_pDlg)->PrintStatus(str);

			break;
		}

		case SKELETON_DATA_RECEIVED:
		{
//			nAddress=byReceive+5;						

			((CNMsgrClntDlg*)pClntSocket->m_pDlg)->m_bReceiveSkeletonMsg = true;

			break;
		}

		case CLIENT_INFO:
		{
			if (byList==CLIENT_INFO_OK)
			{
				nAddress=byReceive+sizeof(WORD)*2;		memcpy(&wData, nAddress, sizeof(WORD));
				nAddress+=sizeof(WORD);					memcpy(&wName, nAddress, sizeof(WORD));
				nAddress+=sizeof(WORD);					memcpy(m_strPath.GetBuffer(wData), nAddress, wData);
				nAddress+=wData;						memcpy(m_strGlobal.GetBuffer(wName), nAddress, wName);
				m_strPath.ReleaseBuffer();
				m_strGlobal.ReleaseBuffer();
				
				wTotal = HEAD_SIZE + EFCD_SIZE;							
				
				BYTE *nAdd = NULL;
				BYTE *bySend = new BYTE[wTotal];
				memset(bySend, 0x00, wTotal);		memcpy(bySend, &wTotal, sizeof(WORD));
/*				nAdd=bySend+sizeof(WORD);		*nAdd = FILE_LIST;	
				nAdd++;							*nAdd = FILE_CONFIRM;
				nAdd++;							memset(nAdd, 0x00, sizeof(WORD));
				nAdd+=sizeof(WORD);				memset(nAdd, 0x00, sizeof(WORD)); 
				nAdd+=sizeof(WORD);				*nAdd = 0xef;
				nAdd++;							*nAdd = 0xcd;
				sendData(bySend, wTotal, 0);
*/
				delete bySend;
			}					
			else
			{
				CString strMsg = "로그인에 실패하였습니다.";
//				SendMessage(m_pHwnd, UM_SHOW_MESSAGE, 0, (LPARAM)&strMsg);
//				SendMessage(m_pHwnd, UM_LOGIN_ERR, 0, 0);
			}
			break;
		}
	} // switch end			
	delete byReceive;

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// WriteComm
///////////////////////////////////////////////////////////////////////////////
// DESCRIPTION:
//		Writes data to the Socket Communication
// PARAMETERS:
//		const LPBYTE lpBuffer: data to write
//		DWORD dwCount: maximum characters to write
//		DWORD dwTimeout: timeout to use in millisecond
///////////////////////////////////////////////////////////////////////////////
DWORD CClntSocket::sendData(const LPBYTE lpBuffer, DWORD dwCount, DWORD dwTimeout)
{
	_ASSERTE( IsOpen() );
	_ASSERTE( NULL != lpBuffer );

	// Accept 0 bytes message
	if (!IsOpen() || NULL == lpBuffer)
		return 0L;

	fd_set	fdWrite  = { 0 };
	TIMEVAL	stTime;
	TIMEVAL	*pstTime = NULL;

	if ( INFINITE != dwTimeout ) {
		stTime.tv_sec = 0;
		stTime.tv_usec = dwTimeout*1000;
		pstTime = &stTime;
	}

	SOCKET s = (SOCKET) m_hSock;
	// Set Descriptor
	if ( !FD_ISSET( s, &fdWrite ) )
		FD_SET( s, &fdWrite );

	// Select function set write timeout
	DWORD dwBytesWritten = 0L;
	int res = select( s+1, NULL, &fdWrite, NULL, pstTime );
	if ( res > 0)
	{
		res = send( s, (LPCSTR)lpBuffer, dwCount, 0);

		dwBytesWritten = (DWORD)((res >= 0)?(res) : (-1));
	}

	return dwBytesWritten;
}

