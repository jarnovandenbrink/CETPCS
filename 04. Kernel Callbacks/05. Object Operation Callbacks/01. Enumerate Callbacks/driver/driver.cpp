#include <ntddk.h>

#define DRIVER_NAME "Object Operation Callback"

 // Global variable to store the callback registration handle
PVOID g_ObCallbackHandle = NULL;

// Callback for process and thread pre-operations
OB_PREOP_CALLBACK_STATUS PreOperationCallback(
    _In_ PVOID RegistrationContext,
    _Inout_ POB_PRE_OPERATION_INFORMATION OperationInformation  // Info about the operation being performed
)
{
    UNREFERENCED_PARAMETER(RegistrationContext);

    // Get the source process and thread ID — this is the process initiating the operation
    HANDLE processId = PsGetCurrentProcessId();
    HANDLE threadId = PsGetCurrentThreadId();

    // Check if the operation is on a process object
    if (OperationInformation->ObjectType == *PsProcessType) {

        // Cast the generic object to a PEPROCESS structure (target process)
        PEPROCESS process = (PEPROCESS)OperationInformation->Object;

        // Check if the operation is handle creation (e.g., OpenProcess)
        if (OperationInformation->Operation == OB_OPERATION_HANDLE_CREATE) {

            // Log the operation: target process ID, requested access, source process ID, and thread ID
            DbgPrintEx(0, 0, "[%s] Process Handle Create: Target Process ID: %d, Access: 0x%X, Source Process ID: %d, Thread ID: %d\n",
                DRIVER_NAME,
                HandleToULong(PsGetProcessId(process)),
                OperationInformation->Parameters->CreateHandleInformation.DesiredAccess,
                HandleToULong(processId),
                HandleToULong(threadId));
        }
        // Check if the operation is handle duplication (e.g., DuplicateHandle)
        else if (OperationInformation->Operation == OB_OPERATION_HANDLE_DUPLICATE) {

            // Log the operation: target process ID, requested access, source process ID, and thread ID
            DbgPrintEx(0, 0, "[%s] Process Handle Duplicate: Target Process ID: %d, Access: 0x%X, Source Process ID: %d, Thread ID: %d\n",
                DRIVER_NAME,
                HandleToULong(PsGetProcessId(process)),
                OperationInformation->Parameters->DuplicateHandleInformation.DesiredAccess,
                HandleToULong(processId),
                HandleToULong(threadId));
        }
    }

    // Check if the operation is being performed on a thread object
    else if (OperationInformation->ObjectType == *PsThreadType) {

        // Cast the generic object to a PETHREAD structure (target thread)
        PETHREAD thread = (PETHREAD)OperationInformation->Object;

        // Check if the operation is handle creation (e.g., OpenThread)
        if (OperationInformation->Operation == OB_OPERATION_HANDLE_CREATE) {

            // Log the operation: target process ID, requested access, source process ID, and thread ID
            DbgPrintEx(0, 0, "[%s] Thread Handle Create: Target Thread ID: %d, Access: 0x%X, Source Process ID: %d, Thread ID: %d\n",
                DRIVER_NAME,
                HandleToULong(PsGetThreadId(thread)),
                OperationInformation->Parameters->CreateHandleInformation.DesiredAccess,
                HandleToULong(processId),
                HandleToULong(threadId));
        }
        // Check if the operation is handle duplication (e.g., DuplicateHandle)
        else if (OperationInformation->Operation == OB_OPERATION_HANDLE_DUPLICATE) {

            // Log the operation: target process ID, requested access, source process ID, and thread ID
            DbgPrintEx(0, 0, "[%s] Thread Handle Duplicate: Target Thread ID: %d, Access: 0x%X, Source Process ID: %d, Thread ID: %d\n",
                DRIVER_NAME,
                HandleToULong(PsGetThreadId(thread)),
                OperationInformation->Parameters->DuplicateHandleInformation.DesiredAccess,
                HandleToULong(processId),
                HandleToULong(threadId));
        }
    }
    // Always return OB_PREOP_SUCCESS to allow the operation to proceed
    return OB_PREOP_SUCCESS;
}

// Unregister the callback when the driver unloads
VOID DriverUnload(_In_ PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
    // If the callback handle is valid 
    if (g_ObCallbackHandle != NULL) {
        // Unregister the object callbacks
        ObUnRegisterCallbacks(g_ObCallbackHandle);
        DbgPrintEx(0, 0, "[%s] Callbacks Unregistered\n", DRIVER_NAME);
    }

    DbgPrintEx(0, 0, "[%s] Driver Unloaded\n", DRIVER_NAME);
}

// DriverEntry is called when the driver is loaded
extern "C" NTSTATUS DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    UNREFERENCED_PARAMETER(RegistryPath);

    NTSTATUS status = STATUS_SUCCESS;

    // Declare an array of two OB_OPERATION_REGISTRATION structures to register callbacks for:
    // [0] Process operations
    // [1] Thread operations
    OB_OPERATION_REGISTRATION operationRegistrations[2] = { 0 };

    // --- Configure the first callback for PROCESS operations ---
    operationRegistrations[0].ObjectType = PsProcessType; // Target object type: Process
    operationRegistrations[0].Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE; // Monitor both create and duplicate operations
    // Monitor both handle creation (e.g. OpenProcess) and duplication (e.g. DuplicateHandle)
    // Function called BEFORE the operation occurs
    operationRegistrations[0].PreOperation = PreOperationCallback;
    operationRegistrations[0].PostOperation = NULL; // We're not using a post-operation callback

    // --- Configure the second callback for THREAD operations ---
    operationRegistrations[1].ObjectType = PsThreadType;  // Target object type: Thread
    // Monitor both thread handle creation and duplication
    operationRegistrations[1].Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE; // Monitor both create and duplicate operations
    operationRegistrations[1].PreOperation = PreOperationCallback;  // Use the same pre-operation callback
    operationRegistrations[1].PostOperation = NULL;  // No post-operation callback

    // Declare and configure the OB_CALLBACK_REGISTRATION structure that describes the full callback setup
    OB_CALLBACK_REGISTRATION callbackRegistration = { 0 };
    callbackRegistration.Version = OB_FLT_REGISTRATION_VERSION;  // Must be set to OB_FLT_REGISTRATION_VERSION (currently 0x100)
    callbackRegistration.OperationRegistrationCount = 2;        // We have 2 operation registrations: one for processes, one for threads
    callbackRegistration.OperationRegistration = operationRegistrations; // Pointer to our array of registrations
    callbackRegistration.RegistrationContext = NULL;            // Optional context passed to callbacks; not used here

    // Register the object callbacks with the Object Manager
    // This enables our driver to intercept handle operations for processes and threads
    status = ObRegisterCallbacks(&callbackRegistration, &g_ObCallbackHandle);   // g_ObCallbackHandle is a global variable used later to unregister the registered object callbacks
    if (!NT_SUCCESS(status)) {
        // Registration failed
        DbgPrintEx(0, 0, "[%s] Failed to register callbacks: 0x%X\n", DRIVER_NAME, status);
        return status;
    }
    // Registration successful
    DbgPrintEx(0, 0, "[%s] Callbacks Registered\n", DRIVER_NAME);

    // Set the unload routine
    DriverObject->DriverUnload = DriverUnload;

    return STATUS_SUCCESS;
}
