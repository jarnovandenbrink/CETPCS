#include <ntddk.h>
#define DRIVER_NAME "Registry Operation Callback"

LARGE_INTEGER g_RegistryCallbackCookie;

NTSTATUS RegistryCallback(
    _In_ PVOID CallbackContext,
    _In_ PVOID Argument1,
    _In_ PVOID Argument2
)
{
    UNREFERENCED_PARAMETER(CallbackContext);
    REG_NOTIFY_CLASS regNotifyClass = (REG_NOTIFY_CLASS)(ULONG_PTR)Argument1;
    HANDLE processId = PsGetCurrentProcessId();
    HANDLE threadId = PsGetCurrentThreadId();

    switch (regNotifyClass) {
    case RegNtPreOpenKeyEx: {
        PREG_OPEN_KEY_INFORMATION openKeyInfo = (PREG_OPEN_KEY_INFORMATION)Argument2;
        if (openKeyInfo && openKeyInfo->CompleteName) {
            DbgPrintEx(0, 0, "[%s] Pre-Open Key: %wZ, Process ID: %d, Thread ID: %d\n",
                DRIVER_NAME, openKeyInfo->CompleteName, HandleToULong(processId), HandleToULong(threadId));
        }
        break;
    }
    default:
        break;
    }
    return STATUS_SUCCESS;
}

VOID DriverUnload(_In_ PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
    CmUnRegisterCallback(g_RegistryCallbackCookie);
    DbgPrintEx(0, 0, "[%s] Driver Unloaded\n", DRIVER_NAME);
}

extern "C" NTSTATUS DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    UNREFERENCED_PARAMETER(RegistryPath);
    NTSTATUS status = CmRegisterCallback(RegistryCallback, NULL, &g_RegistryCallbackCookie);
    if (!NT_SUCCESS(status)) {
        DbgPrintEx(0, 0, "[%s] Failed to register registry callback: 0x%X\n", DRIVER_NAME, status);
        return status;
    }
    DbgPrintEx(0, 0, "[%s] Driver Loaded\n", DRIVER_NAME);
    DriverObject->DriverUnload = DriverUnload;
    return STATUS_SUCCESS;
}