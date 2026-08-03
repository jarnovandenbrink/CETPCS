#include <ntddk.h>

// Define the driver name for debugging purposes
#define DRIVER_NAME "SendDriver"

// Define an IOCTL code for sending data from user mode to kernel mode
#define IOCTL_SEND_DATA	CTL_CODE(0x8000, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Function prototypes
VOID UnloadDriver(_In_ PDRIVER_OBJECT DriverObject);
NTSTATUS DriverCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);
NTSTATUS DriverDeviceIoControl(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);

// Entry point for the driver, called when the driver is loaded
extern "C" {
    NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath) {

        // RegistryPath is unused, avoid compiler warnings
        UNREFERENCED_PARAMETER(RegistryPath);

        // Set the driver unload function to be called when the driver is unloaded
        DriverObject->DriverUnload = UnloadDriver;

        // Assign major functions for handling device interactions
        DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverCreateClose;
        DriverObject->MajorFunction[IRP_MJ_CLOSE] = DriverCreateClose;
        DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DriverDeviceIoControl;

        // Define the device name
        UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\SendDriver");

        // Create the device object
        PDEVICE_OBJECT DeviceObject;
        NTSTATUS status = IoCreateDevice(DriverObject, 0, &devName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &DeviceObject);
        if (!NT_SUCCESS(status)) {
            DbgPrintEx(0, 0, "[%s] PCKC Driver: Failed to create device (0x%08X)\n", DRIVER_NAME, status);
            return status;
        }

        // Set device flags to use buffered I/O (data is copied between user and kernel mode buffers)
        DeviceObject->Flags |= DO_BUFFERED_IO;

        // Create a symbolic link for user-mode applications to access the device
        UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\SendDriver");
        status = IoCreateSymbolicLink(&symLink, &devName);
        if (!NT_SUCCESS(status)) {
            DbgPrintEx(0, 0, "[%s] PCKC Driver: Failed to create symbolic link (0x%08X)\n", DRIVER_NAME, status);
            IoDeleteDevice(DeviceObject); // Cleanup device if symbolic link creation fails
            return status;
        }

        // Log that the driver has been loaded successfully
        DbgPrintEx(0, 0, "[%s] Driver loaded!", DRIVER_NAME);

        return STATUS_SUCCESS;
    }
}

// Unload routine, called when the driver is unloaded
VOID UnloadDriver(_In_ PDRIVER_OBJECT DriverObject) {
    UNREFERENCED_PARAMETER(DriverObject);
    DbgPrintEx(0, 0, "[%s] Driver unloaded!\n", DRIVER_NAME);
}

// Handles IRP_MJ_CREATE and IRP_MJ_CLOSE requests from user-mode applications
_Use_decl_annotations_
NTSTATUS DriverCreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);

    // Set IRP status to success
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    // Complete the request and return success
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

// Handles IOCTL requests from user-mode applications
_Use_decl_annotations_
NTSTATUS DriverDeviceIoControl(PDEVICE_OBJECT, PIRP Irp) {
    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION CurrentStackLocation = IoGetCurrentIrpStackLocation(Irp);
    CHAR* data = (CHAR*)Irp->AssociatedIrp.SystemBuffer; // Get the user-mode buffer

    // Process different IOCTL commands
    switch (CurrentStackLocation->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_SEND_DATA:
    {
        // Debug output when IOCTL_SEND_DATA is received
        DbgPrintEx(0, 0, "[%s] Inside IOCTL_SEND_DATA\n", DRIVER_NAME);
        DbgPrintEx(0, 0, "[%s] Data from User-mode process is %s\n", DRIVER_NAME, data);
    }
    break;

    default:
        // Handle unsupported IOCTL requests
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    // Set IRP status and complete the request
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}