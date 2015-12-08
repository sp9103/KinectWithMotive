#if !defined(AFX_REGISTRY_H__00E00000_56E7_4283_8EC6_F12FFFBAFCDE__INCLUDED_)
#define AFX_REGISTRY_H__00E00000_56E7_4283_8EC6_F12FFFBAFCDE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Registry.h : header file
//



/////////////////////////////////////////////////////////////////////////////
// CRegistry command target

class CRegistry : public CCmdTarget
{
	DECLARE_DYNCREATE(CRegistry)

	CRegistry();           // protected constructor used by dynamic creation
	virtual ~CRegistry();

// Attributes
public:
	struct UserInfo {
		CString id;
		CString pass;
		CString name;
		CString global;
		CString pri;
	} user;

protected:
	struct ValueName {
		CString id;
		CString pass;
		CString name;
		CString global;
		CString pri;
	} value;

	CString m_strKey;
	
	HWND m_hWnd;

// Operations
public:
	BOOL LoginCheck(CString id, CString pass);
	void AllUser();
	BOOL UserCheck(CString str);
	LPVOID UserModify(CString str);
	void UserCreate();
	void UserDelete(CString str);

	void SetWnd(HWND hwnd) { m_hWnd = hwnd; }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRegistry)
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CRegistry)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REGISTRY_H__00E00000_56E7_4283_8EC6_F12FFFBAFCDE__INCLUDED_)
