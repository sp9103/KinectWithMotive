#pragma once

#include "stdafx.h"

//using namespace std;

class CUserEvent
{
public:
	CUserEvent(void);
	~CUserEvent(void);
public:
	HANDLE GetThreadHandle();						// m_hThread�� ��ȯ
	void SetThreadHandle(HANDLE Thread);			// m_hThread���� ����
	void AddEvent(WSAEVENT& event);					// �̺�Ʈ�� �߰� 
	int GetEventCount();							// �� �̺�Ʈ���� ��ȯ
	WSAEVENT* GetEventsHandle();					// userevent�� ��ȯ
	void DeleteEvent(DWORD dwIdx);					// dwIdx���� �̺�Ʈ�� ����

	WSAEVENT userevent[MAX_USER_COUNT];							// �� ������ ���� �̺�Ʈ 
protected:
	std::map<WSAEVENT, WSAEVENT> eventmap;			// �̺�Ʈ array
	
	int nIdx;										// �� ������ �� 
	HANDLE m_hThread;								// CUserEvent�� recv�����尡 �ϳ��̱� ������ �������� �ڵ�
};
