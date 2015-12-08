// ClntSocket.h: interface for the CClntSocket class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CLNTSOCKET_H__6B8F2092_094A_4892_B7A0_F0E4D77C386C__INCLUDED_)
#define AFX_CLNTSOCKET_H__6B8F2092_094A_4892_B7A0_F0E4D77C386C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CClntSocket  
{
public:
	CClntSocket();
	virtual ~CClntSocket();

//method
	bool ConnectTo(LPCTSTR strDestination, LPCTSTR strServiceName, int nProtocol, int nType);
	bool IsOpen() const;
	bool GetLocalName(LPTSTR strName, UINT nSize);
	ULONG GetIPAddress( LPCTSTR strHostName );
	USHORT GetPortNumber( LPCTSTR strServiceName );
	bool  WatchConn();
	DWORD ReadData(LPBYTE lpBuffer, DWORD dwSize, DWORD dwTimeout);
	UINT HandleReadData(void* pArg);
	void StopConn();
	void CloseConn();
	DWORD sendData(const LPBYTE lpBuffer, DWORD dwCount, DWORD dwTimeout);
	void SetWnd(CDialog* pDlg); 

	static unsigned __stdcall OnReceive(void* pArg);

//attribute
	CDialog*		m_pDlg;		// CCNMsrgClntDlg �ڵ鰪 
	CPtrList		m_lstQueue;
	HANDLE			m_hSock;		// socket handle
	HANDLE			m_hThread;
	int				m_nRest;
	BYTE*			m_byRest;
	CString			m_strPath;
	CString			m_strGlobal;
	DWORD			m_dwFileSize;
};

#endif // !defined(AFX_CLNTSOCKET_H__6B8F2092_094A_4892_B7A0_F0E4D77C386C__INCLUDED_)
