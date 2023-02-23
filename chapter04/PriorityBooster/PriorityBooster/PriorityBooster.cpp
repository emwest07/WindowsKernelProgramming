#include <ntifs.h> // PsLookupThreadByThreadId()
#include <ntddk.h>
#include "PriorityBoosterCommon.h"

// prototypes
void PriorityBoosterUnload(_In_ PDRIVER_OBJECT DriverObject);
NTSTATUS PriorityBoosterCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);
NTSTATUS PriorityBoosterDeviceControl(_In_ PDEVICE_OBJECT, _In_ PIRP Irp);

// Driver Entry
extern "C" NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath); //needed bc compiler warns us that funct does not use its args

	DriverObject->DriverUnload = PriorityBoosterUnload;

	// Need in order to open and close a handle to a device
	DriverObject->MajorFunction[IRP_MJ_CREATE] = PriorityBoosterCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = PriorityBoosterCreateClose;

	// change thread's priority
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = PriorityBoosterDeviceControl;

	UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\PriorityBooster");
	//RtlInitUnicodeString(&devName, L"\\Device\\ThreadBoost");

	// should have pointer to device object after this
	PDEVICE_OBJECT DeviceObject;
	NTSTATUS status = IoCreateDevice(
		DriverObject, // our driver object
		0, // no need for extra bytes
		&devName, // device name
		FILE_DEVICE_UNKNOWN, // device type
		0, // characteristic flags
		FALSE, // not exclusive
		&DeviceObject // the resulting pointer
	);
	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed to create device (0x%08X)\n", status));
		return status;
	}

	// make device object accessible to user-mode callers by providing a symbolic link
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\PriorityBooster");
	status = IoCreateSymbolicLink(&symLink, &devName);
	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed to create symbolic link (0x%08X)\n", status));
		IoDeleteDevice(DeviceObject);
		return status;
	}

	KdPrint(("Sample driver initialized successfully\n"));

	return STATUS_SUCCESS;
}

void PriorityBoosterUnload(_In_ PDRIVER_OBJECT DriverObject)
{
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\PriorityBooster");

	// delete symbolic link
	IoDeleteSymbolicLink(&symLink);

	// delete device object
	IoDeleteDevice(DriverObject->DeviceObject);

	KdPrint(("PriorityBooster unloaded\n"));
}

// Every request to driver is wrapped in an IRP
_Use_decl_annotations_
NTSTATUS PriorityBoosterCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	// Type IO_STATUS_BLOCK
	Irp->IoStatus.Status = STATUS_SUCCESS; // status requests would complete with
	Irp->IoStatus.Information = 0; // polymorphic member

	// actually completes IRP
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS PriorityBoosterDeviceControl(_In_ PDEVICE_OBJECT, _In_ PIRP Irp)
{
	// get our IO_STACK_LOCATION
	auto stack = IoGetCurrentIrpStackLocation(Irp); // IO_STACK_LOCATION*
	auto status = STATUS_SUCCESS;

	switch (stack->Parameters.DeviceIoControl.IoControlCode)
	{
		case IOCTL_PRIORITY_BOOSTER_SET_PRIORITY:
		{
			// is buffer we received large enough to contain a thread object?
			if (stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(ThreadData))
			{
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}

			auto data = (ThreadData*)stack->Parameters.DeviceIoControl.Type3InputBuffer;
			if (data == nullptr)
			{
				status = STATUS_INVALID_PARAMETER;
				break;
			}

			if (data->Priority < 1 || data->Priority > 31)
			{
				status = STATUS_INVALID_PARAMETER;
				break;
			}

			PETHREAD Thread; //pointer to ETHREAD (undocumented)

			// Id typed as handle
			// if successful, fn increments the reference count on the kernel thread object
			status = PsLookupThreadByThreadId(ULongToHandle(data->ThreadId), &Thread);
			if (!NT_SUCCESS(status))
			{
				break;
			}
			
			// make priority change
			KeSetPriorityThread((PKTHREAD)Thread, data->Priority);

			// decrement the thread object's reference, otherwise we create a leak
			ObDereferenceObject(Thread);
			KdPrint(("Thread Priority change for %d to %d succeeded!\n",
				data->ThreadId, data->Priority));
			break;
		}
	default:
		// we don't understand control code
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}