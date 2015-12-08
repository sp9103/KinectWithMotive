using namespace cv;

class YMLParser
{
public:
	YMLParser(void);
	~YMLParser(void);

	//path에 파일을 읽어서 멤버 변수인 cv::Mat에 저장
	void ReadYML(char *path);

	Mat GetRMatrix();
	Mat GetTMatrix();
	Mat GetKMatrix();

private:
	Mat CamR_mat;
	Mat CamT_mat;
	Mat CamK_mat;

	//file descriptor를 원하는 단어가 나오는 곳까지 이동시킴
	void TransFileDescriptor(FILE *fp, char *word);
	bool FindWordRecursive(FILE *fp, char *word);

	//각 매트릭스 데이터를 받아와 멤버 변수에 저장.
	void GetRmat(FILE *fp);
	void GetTmat(FILE *fp);
	void GetKmat(FILE *fp);
	
	//Get & Set Data to matrix
	void GetData(int rows, int cols, char type, Mat *dst, FILE *fp);
};

