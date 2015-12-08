// ServerSocket.h: interface for the CServerSocket class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SERVERSOCKET_H__23F1D587_1305_4947_8178_4FFB201B4E4C__INCLUDED_)
#define AFX_SERVERSOCKET_H__23F1D587_1305_4947_8178_4FFB201B4E4C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <string>
#include "userevent.h"
#include "ChildSocket.h"
//using namespace std;

class CServerSocket  
{

public:
	SOCKET g_ListenSocket;								// listen ����
	std::map<CUserEvent*, CUserEvent*> g_eventlist;		// CUserEvent�� ���� �ڷḦ �����ϴ� map
	std::map<SOCKET, CChildSocket*> g_users;			// ���ϰ� ���Ͽ� ���� �ڼ� Ŭ������ �����ϴ� map 
	CUserEvent* m_pCurEvent;
	CDialog*	m_pDlg;
	int			m_Conn;
	CList<CChildSocket*, CChildSocket*> m_childSocket_list;
	CChildSocket*	m_motive_socket;
	int	child_socket_count;

	CRITICAL_SECTION	m_cs;
public:
	CServerSocket();
	virtual ~CServerSocket();

public:
	bool InitListen();
	void SetMainDlg(CDialog* pDlg) { m_pDlg = pDlg; };

	static unsigned __stdcall AcceptThread(void* pArg);		// Ŭ���̾�Ʈ�� ���������� ����ϴ� ������ 
	static unsigned __stdcall RecvThread(void* pArg);
};

#endif // !defined(AFX_SERVERSOCKET_H__23F1D587_1305_4947_8178_4FFB201B4E4C__INCLUDED_)
