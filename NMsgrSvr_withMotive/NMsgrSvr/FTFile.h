// FTFile.h: interface for the CFTFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FTFILE_H__B2FEA21D_69E3_4651_BF07_C5D76A152BFF__INCLUDED_)
#define AFX_FTFILE_H__B2FEA21D_69E3_4651_BF07_C5D76A152BFF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CFTFile  
{
// Attribute
protected:
	BOOL		m_bFirst;
	CFileFind*	m_file;
	DWORD		m_dwFileSize;
	DWORD		m_dwSum;

public:
	CString		m_strName;
	HANDLE		m_hFile;
	

// Method
protected:


public:
	BOOL IsExist(CString strDir, CString strName);
	BOOL MakeDirectory(CString strDir, CString strName);
	BOOL Delete(BYTE lst, CString strDir, CString strName);	// Delete file or directory
	void DeleteDir(CString strDir);

	BOOL FindFile(CString strDir);
	BOOL FindOneFile(CString strName);
	BOOL FindNextFile();
	BOOL CreateList(BYTE* pData);
	BOOL FileCreate(CString strName);
	BOOL OpenFile(CString strName);
	BOOL FileWrite(SendFile *file);
	BOOL FileSend(BYTE *pDt);

	void DeleteFindFile();
// Construction
public:
	CFTFile();
	virtual ~CFTFile();

};

#endif // !defined(AFX_FTFILE_H__B2FEA21D_69E3_4651_BF07_C5D76A152BFF__INCLUDED_)
