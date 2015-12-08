#include "stdafx.h"
#include "DeviceController.h"


DeviceController::DeviceController(void)
{
	DeviceConunt = 0;
}


DeviceController::~DeviceController(void)
{
}

//연결된 키넥트 찾고 각각 커넥터로 연결해줌.
void DeviceController::Initialize(){
	printf("Start system initialize...\n");

	printf("System initialize complete.\n");
}
