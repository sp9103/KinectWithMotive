#include "StdAfx.h"
#include "userevent.h"

CUserEvent::CUserEvent(void)
{
	nIdx = 0;
}

CUserEvent::~CUserEvent(void)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
//
//	AddEvent
//
//	Desc : �̺�Ʈ�� �߰� 
//
void CUserEvent::AddEvent(WSAEVENT& event)
{
	eventmap[event] = event;
	userevent[nIdx] = event;
	nIdx++;

}

//////////////////////////////////////////////////////////////////////////////////////////
//
//	GetEventCount
//
//	Desc : �� �̺�Ʈ���� ��ȯ
//
int CUserEvent::GetEventCount()
{
	return nIdx;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
//	GetEventCount
//		
//	Desc :	userevent�� ��ȯ
//
WSAEVENT* CUserEvent::GetEventsHandle()
{
	return userevent;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
//	DeleteEvent
//
//	Desc : dwIdx���� �̺�Ʈ�� ����
//
void CUserEvent::DeleteEvent(DWORD dwIdx)
{
	std::map<WSAEVENT, WSAEVENT>::iterator it;
	eventmap.erase(userevent[dwIdx]);
	WSACloseEvent(userevent[dwIdx]);
	nIdx--;

	int nNewIdx = 0;
	it = eventmap.begin();
	while (it != eventmap.end())
	{
		userevent[nNewIdx] = (WSAEVENT)(*it).first;
		nNewIdx++;
		it++;
	}
/*	while (it != eventmap.end())
	{
		userevent[nNewIdx] = userevent[nNewIdx+1];
		nNewIdx++;
		it++;
	}*/
}

//////////////////////////////////////////////////////////////////////////////////////////
//
//	SetThreadHandle
//
//	Desc : m_hThread���� ����
//
void CUserEvent::SetThreadHandle(HANDLE Thread)
{
	m_hThread = Thread;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
//	GetThreadHandle
//
//	Desc : m_hThread�� ��ȯ
//
HANDLE CUserEvent::GetThreadHandle()
{
	return m_hThread;
}
