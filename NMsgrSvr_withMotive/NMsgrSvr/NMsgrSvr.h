
// NMsgrSvr.h : PROJECT_NAME ���� ���α׷��� ���� �� ��� �����Դϴ�.
//

#pragma once

#ifndef __AFXWIN_H__
	#error "PCH�� ���� �� ������ �����ϱ� ���� 'stdafx.h'�� �����մϴ�."
#endif

#include "resource.h"		// �� ��ȣ�Դϴ�.


// CNMsgrSvrApp:
// �� Ŭ������ ������ ���ؼ��� NMsgrSvr.cpp�� �����Ͻʽÿ�.
//

class CNMsgrSvrApp : public CWinApp
{
public:
	CNMsgrSvrApp();

// �������Դϴ�.
public:
	virtual BOOL InitInstance();

// �����Դϴ�.

	DECLARE_MESSAGE_MAP()
};

extern CNMsgrSvrApp theApp;