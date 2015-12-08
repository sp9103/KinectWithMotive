#include "stdafx.h"

#include "RecordBodyInfo.h"

RecordBodyInfo::RecordBodyInfo(void)
{
	m_bStr = (char*)malloc(sizeof(SkeletonInfo)*2);

	m_fp = NULL;
}


RecordBodyInfo::~RecordBodyInfo(void)
{
	free(m_bStr);
}

void RecordBodyInfo::WriteInfo(char *Path, char *filename){
	char buf[256];
	SkeletonInfo t_Body;
	int t_BodyCount = 0;

	if(Path == NULL)	sprintf(buf, "%s/%s.bin", DEFAULT_FILE_PATH, filename);
	else				sprintf(buf, "%s/%s.bin", Path, filename);

	FILE *fp = fopen(buf, "wb");

	t_BodyCount = m_VecBody.size();
	fwrite(&t_BodyCount, sizeof(int), 1, fp);
	for(int i = 0; i < m_VecBody.size(); i++){
		t_Body = m_VecBody.at(i);
		fwrite(&t_Body, sizeof(SkeletonInfo), 1, fp);
	}

	fclose(fp);
}

void RecordBodyInfo::ReadInfo(char *file){
	SkeletonInfo t_Body;
	int FrameCount = 0;
	FILE *fp = fopen(file, "rb");

	if(fp == NULL){
		printf("File not found!\n");
		return;
	}

	fread(&FrameCount, sizeof(int), 1, fp);

	m_VecBody.clear();
	for(int i = 0; i < FrameCount; i++){
		fread(&t_Body, sizeof(SkeletonInfo), 1, fp);
		m_VecBody.push_back(t_Body);
	}

	printf("%d Frame Body information read Complete!\n", m_VecBody.size());

	fclose(fp);
}

//클래스 내부 vector에 현재 프레임 SkeletonInfo 저장
void RecordBodyInfo::SaveBodyInfo(SkeletonInfo src){
	m_VecBody.push_back(src);
}

//클래스 내부 vector를 받아옴
void RecordBodyInfo::GetBodyVec(std::vector<SkeletonInfo> *dst){
	dst->clear();

	for(int i = 0; i < m_VecBody.size(); i++)
		dst->push_back(m_VecBody.at(i));
}

void RecordBodyInfo::OpenFile(char *Path, char *filename, char mode){
	char buf[256];

	if(Path == NULL)	
		sprintf(buf, "%s/%s.bin", DEFAULT_FILE_PATH, filename);
	else				
		sprintf(buf, "%s/%s.bin", Path, filename);

	if(mode == 'r'){
		m_fp = fopen(buf, "rb");
	}
	else if(mode == 'w'){
		m_fp = fopen(buf, "wb");
	}
	else{
		printf("OpenFile mode error!\n");
		return;
	}

	if(m_fp == NULL){
		printf("File not found!\n");
		return;
	}
}

void RecordBodyInfo::CloseFile(){
	fclose(m_fp);
}

void RecordBodyInfo::WriteBodyInfo(SkeletonInfo *src){
	if(m_fp != NULL){
		fwrite(src, sizeof(SkeletonInfo)*NUM_KINECTS, 1, m_fp);
	}
}

void RecordBodyInfo::WriteSyncSkelsAllInfo(synconizedSkeletonsAll *src){
	if(m_fp != NULL){
		fwrite(src, sizeof(synconizedSkeletonsAll), 1, m_fp);
	}
}

bool RecordBodyInfo::ReadBodyInfo(SkeletonInfo *dst){
	if(m_fp != NULL){
		fread(dst, sizeof(SkeletonInfo)*NUM_KINECTS, 1, m_fp);
	}
	else return false;

	if(feof(m_fp)){
		printf("End of File!\n");
		return false;
	}

	return true;
}
