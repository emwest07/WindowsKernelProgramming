// Booster.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>
#include "..\PriorityBooster\PriorityBoosterCommon.h"

int Error(const char* message)
{
	printf("%s (error=%d)\n", message, GetLastError());
	return 1;
}

int main(int argc, const char* argv[])
{
	if (argc < 3)
	{
		printf("Usage: Booster <threadid> <priority>\n");
		return 0;
	}


	// handle to device
	// should reach driver in its IRP_MJ_CREATE Dispatch routine
	// if driver is not loaded (no device object and no sym link) - receiver error 2 (file not found)
	HANDLE hDevice = CreateFile(L"\\\\.\\PriorityBooster", GENERIC_WRITE, FILE_SHARE_WRITE,
		nullptr, OPEN_EXISTING, 0, nullptr);

	if (hDevice == INVALID_HANDLE_VALUE)
	{
		return Error("Failed to open device");
	}

	ThreadData data;
	data.ThreadId = atoi(argv[1]); // first cmd arg
	data.Priority = atoi(argv[2]); // second cmd arg

	DWORD returned;
	
	// reaches driver by invoking the IRP_MJ_DEVICE_CONTROL fn routine
	BOOL success = DeviceIoControl(
		hDevice,
		IOCTL_PRIORITY_BOOSTER_SET_PRIORITY, // control code
		&data, // input buffer
		sizeof(data), // input buffer length
		nullptr, // output buffer
		0, // output buffer length
		&returned,
		nullptr
	);

	if (success)
	{
		printf("Priority change succeeded!\n");
	}
	else
	{
		Error("Priority change failed!\n");
	}

	CloseHandle(hDevice);
}
