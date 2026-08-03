/*
 * -----------------------------------------------------------
 * This code is part of the Evasion Lab for the
 * Certified Evasion Techniques Professional (CETP) course
 * by Altered Security.
 *
 * Copyright (c) 2025 Altered Security. All rights reserved.
 *
 * This code is provided solely for educational purposes.
 * Unauthorized use, duplication, or distribution of this
 * code is strictly prohibited without explicit permission
 * from Altered Security.
 * -----------------------------------------------------------
 */

#include <ntddk.h>

 // Define the driver name for debugging purposes
#define DRIVER_NAME "GetDriver"

// Define an IOCTL code for retrieving data from the driver
#define IOCTL_GET_DATA CTL_CODE(0x8000, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Function prototypes
VOID UnloadDriver(_In_ PDRIVER_OBJECT DriverObject);
NTSTATUS DriverCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);
NTSTATUS DriverRead(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);

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
        DriverObject->MajorFunction[IRP_MJ_READ] = DriverRead;

        // Define the device name
        UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\GetDriver");

        // Create the device object
        PDEVICE_OBJECT DeviceObject;
        NTSTATUS status = IoCreateDevice(DriverObject, 0, &devName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &DeviceObject);
        if (!NT_SUCCESS(status)) {
            DbgPrintEx(0, 0, "[%s] Driver: Failed to create device (0x%08X)\n", DRIVER_NAME, status);
            return status;
        }

        // Set device flags to use buffered I/O (data is copied between user and kernel mode buffers)
        DeviceObject->Flags |= DO_BUFFERED_IO;

        // Create a symbolic link for user-mode applications to access the device
        UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\GetDriver");
        status = IoCreateSymbolicLink(&symLink, &devName);
        if (!NT_SUCCESS(status)) {
            DbgPrintEx(0, 0, "[%s] Driver: Failed to create symbolic link (0x%08X)\n", DRIVER_NAME, status);
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
    // Delete the symbolic link
    UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\GetDriver");
    IoDeleteSymbolicLink(&symLink);

    // Delete the device object
    IoDeleteDevice(DriverObject->DeviceObject);

    // Log that the driver has been unloaded successfully
    DbgPrintEx(0, 0, "[%s] Driver unloaded!\n", DRIVER_NAME);
}

// Handles IRP_MJ_CREATE and IRP_MJ_CLOSE requests from user-mode applications
NTSTATUS DriverCreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);

    // Set IRP status to success
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    // Complete the request and return success
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

// Handles IRP_MJ_READ requests to provide data to user-mode applications
NTSTATUS DriverRead(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);

    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION stackLocation = IoGetCurrentIrpStackLocation(Irp);
    ULONG bufferSize = stackLocation->Parameters.Read.Length;

    // Prepare the data to send to the client
    const char* message = "Hello from the driver!";
    size_t messageLength = strlen(message) + 1; // Include the null terminator

    DbgPrintEx(0, 0, "[%s] Check Client Console\n", DRIVER_NAME);

    // Check if the provided buffer is large enough to hold the message
    if (bufferSize < messageLength) {
        status = STATUS_BUFFER_TOO_SMALL;
    }
    else {
        // Copy the message into SystemBuffer to be sent to user-mode
        RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, message, messageLength);
        Irp->IoStatus.Information = messageLength;
    }

    // Set IRP status and complete the request
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}