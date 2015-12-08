// FTFile.cpp: implementation of the CFTFile class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "FTFile.h"
#include "direct.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CFTFile::CFTFile()
{
	m_bFirst = FALSE;
	m_hFile = NULL;
	m_strName = "";
	m_dwFileSize = 0;
	m_dwSum = 0;
}

CFTFile::~CFTFile()
{
}

/*-----------------------------------------------------------------------
| FindFile : 디렉토리에 존재하는 모든 파일 찾기 
| argument : strDir - 디렉토리 명 
+----------------------------------------------------------------------*/
BOOL CFTFile::FindFile(CString strDir)
{
	_chdir(strDir);
	
	m_file = new CFileFind;
	if(m_file->FindFile("*.*"))
	{
		m_bFirst = TRUE;
		return TRUE;
	}
	else return FALSE;


}

/*-----------------------------------------------------------------------
| FindOneFile : 파일 찾기 
| argument    : strName - 파일 명 
+----------------------------------------------------------------------*/
BOOL CFTFile::FindOneFile(CString strName)
{
	m_file = new CFileFind;
	if(m_file->FindFile(strName))
	{
		m_file->FindNextFile();
		m_bFirst = FALSE;
		return TRUE;
	}
	else return FALSE;
}

/*-----------------------------------------------------------------------
| CreateList : 클라이언트에 보내질 파일 목록 생성하기 
| argument   : pData - FileList 구조체의 내용이 저장될 변수 
+----------------------------------------------------------------------*/
BOOL CFTFile::CreateList(BYTE* pData)
{
	if(pData == NULL) return FALSE;
	
	CTime		FileTime;
	DWORD		dwFileSize=0;
	WORD		wTotal=0, wData=0;
	BYTE*		nAddress=NULL;
	char		filetime[19];
	CString		strName="";
	
	wTotal = HEAD_SIZE + FILE_SIZE + EFCD_SIZE;
	wData = FILE_SIZE;
	strName = m_file->GetFileName();
	dwFileSize = m_file->GetLength();	
	m_file->GetLastWriteTime(FileTime);
	
	sprintf(filetime,"%d-%02d-%02d %2d:%2d", 
		FileTime.GetYear(), FileTime.GetMonth(), FileTime.GetDay(),
		FileTime.GetHour(), FileTime.GetMinute());
	
	memcpy(pData, &wTotal, sizeof(WORD));
	nAddress=pData+sizeof(WORD);	*nAddress = FILE_LIST;
	nAddress++;						if (m_bFirst) *nAddress = START_LIST;
									else *nAddress = ADD_LIST;
	nAddress++;						memcpy(nAddress, &wData, sizeof(WORD));
	nAddress+=sizeof(WORD);			memset(nAddress, 0x00, sizeof(WORD));
	nAddress += sizeof(WORD);		if(m_file->IsDirectory()) *nAddress = FILE_DIR; 
									else  *nAddress = FILE_FILE; 
	nAddress++;						memcpy(nAddress, &dwFileSize, sizeof(DWORD)); 
	nAddress+=sizeof(DWORD);		memcpy(nAddress, strName.GetBuffer(strName.GetLength()), strName.GetLength()); 
	nAddress+=75;					memcpy(nAddress, filetime, sizeof(filetime)-1); 
	nAddress+=sizeof(filetime)-1;	*nAddress = 0xef;
	nAddress++;						*nAddress = 0xcd;
	
	m_bFirst = FALSE;
	return TRUE;
}

/*-----------------------------------------------------------------------
| FindNextFile : 전체 파일 목록 보낼때 다음 파일 찾기 
+----------------------------------------------------------------------*/
BOOL CFTFile::FindNextFile()
{
	BOOL out=FALSE;
	out = m_file->FindNextFile();

	return out;
}

/*-----------------------------------------------------------------------
| IsExist  : 현재 폴더에 같은 이름의 폴더나 파일의 존재 여부  
| argument : strDir - 디렉토리 명, strName - 폴더나 파일 이름  
+----------------------------------------------------------------------*/
BOOL CFTFile::IsExist(CString strDir, CString strName)
{
	_chdir(strDir);

	CFileFind filefind;

	if(filefind.FindFile(strName))
		return TRUE;
	else return FALSE;
}

/*-----------------------------------------------------------------------
| MakeDirectory : 새폴더 생성 
| argument		: strDir - 폴더가 생성될 디렉토리 명
				  strName - 폴더이름 
+----------------------------------------------------------------------*/
BOOL CFTFile::MakeDirectory(CString strDir, CString strName)
{
	CString	strDirname="", strStatus="", strFile="";
	
	strDirname = strDir + "\\" + strName;
	if (CreateDirectory((LPCTSTR)strDirname, NULL))
		return TRUE;
	else return FALSE;
}

/*-----------------------------------------------------------------------
| Delete   : 파일이나 폴더 삭제 
| argument : lst - 파일, 폴더 구별 
			 strDir - 디렉토리 명 
			 strName - 파일이나 폴더 이름  
+----------------------------------------------------------------------*/
BOOL CFTFile::Delete(BYTE lst, CString strDir, CString strName)
{
	if (strDir!="") _chdir(strDir);

	if (lst==REMOVE_FILE)
		DeleteFile((LPCTSTR)strName);

	else 
	{
		CString str = strDir + strName;
		DeleteDir(str);
		RemoveDirectory((LPCTSTR)str);
	}
	
	return TRUE;
}

