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

	//////////////////////////////////////////////////////////////한번에 쓰고 한번에 읽기//////////////////////////////////////////
	void WriteInfo(char *Path, char *filename);								//여태 기록한 정보를 저장. 프로그램 종료 직전 호출
	void ReadInfo(char *file);												//열려있는 파일에서 읽어들임

	void GetBodyVec(std::vector<SkeletonInfo> *dst);						//Body Vector를 dst에 복사
	void SaveBodyInfo(SkeletonInfo src);
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

private:
	std::vector<SkeletonInfo> m_VecBody;									//프로그램 종료시 write할 vector

	char *m_bStr;
	FILE *m_fp;																//file pointer;
};

