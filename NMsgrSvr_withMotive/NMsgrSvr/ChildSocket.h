// ChildSocket.h: interface for the CChildSocket class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CHILDSOCKET_H__2B530628_2A7F_4E61_9CE4_C6537BD590AC__INCLUDED_)
#define AFX_CHILDSOCKET_H__2B530628_2A7F_4E61_9CE4_C6537BD590AC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CChildSocket  
{
public:
	CChildSocket();
	virtual ~CChildSocket();

public:
	CRITICAL_SECTION		m_cs;
	SOCKET			m_hSocket;
//	BYTE			m_byData[10][512];
	DWORD			m_dwThID;
	DWORD			m_dwServerID;
	CDialog*		m_pDlg;
	HANDLE			m_hFile;
	int				m_nRest;
	BYTE*			m_byRest;
	CPtrList		m_lstQueue;
	CString			m_strID;
	CString			m_strName;
	CString			m_strGlobal;
	CString			m_strPrivate;
	CWinThread*		m_pThread;
	int				m_Kinect_id;
	SkeletonInfo	m_LastSkeletonInfo;
	BYTE*			m_skel_data;
	int				m_skelDataCount;
	bool			m_bReceiveNewData;
	bool			m_bMotiveSocket;
//	std::map<SkeletonInfo, SkeletonInfo> m_skeleton_map;
	CList<SkeletonInfo*, SkeletonInfo*> m_skeletonList;


public:
	void SetMainDlg(CDialog* pDlg);
	void OnConnect();
	void OnReceive();
	void setSocket(SOCKET socket) { m_hSocket = socket; };
	void CloseSocket();
	DWORD Send(const LPBYTE lpBuffer, DWORD dwCount, DWORD dwTimeout);
	void SendSystemTime();
	void receive_skeleton(BYTE* skel_data);

	static UINT ReceiveThread(LPVOID pParam);
// Method
protected:
	void Begin_Thread();

};

#endif // !defined(AFX_CHILDSOCKET_H__2B530628_2A7F_4E61_9CE4_C6537BD590AC__INCLUDED_)
