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
//	Desc : 이벤트를 추가 
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
//	Desc : 총 이벤트수를 반환
//
int CUserEvent::GetEventCount()
{
	return nIdx;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
//	GetEventCount
//		
//	Desc :	userevent를 반환
//
WSAEVENT* CUserEvent::GetEventsHandle()
{
	return userevent;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
//	DeleteEvent
//
//	Desc : dwIdx값의 이벤트를 삭제
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
//	Desc : m_hThread값을 설정
//
void CUserEvent::SetThreadHandle(HANDLE Thread)
{
	m_hThread = Thread;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
//	GetThreadHandle
//
//	Desc : m_hThread을 반환
//
HANDLE CUserEvent::GetThreadHandle()
{
	return m_hThread;
}
