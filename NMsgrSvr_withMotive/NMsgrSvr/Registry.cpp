// Registry.cpp : implementation file
//

#include "stdafx.h"
#include "nmsgrsvr.h"
#include "Registry.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRegistry

IMPLEMENT_DYNCREATE(CRegistry, CCmdTarget)

CRegistry::CRegistry()
{
	m_strKey = "Software\\CNMsgrSvr";	

	user.id = "";
	user.pass = "";
	user.name = "";
	user.global = "";
	user.pri = "";
	
	value.id = "id";
	value.pass = "password";
	value.name = "name";
	value.global = "global_path";
	value.pri = "private_path";
}

CRegistry::~CRegistry()
{

}


BEGIN_MESSAGE_MAP(CRegistry, CCmdTarget)
	//{{AFX_MSG_MAP(CRegistry)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRegistry message handlers


/////////////////////////////////////////////////////////////////////////////
// CRegistry

/*--------------------------------------------------------------
| UserCreate : 사용자 정보 추가
+-------------------------------------------------------------*/
void CRegistry::UserCreate()
{
	HKEY hKeyExt;
	DWORD dwDisposition=0;
	CHAR szID[13], szPass[13], szName[26], szGlobal[30], szPrivate[50];
	CString strSubKey="", strSub="";

	strSubKey = m_strKey + "\\" + user.id;
	RegCreateKeyEx(HKEY_CURRENT_USER, (LPCTSTR)strSubKey, 
					0, "",	REG_OPTION_NON_VOLATILE, 
					KEY_ALL_ACCESS, NULL, &hKeyExt, &dwDisposition);

	strcpy(szID, (LPCTSTR)user.id);
	RegSetValueEx(hKeyExt, value.id, 0, REG_EXPAND_SZ, (LPBYTE)szID, strlen(szID));  

	strcpy(szPass, (LPCTSTR)user.pass);
	RegSetValueEx(hKeyExt, value.pass, 0, REG_EXPAND_SZ, (LPBYTE)szPass, strlen(szPass));  

	strcpy(szName, (LPCTSTR)user.name);
	RegSetValueEx(hKeyExt, value.name, 0, REG_EXPAND_SZ, (LPBYTE)szName, strlen(szName));

	strcpy(szGlobal, (LPCTSTR)user.global);
	RegSetValueEx(hKeyExt, value.global, 0, REG_EXPAND_SZ, (LPBYTE)szGlobal, strlen(szGlobal));

	strcpy(szPrivate, (LPCTSTR)user.pri);
	RegSetValueEx(hKeyExt, value.pri, 0, REG_EXPAND_SZ, (LPBYTE)szPrivate, strlen(szPrivate));
}

/*--------------------------------------------------------------
| UserDelete : 사용자 정보 삭제 
| argument   : 사용자 아이디 
+-------------------------------------------------------------*/
void CRegistry::UserDelete(CString str)
{
	CString strKey="";
	strKey = m_strKey + "\\" + str;

	RegDeleteKey(HKEY_CURRENT_USER, (LPCTSTR)strKey);	
}

/*--------------------------------------------------------------
| UserModify : 사용자 정보 수정 (수정전 사용자 정보 알아내기)
| argument   : 사용자 아이디 
+-------------------------------------------------------------*/
LPVOID CRegistry::UserModify(CString str)
{
	CString strKey="";
	HKEY hNewKey;
	LPSTR lpszName = new char[13];
	LPSTR lpszData = new char[51];
	LONG lReturn=0;
	DWORD dwName=13, dwData=51, dwIndex=0;

	strKey = m_strKey + "\\" + str;
	
	if (RegOpenKeyEx(HKEY_CURRENT_USER, (LPCTSTR)strKey,
		0, KEY_ALL_ACCESS, &hNewKey) == ERROR_SUCCESS)
	{
		do 
		{
			memset(lpszName, 0x00, 13);
			lReturn = RegEnumValue(hNewKey, dwIndex, lpszName, &dwName, 0, NULL, (LPBYTE)lpszData, &dwData);

			if (lpszName == value.id)
				memcpy(user.id.GetBuffer(dwData), lpszData, dwData);

			else if (lpszName == value.pass)
				memcpy(user.pass.GetBuffer(dwData), lpszData, dwData);

			else if (lpszName == value.name)
				memcpy(user.name.GetBuffer(dwData), lpszData, dwData);
				
			else if (lpszName == value.global)
				memcpy(user.global.GetBuffer(dwData), lpszData, dwData);
			
			else if (lpszName == value.pri)
				memcpy(user.pri.GetBuffer(dwData), lpszData, dwData);
						
			dwName = 13;
			dwData = 51;
			dwIndex++;
		} while (!(lReturn == ERROR_NO_MORE_ITEMS));
		RegCloseKey(hNewKey);

	}
	delete lpszName;
	delete lpszData;

	return &user;
}

