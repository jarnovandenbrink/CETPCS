#include <ntddk.h>

#define DRIVER_NAME "Thread Creation Callback"

 // Global function pointer for PsLookupThreadByThreadId
typedef NTSTATUS(*PS_LOOKUP_THREAD_BY_THREAD_ID)(HANDLE ThreadId, PETHREAD* Thread);
PS_LOOKUP_THREAD_BY_THREAD_ID PsLookupThreadByThreadIdFunc = NULL;

// Function to dynamically resolve PsLookupThreadByThreadId
NTSTATUS ResolvePsLookupThreadByThreadId()
{
    UNICODE_STRING routineName;
    RtlInitUnicodeString(&routineName, L"PsLookupThreadByThreadId");

    // Use MmGetSystemRoutineAddress to dynamically resolve the function
    PsLookupThreadByThreadIdFunc = (PS_LOOKUP_THREAD_BY_THREAD_ID)MmGetSystemRoutineAddress(&routineName);

    if (PsLookupThreadByThreadIdFunc == NULL) {
        DbgPrintEx(0, 0, "[%s] Failed to resolve PsLookupThreadByThreadId\n", DRIVER_NAME);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

// Function declaration for the thread callback
VOID ThreadCreateCallback(
    _In_ HANDLE ProcessId,
    _In_ HANDLE ThreadId,
    _In_ BOOLEAN Create
)
{
    PETHREAD thread = NULL;
    NTSTATUS status;

    if (Create) {
        if (PsLookupThreadByThreadIdFunc != NULL) {
            // Thread creation event
            status = PsLookupThreadByThreadIdFunc(ThreadId, &thread);
            if (NT_SUCCESS(status)) {
                DbgPrintEx(0, 0,
                    "[%s] Thread Created: Process ID: %d, Thread ID: %d, ETHREAD Address: %p\n",
                    DRIVER_NAME,
                    HandleToULong(ProcessId),
                    HandleToULong(ThreadId),
                    thread
                );

                // Dereference the thread object when done
                ObDereferenceObject(thread);
            }
            else {
                DbgPrintEx(0, 0,
                    "[%s] Failed to lookup ETHREAD for Thread ID: %d, Status: 0x%X\n",
                    DRIVER_NAME,
                    HandleToULong(ThreadId),
                    status
                );
            }
        }
    }
    else {
        // Thread termination event
        DbgPrintEx(0, 0,
            "[%s] Thread Terminated: Process ID: %d, Thread ID: %d\n",
            DRIVER_NAME,
            HandleToULong(ProcessId),
            HandleToULong(ThreadId)
        );
    }
}

// Unload routine for driver
VOID DriverUnload(_In_ PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
    // Unregister the thread callback
    PsRemoveCreateThreadNotifyRoutine(ThreadCreateCallback);

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

    // Dynamically resolve PsLookupThreadByThreadId
    status = ResolvePsLookupThreadByThreadId();
    if (!NT_SUCCESS(status)) {
        DbgPrintEx(0, 0, "[%s] Could not resolve PsLookupThreadByThreadId\n", DRIVER_NAME);
        return status;
    }

    // Register the thread creation callback
    status = PsSetCreateThreadNotifyRoutine(ThreadCreateCallback);
    if (!NT_SUCCESS(status)) {
        DbgPrintEx(0, 0,
            "[%s] Failed to register thread creation callback: 0x%X\n", DRIVER_NAME, status);
        return status;
    }

    DbgPrintEx(0, 0, "[%s] Driver Loaded\n", DRIVER_NAME);

    // Set the unload routine
    DriverObject->DriverUnload = DriverUnload;

    return STATUS_SUCCESS;
}
