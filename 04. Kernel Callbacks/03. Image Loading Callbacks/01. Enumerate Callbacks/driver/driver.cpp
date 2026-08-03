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

#define DRIVER_NAME "Image Load Callback"

 // Function declaration for the image load callback
VOID ImageLoadCallback(
    _In_opt_ PUNICODE_STRING FullImageName,
    _In_ HANDLE ProcessId,
    _In_ PIMAGE_INFO ImageInfo
)
{
    if (FullImageName != NULL) {
        // Log the image name
        DbgPrintEx(0, 0, "[%s] Image Loaded: %wZ\n", DRIVER_NAME, FullImageName);
    }
    else {
        DbgPrintEx(0, 0, "[%s] Image Loaded: Name not available\n", DRIVER_NAME);
    }

    // Log the process ID where the image is loaded
    DbgPrintEx(0, 0, "[%s] Loaded into Process ID: %d\n", DRIVER_NAME, HandleToULong(ProcessId));

    // Log the base address where the image is loaded
    DbgPrintEx(0, 0, "[%s] Image Base Address: %p\n", DRIVER_NAME, ImageInfo->ImageBase);

    // Log the image size
    DbgPrintEx(0, 0, "[%s] Image Size: 0x%X bytes\n", DRIVER_NAME, ImageInfo->ImageSize);

}

// Unload routine for driver
VOID DriverUnload(_In_ PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
    PsRemoveLoadImageNotifyRoutine(ImageLoadCallback);
    DbgPrintEx(0, 0, "[%s] Driver Unloaded\n", DRIVER_NAME);
}

// DriverEntry is called when the driver is loaded
extern "C" NTSTATUS DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    UNREFERENCED_PARAMETER(RegistryPath);

    NTSTATUS status;

    // Register the image load callback
    status = PsSetLoadImageNotifyRoutine(ImageLoadCallback);
    if (!NT_SUCCESS(status)) {
        DbgPrintEx(0, 0, "[%s] Failed to register image load callback: 0x%X\n", DRIVER_NAME, status);
        return status;
    }

    DbgPrintEx(0, 0, "[%s] Driver Loaded\n", DRIVER_NAME);

    // Set the unload routine
    DriverObject->DriverUnload = DriverUnload;

    return STATUS_SUCCESS;
}