/*--------------------------------------------------------------
| UserCheck : 아이디 중복 체크 
| argument  : 추가나 수정할 새 아이디 
+-------------------------------------------------------------*/
BOOL CRegistry::UserCheck(CString str)
{
	BOOL bReturn = FALSE;
	HKEY hKey;
	LPTSTR lpszSub = new char[13];
	LONG lReturn=0;
	DWORD dwIndex=0, dwSub=13;
	CString strReturn="";

	str.TrimLeft();
	str.TrimRight();

	if (RegOpenKeyEx(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, 
		0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
	{
		do 
		{
			lReturn = RegEnumKeyEx(hKey, dwIndex, lpszSub, &dwSub, NULL, NULL, NULL, NULL);
						
			memcpy(strReturn.GetBuffer(13), lpszSub, 13);
			strReturn.TrimLeft();
			strReturn.TrimRight();
			strReturn.MakeUpper();
			str.TrimLeft();
			str.TrimRight();
			str.MakeUpper();
			if (strReturn == str)
				bReturn = TRUE;
			
			dwIndex++;
			dwSub = 13;	
		} while (!(lReturn == ERROR_NO_MORE_ITEMS));
		
		RegCloseKey(hKey);
	}		
	
	delete lpszSub;
	return bReturn;
}

/*--------------------------------------------------------------
| AllUser : 레지스트리에 등록된 모든 사용자 찾기
+-------------------------------------------------------------*/
void CRegistry::AllUser()
{
	HKEY hKey, hNewKey;
	LPTSTR lpszSub = new char[13];
	LPTSTR lpszData = new char[26];
	LPTSTR lpszName = new char[13];

	LONG lReturn=0, lRe=0;
	DWORD dwIndex=0, dwInx=0, dwSub=13, dwData=26, dwName=13;
	CString strReturn="", strID="", strName="";

	if (RegOpenKeyEx(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, 
		0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
	{
		do 
		{
			lReturn = RegEnumKeyEx(hKey, dwIndex, lpszSub, &dwSub, NULL, NULL, NULL, NULL);
			memcpy(strReturn.GetBuffer(dwSub), lpszSub, dwSub);
			strReturn.ReleaseBuffer();
			strReturn = m_strKey + "\\" + strReturn;

			if ((lReturn == ERROR_SUCCESS) &&
				(RegOpenKeyEx(HKEY_CURRENT_USER, (LPCTSTR)strReturn, 
				0, KEY_ALL_ACCESS, &hNewKey) == ERROR_SUCCESS))
			{
				do 
				{				
					lRe = RegEnumValue(hNewKey, dwInx, lpszName, &dwName, 0, NULL, (LPBYTE)lpszData, &dwData);

					if (lRe == ERROR_SUCCESS)
					{
						if (lpszName == value.id)
							memcpy(strID.GetBuffer(dwData), lpszData, dwData);
						
						else if (lpszName == value.name)
							memcpy(strName.GetBuffer(dwData), lpszData, dwData);
					}

					dwInx++;
					dwName = 13;
					dwData = 26;
					memset(lpszName, 0x00, dwName);
					memset(lpszData, 0x00, dwData);
					
				} while (!(lRe == ERROR_NO_MORE_ITEMS));
				strID.ReleaseBuffer();
				strName.ReleaseBuffer();

//				if (!(strID=="" && strName==""))
//					SendMessage(m_hWnd, UM_REG_LIST, (WPARAM)&strID, (LPARAM)&strName);

				strID.Empty();
				strName.Empty();
			}
			dwInx=0;
			dwIndex++;
			dwSub = 13;	
			memset(lpszSub, 0x00, dwSub);
			strReturn.Empty();

		} while (!(lReturn == ERROR_NO_MORE_ITEMS));
		
		RegCloseKey(hKey);
	}

	delete lpszSub;
	delete lpszData;
	delete lpszName;
}

/*--------------------------------------------------------------
| LoginCheck : 레지스트리에 등록된 아이디와 패스워드인지 확인 
| argument   : id	- 접속한 클라이언트 아이디
			 : pass - 접속한 클라이언트 패스워드  
+-------------------------------------------------------------*/
BOOL CRegistry::LoginCheck(CString id, CString pass)
{
	BOOL bReturn=FALSE;
	CString strKey="", strPass="";
	HKEY hNewKey;
	LPSTR lpszName = new char[13];
	LPSTR lpszData = new char[51];
	LONG lReturn=0;
	DWORD dwName=13, dwData=51, dwIndex=0;


	if (UserCheck(id))
	{
		strKey = m_strKey + "\\" + id;
		
		if (RegOpenKeyEx(HKEY_CURRENT_USER, (LPCTSTR)strKey,
			0, KEY_ALL_ACCESS, &hNewKey) == ERROR_SUCCESS)
		{
			do 
			{
				memset(lpszName, 0x00, 13);
				lReturn = RegEnumValue(hNewKey, dwIndex, lpszName, &dwName, 0, NULL, (LPBYTE)lpszData, &dwData);
				
				if (lpszName == value.pass)
				{
					memcpy(strPass.GetBuffer(dwData), lpszData, dwData);
					strPass.ReleaseBuffer();
					strPass.TrimLeft();
					strPass.TrimRight();
					strPass.MakeUpper();
					pass.MakeUpper();

					if (strPass == pass)
						bReturn = TRUE;
				}				
				dwName = 13;
				dwData = 51;
				dwIndex++;
			
			} while (!(lReturn == ERROR_NO_MORE_ITEMS));
			RegCloseKey(hNewKey);
			
		}
	}
	delete lpszName;
	delete lpszData;

	return bReturn;	
}