#pragma once

#include "stdafx.h"

//using namespace std;

class CUserEvent
{
public:
	CUserEvent(void);
	~CUserEvent(void);
public:
	HANDLE GetThreadHandle();						// m_hThread을 반환
	void SetThreadHandle(HANDLE Thread);			// m_hThread값을 설정
	void AddEvent(WSAEVENT& event);					// 이벤트를 추가 
	int GetEventCount();							// 총 이벤트수를 반환
	WSAEVENT* GetEventsHandle();					// userevent를 반환
	void DeleteEvent(DWORD dwIdx);					// dwIdx값의 이벤트를 삭제

	WSAEVENT userevent[MAX_USER_COUNT];							// 각 소켓의 소켓 이벤트 
protected:
	std::map<WSAEVENT, WSAEVENT> eventmap;			// 이벤트 array
	
	int nIdx;										// 총 소켓의 수 
	HANDLE m_hThread;								// CUserEvent당 recv쓰레드가 하나이기 때문에 쓰레드의 핸들
};
