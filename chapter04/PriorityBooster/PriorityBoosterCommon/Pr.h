#pragma once

// define control code: Device Type, Function, Method, Access
#define PRIORITY_BOOSTER_DEVICE 0x8000
#define IOCTL_PRIORITY_BOOSTER_SET_PRIORITY CTL_CODE(PRIORITY_BOOSTER_DEVICE, \
	0x800, METHOD_NEITHER, FILE_ANY_ACCESS)

// prototypes
void PriorityBoosterUnload(_In_ PDRIVER_OBJECT DriverObject);
NTSTATUS PriorityBoosterCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);

struct ThreadData {
	ULONG ThreadId; /// 32-bit unsigned ints
	int Priority; // number bwtn 1 and 31
};
