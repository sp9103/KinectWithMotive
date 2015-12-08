#include "stdafx.h"
#include "Thread.h"

#include <mutex>
#include <Windows.h>
#include <gl/GLU.h>
#include <GLFW/glfw3.h>

static bool streamOpen = true;

class glBodyRenderer
{
public:
	glBodyRenderer(void);
	~glBodyRenderer(void);

	//Initialize.
	//Create Thread, Initialize variable
	//set number of kinect to rendering.
	void InitializeRenderer(int numKinect, char *WindowName, int xpos, int ypos);
	void DeInitializeRenderer();

	//Check Window close in thread.
	int CheckWindowClose();

	/*Copy & send to thread*/
	//루프 안에서 매 프레임마다 호출해줘야함.
	void SetBodyInfo(SkeletonInfo *srcBodyInfo);

	//Thread가 정상 종료되길 기다림
	void WaitUntilThreadDead();
	void WaitUntilThreadInit();

private:
	static void key( GLFWwindow* window, int k, int s, int action, int mods );
	static void scroll_callback(GLFWwindow* window, double x, double y);

	void draw(SkeletonInfo *JointData, int numKinect);
	void drawFrame(float len);
	void drawFloor();
	void drawSphere(double r, int lats, int longs, GLfloat *pColor);
	void drawBody(BodyInfo tBody);
	void DrawSkelBone(Joint* pJoints, cv::Point3f* pJointPoints, JointType joint0, JointType joint1, GLfloat *t_Color);

	static void reshape( GLFWwindow* window, int width, int height );

	void init();

	void glTransformCoordinate(Joint src, cv::Point3f *dst);

	GLFWwindow* m_window;
	GLfloat angle;

	int m_width, m_height;
	int m_numKinect;
	unsigned m_floorTexID;

	/*input : Skeleton Info Array
	numKinect : number of Kinect.(to Rendering)*/
	void Display(SkeletonInfo *JointData, int numKinect);											//나중에 스켈레톤 받아오기

	/*gl Coordiante Transformation Matrix*/
	cv::Mat TransformMat;

	//For Multi Threading
	CRITICAL_SECTION m_cs;
	Thread m_glThread;

	bool m_EndThread;
	static UINT WINAPI renderThread(LPVOID param); // 쓰레드 함수.

	//Shared Variable. 쓰레드 터지면 이 변수 살펴포기. - 구조체 큐로 변경할까...
	SkeletonInfo *m_BodyInfo;					//스레드에 넘겨줄 변수.

	GLfloat view_rotx, view_roty, view_rotz;
	GLfloat view_tranx, view_trany, view_tranz;
	bool m_EnableThread;
	bool m_InitCheck;

	char WindowName[256];
	cv::Point2d Winpos;
};

