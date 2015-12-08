#include <stdlib.h>

//#include "stdafx.h"

//#define DEFAULT_FILE_PATH "C:\\Users\\Pa9301\\Desktop\\Kinect Project\\NMsgrSvr\\NMsgrSvr\\"

#define DEFAULT_FILE_PATH "."


class RecordBodyInfo
{
public:
	RecordBodyInfo(void);
	~RecordBodyInfo(void);

	void OpenFile(char *Path, char *filename, char mode);					//mode 'r' = read, mode 'w' = write	
	void CloseFile();

	void WriteBodyInfo(SkeletonInfo *src);
	void WriteSyncSkelsAllInfo(synconizedSkeletonsAll *src);
	bool ReadBodyInfo(SkeletonInfo *dst);

	//////////////////////////////////////////////////////////////�ѹ��� ���� �ѹ��� �б�//////////////////////////////////////////
	void WriteInfo(char *Path, char *filename);								//���� ����� ������ ����. ���α׷� ���� ���� ȣ��
	void ReadInfo(char *file);												//�����ִ� ���Ͽ��� �о����

	void GetBodyVec(std::vector<SkeletonInfo> *dst);						//Body Vector�� dst�� ����
	void SaveBodyInfo(SkeletonInfo src);
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

private:
	std::vector<SkeletonInfo> m_VecBody;									//���α׷� ����� write�� vector

	char *m_bStr;
	FILE *m_fp;																//file pointer;
};

