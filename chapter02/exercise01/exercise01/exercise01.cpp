// Outputs the Windows OS version: major, minor, and build number

#include <ntddk.h>

void SampleUnload(_In_ PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);
	KdPrint(("Sample driver Unload called\n"));
}

// DriverEntry funct must have C linkage (extern "C")
// The _In_ annotations are part of the Source (Code) Annotation Language (SAL). These annotations
// are transparent to the compiler, but provide metadata useful for human readers and static analysis tools
extern "C" NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath); //needed bc compiler warns us that funct does not use its args

	// automatically called before the driver is unloaded from memory
	DriverObject->DriverUnload = SampleUnload;

	RTL_OSVERSIONINFOW osvi;

	if (STATUS_SUCCESS != RtlGetVersion(&osvi))
	{
		KdPrint(("Error calling RtlGetVersion\n"));
		return STATUS_SUCCESS;
	}

	KdPrint(("Windows OS version: %d.%d.%d\n", osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber));

	return STATUS_SUCCESS;
}