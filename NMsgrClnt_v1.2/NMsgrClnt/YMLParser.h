using namespace cv;

class YMLParser
{
public:
	YMLParser(void);
	~YMLParser(void);

	//path�� ������ �о ��� ������ cv::Mat�� ����
	void ReadYML(char *path);

	Mat GetRMatrix();
	Mat GetTMatrix();
	Mat GetKMatrix();

private:
	Mat CamR_mat;
	Mat CamT_mat;
	Mat CamK_mat;

	//file descriptor�� ���ϴ� �ܾ ������ ������ �̵���Ŵ
	void TransFileDescriptor(FILE *fp, char *word);
	bool FindWordRecursive(FILE *fp, char *word);

	//�� ��Ʈ���� �����͸� �޾ƿ� ��� ������ ����.
	void GetRmat(FILE *fp);
	void GetTmat(FILE *fp);
	void GetKmat(FILE *fp);
	
	//Get & Set Data to matrix
	void GetData(int rows, int cols, char type, Mat *dst, FILE *fp);
};

