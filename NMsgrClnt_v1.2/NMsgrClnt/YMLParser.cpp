#include "stdafx.h"
#include "YMLParser.h"


YMLParser::YMLParser(void)
{
	////matrix initialize
	//CamK_mat = (Mat_<double>(3,3) << 1., 0., 0., 0., 1., 0., 0., 0., 1.);
	//CamR_mat = (Mat_<double>(3,3) << 1., 0., 0., 0., 1., 0., 0., 0., 1.);
	//CamT_mat = (Mat_<double>(3,1) << 0., 0., 0.);
}

YMLParser::~YMLParser(void)
{
	CamK_mat.release();
	CamR_mat.release();
	CamT_mat.release();
}

void YMLParser::ReadYML(char *path){
	FILE *t_fp = NULL;
	char buf[256];

	printf("\nRead yml file start.\n");

	t_fp = fopen(path, "r");

	if(t_fp == NULL){
		printf("File not found error!\n");
		return;
	}

	//file format 확인
	fread(buf, sizeof(char), 5, t_fp);
	if(strncmp(buf, "%YAML", 5) != 0){
		printf("File format is not correct.\n");
		return;
	}

	//파일 형식상 순서 지켜야함.
	GetKmat(t_fp);
	GetRmat(t_fp);
	GetTmat(t_fp);

	printf("File read complete.\n");
	fclose(t_fp);
	return;
}

void YMLParser::TransFileDescriptor(FILE *fp, char *word){
	char buf[256];
	int wordsize = 0;

	strcpy(buf, word);
	wordsize = strlen(buf);

	while(1){
		char c = fgetc(fp);

		if(c == buf[0]){
			if(FindWordRecursive(fp, &buf[1]))
				return;
		}

		if(feof(fp)){
			printf("%s not found in this file.\n", word);
			break;
		}
	}

}

bool YMLParser::FindWordRecursive(FILE *fp, char *word){
	char buf[256];
	int wordsize = 0;

	strcpy(buf, word);
	wordsize = strlen(buf);

	if(wordsize == 0)
		return true;
	else{
		char c = fgetc(fp);

		if(c == buf[0]){
			if(FindWordRecursive(fp, &buf[1]))	return true;
		}
	}

	return false;
}

void YMLParser::GetKmat(FILE *fp){
	int rows, cols;
	char datatype;

	TransFileDescriptor(fp, "depth_intrinsics");
	TransFileDescriptor(fp, "rows: ");
	fscanf(fp, "%d", &rows);
	TransFileDescriptor(fp, "cols: ");
	fscanf(fp, "%d", &cols);
	TransFileDescriptor(fp, "dt: ");
	datatype = fgetc(fp);
	TransFileDescriptor(fp, "data: ");

	//[x, y, z; , ....]으로 이루어진 데이터 parsing
	GetData(rows, cols, datatype, &CamK_mat, fp);
}

void YMLParser::GetRmat(FILE *fp){
	int rows, cols;
	char datatype;

	TransFileDescriptor(fp, "R_extrinsics");
	TransFileDescriptor(fp, "rows: ");
	fscanf(fp, "%d", &rows);
	TransFileDescriptor(fp, "cols: ");
	fscanf(fp, "%d", &cols);
	TransFileDescriptor(fp, "dt: ");
	datatype = fgetc(fp);
	TransFileDescriptor(fp, "data: ");

	//[x, y, z; , ....]으로 이루어진 데이터 parsing
	GetData(rows, cols, datatype, &CamR_mat, fp);
}

void YMLParser::GetTmat(FILE *fp){
	int rows, cols;
	char datatype;

	TransFileDescriptor(fp, "T_extrinsics");
	TransFileDescriptor(fp, "rows: ");
	fscanf(fp, "%d", &rows);
	TransFileDescriptor(fp, "cols: ");
	fscanf(fp, "%d", &cols);
	TransFileDescriptor(fp, "dt: ");
	datatype = fgetc(fp);
	TransFileDescriptor(fp, "data: ");

	//[x, y, z; , ....]으로 이루어진 데이터 parsing
	GetData(rows, cols, datatype, &CamT_mat, fp);
}

void YMLParser::GetData(int rows, int cols, char type, Mat *dst, FILE *fp){
	char buf[256];
	char c = fgetc(fp);
	if(c != '['){
		printf("Format error!\n");
		return;
	}
	c = fgetc(fp);

	switch(type){
	case 'i':
		{
			int temp;

			*dst = (Mat_<int>(rows, cols));

			for(int i = 0; i < rows; i++){
				for(int j = 0; j < cols; j++){
					/*fgets(buf, 256, fp);*/
					fscanf(fp, "%s", buf);
					buf[strlen(buf)] = '\0';
					temp = atoi(buf);

					dst->at<int>(i,j) = temp;
				}
			}
			break;
		}
	case 'd':
		{
			double temp;

			*dst = (Mat_<double>(rows, cols));

			for(int i = 0; i < rows; i++){
				for(int j = 0; j < cols; j++){
					fscanf(fp, "%s", buf);
					buf[strlen(buf)] = '\0';
					temp = atof(buf);

					dst->at<double>(i,j) = temp;
				}
			}

			break;
		}
	case 'f':
		{
			float temp;

			*dst = (Mat_<float>(rows, cols));

			for(int i = 0; i < rows; i++){
				for(int j = 0; j < cols; j++){
					fscanf(fp, "%s", buf);
					buf[strlen(buf)] = '\0';
					temp = atof(buf);

					dst->at<float>(i,j) = temp;
				}
			}
			break;
		}
	default:
		printf("Data type Error!\n");
		break;
	}

	for(int i = 0; i < rows; i++){
		for(int j = 0; j < cols; j++){
			printf("%.3f ",  dst->at<double>(i,j));
		}
		printf("\n");
	}
	printf("\n");
}

Mat YMLParser::GetRMatrix(){
	return CamR_mat.clone();
}

Mat YMLParser::GetKMatrix(){
	return CamK_mat.clone();
}

Mat YMLParser::GetTMatrix(){
	return CamT_mat.clone();
}