/*-----------------------------------------------------------------------
| DeleteDir : 파일, 폴더 삭제 (재귀함수)
| argument  : strDir - 디렉토리 명 
+----------------------------------------------------------------------*/
void CFTFile::DeleteDir(CString strDir)
{
	CString strName="", strDirname="";
		
	_chdir(strDir);

	BOOL out;
	CFileFind file;
	file.FindFile("*.*");

	do 
	{
		out = file.FindNextFile();
		strName = file.GetFileName();
		if (file.IsDirectory())
		{	// 지워야 할 파일이 디렉토리라면 
			if ( !(strName==".") && !(strName==".."))
			{
				strDirname = strDir + "\\" + strName;
				DeleteDir(strDirname);
				RemoveDirectory(strDirname);
			}
		}		
		else
			DeleteFile(strName);
		
	} while(out);	

	_chdir(strDir+"\\"+"..");

}

/*-----------------------------------------------------------------------
| FileCreate : 업로드 파일 생성 
| argument	 : strName - 파일 이름  
+----------------------------------------------------------------------*/
BOOL CFTFile::FileCreate(CString strName)
{
	m_hFile = CreateFile(strName, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
					   FILE_ATTRIBUTE_NORMAL, NULL);
	int nError = GetLastError();

	if (nError!=0 && nError!=183) 
		return FALSE;
	else 
	{
		m_strName = strName;
		return TRUE;
	}
}

/*-----------------------------------------------------------------------
| OpenFile : 업로드할 파일의 내용을 읽기 위해 파일 열기 
| argument : strName - 업로드할 파일 명 
+----------------------------------------------------------------------*/
BOOL CFTFile::OpenFile(CString strName)
{
	m_hFile = CreateFile((LPCTSTR)strName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 
				FILE_ATTRIBUTE_NORMAL, NULL);
	int nError = GetLastError();

	m_dwFileSize = m_file->GetLength();
	
	if (nError!=0)
		return FALSE;
	else 
		return TRUE;
}

/*-----------------------------------------------------------------------
| FileWrite : 업로드 파일 내용 파일에 쓰기 
| argument  : SendFile 구조체의 포인터 
+----------------------------------------------------------------------*/
BOOL CFTFile::FileWrite(SendFile *file)
{
	DWORD dwSize=0;
	WORD  wData=0;

	SendFile *send = new SendFile;
	send->diff = file->diff;
	memcpy(send->filesize, file->filesize, sizeof(WORD));
	memcpy(&wData, send->filesize, sizeof(WORD));
	memcpy(send->content, file->content, wData);
	
	int out = WriteFile(m_hFile, send->content, wData, &dwSize, NULL);

	if(send->diff==SEND_LAST)
		CloseHandle(m_hFile);

	delete send;
	
	if(out)
		return TRUE;
	else
	{
		return FALSE;
	}		
}

/*-----------------------------------------------------------------------
| DeleteFindFile : 멤버 변수 m_file 삭제 
+----------------------------------------------------------------------*/
void CFTFile::DeleteFindFile()
{
	if (m_file!=NULL)
	{
		delete m_file;
		m_file = NULL;
	}
	m_dwSum = 0;
	m_strName.Empty();
}

/*-----------------------------------------------------------------------
| FileSend : 업로드 파일 내용 읽기 
| argument : pDt - 서버로 보내질 업로드 내용을 저장하기 위한 바이트 포인터 
+----------------------------------------------------------------------*/
BOOL CFTFile::FileSend(BYTE *pDt)
{
	DWORD		dwSize=0;
	WORD		wFile=0, wTotal=0, wData=0;
	BYTE		byContent[1024], byList=0, *nAddress=NULL;

	wData = SEND_SIZE;
	wTotal = HEAD_SIZE + SEND_SIZE + EFCD_SIZE;
	m_dwSum += sizeof(byContent);

	if (m_dwFileSize < sizeof(byContent))
	{	
		wFile = (WORD)m_dwFileSize;
		BOOL reok = ReadFile(m_hFile, byContent, m_dwFileSize, &dwSize, NULL); 
		if (!reok)
			return FALSE;
		byList = SEND_LAST;
	}
	else
	{
		wFile = sizeof(byContent);
		if (m_dwSum==0) byList = SEND_FIRST;
		else if(m_dwSum>=m_dwFileSize)	
		{
			byList = SEND_LAST;
			wFile = (WORD)m_dwFileSize%1024;
		}
		else byList = SEND_CENTER;
		BOOL reok = ReadFile(m_hFile, byContent, wFile, &dwSize, NULL);
		if (!reok)
			return FALSE;
	}

	memcpy(pDt, &wTotal, sizeof(WORD));
	nAddress=pDt+sizeof(WORD);		*nAddress = DOWN_LOAD;
	nAddress++;						*nAddress = SEND_FILE;
	nAddress++;						memset(nAddress, 0x00, sizeof(WORD));
	nAddress+=sizeof(WORD);			memcpy(nAddress, &wData, sizeof(WORD));
	nAddress+=sizeof(WORD);			*nAddress = byList;
	nAddress++;						memcpy(nAddress, &wFile, sizeof(WORD));
	nAddress+=sizeof(WORD);			memcpy(nAddress, byContent, sizeof(byContent));
	nAddress+=sizeof(byContent);	*nAddress = 0xef;
	nAddress++;						*nAddress = 0xcd;
	
	if (byList == SEND_LAST)
		CloseHandle(m_hFile);
	return TRUE;
}
