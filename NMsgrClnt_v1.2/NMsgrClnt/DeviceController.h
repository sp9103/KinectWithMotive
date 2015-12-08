//#include "stdafx.h"
#include "KinectConnector.h"

class DeviceController
{
public:
	DeviceController(void);
	~DeviceController(void);

	void Initialize();

private:
	int DeviceConunt;
	HRESULT hr;
};

