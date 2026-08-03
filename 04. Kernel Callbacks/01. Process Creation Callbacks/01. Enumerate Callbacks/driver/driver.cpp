#include <ntddk.h>
#include <ntstrsafe.h>

#define DRIVER_NAME "Process Creation Callback"

// Function declaration for the callback
VOID ProcessCreateCallback(
    _Inout_ PEPROCESS Process,
    _In_ HANDLE ProcessId,
    _In_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo
)
{
    UNREFERENCED_PARAMETER(Process);
    // Check if CreateInfo is not NULL, indicating that the process is being created (not terminated)
    if (CreateInfo != NULL) {
        DbgPrintEx(0, 0,
            "[%s] Process Created: PID: %d, Parent PID: %d, Image File Name: %wZ\n",
            DRIVER_NAME,
            HandleToULong(ProcessId),
            HandleToULong(CreateInfo->ParentProcessId),
            CreateInfo->ImageFileName
        );

        if (CreateInfo->CommandLine) {
            DbgPrintEx(0, 0,
                "[%s] Command Line: %wZ\n",
                DRIVER_NAME,
                CreateInfo->CommandLine);
        }

        DbgPrintEx(0, 0,
            "[%s] Creating Thread ID: %d\n",
            DRIVER_NAME,
            HandleToULong(CreateInfo->CreatingThreadId.UniqueThread));

        DbgPrintEx(0, 0,
            "[%s] Creating Process ID: %d\n",
            DRIVER_NAME,
            HandleToULong(CreateInfo->CreatingThreadId.UniqueProcess));

        if (CreateInfo->FileObject) {
            DbgPrintEx(0, 0,
                "[%s] Executable File Object: %p\n",
                DRIVER_NAME,
                CreateInfo->FileObject);
        }
    }
    else {
        // Process termination event
        DbgPrintEx(0, 0,
            "[%s] Process Terminated: PID: %d\n",
            DRIVER_NAME,
            HandleToULong(ProcessId));
    }
}

// Unload routine for driver
VOID DriverUnload(_In_ PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
    // Unregister the callback
    PsSetCreateProcessNotifyRoutineEx(ProcessCreateCallback, TRUE);

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

    // Register the process creation callback
    status = PsSetCreateProcessNotifyRoutineEx(ProcessCreateCallback, FALSE);
    if (!NT_SUCCESS(status)) {
        DbgPrintEx(0, 0,
            "[%s] Failed to register process creation callback: 0x%X\n", DRIVER_NAME, status);
        return status;
    }

    DbgPrintEx(0, 0, "[%s] Driver Loaded\n", DRIVER_NAME);

    // Set the unload routine
    DriverObject->DriverUnload = DriverUnload;

    return STATUS_SUCCESS;
}
