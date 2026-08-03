#pragma once
#include <fltKernel.h>

 // Define callbacks for pre/post operations
extern "C" {

    FLT_PREOP_CALLBACK_STATUS PreOperationCallback(
        _Inout_ PFLT_CALLBACK_DATA Data,
        _In_ PCFLT_RELATED_OBJECTS FltObjects,
        _Flt_CompletionContext_Outptr_ PVOID* CompletionContext
    );

    FLT_POSTOP_CALLBACK_STATUS PostOperationCallback(
        _Inout_ PFLT_CALLBACK_DATA Data,
        _In_ PCFLT_RELATED_OBJECTS FltObjects,
        _In_opt_ PVOID CompletionContext,
        _In_ FLT_POST_OPERATION_FLAGS Flags
    );

    // Define the unload routine
    NTSTATUS Unload(_In_ FLT_FILTER_UNLOAD_FLAGS Flags);

}

// Declare the global variable (if needed in other files)
extern PFLT_FILTER g_FilterHandle;



// Global variable to store the minifilter driver object
PFLT_FILTER g_FilterHandle = NULL;

// Minifilter registration structure
CONST FLT_OPERATION_REGISTRATION Callbacks[] = {
    { IRP_MJ_READ, 0, PreOperationCallback, PostOperationCallback },  // Monitor Read
    { IRP_MJ_WRITE, 0, PreOperationCallback, PostOperationCallback }, // Monitor Write
    { IRP_MJ_SET_INFORMATION, 0, PreOperationCallback, PostOperationCallback }, // Monitor Delete/Rename
    { IRP_MJ_OPERATION_END } // End of callback list
};

// Define the filter registration structure properly
CONST FLT_REGISTRATION FilterRegistration = {
    sizeof(FLT_REGISTRATION),        // Size of FLT_REGISTRATION structure
    FLT_REGISTRATION_VERSION,        // Version
    0,                               // Flags
    NULL,                            // Context Registration (optional)
    Callbacks,                       // Operation Registration (array of callbacks)
    Unload,                          // Filter Unload callback
    NULL,                            // Instance Setup callback (optional)
    NULL,                            // Instance Query Teardown callback (optional)
    NULL,                            // Instance Teardown Start callback (optional)
    NULL,                            // Instance Teardown Complete callback (optional)
    NULL,                            // Generate File Name callback (optional)
    NULL,                            // Generate Destination File Name callback (optional)
    NULL                             // Normalize Name Component callback (optional)
};

// Helper function to get file information (file name and size)
void GetFileInfo(
    _In_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects
)
{
    NTSTATUS status;
    PFLT_FILE_NAME_INFORMATION fileNameInfo = NULL;
    FILE_STANDARD_INFORMATION fileInfo = { 0 };

    // Get the file name (file path)
    status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &fileNameInfo);
    if (NT_SUCCESS(status)) {
        status = FltParseFileNameInformation(fileNameInfo);
        if (NT_SUCCESS(status)) {
            DbgPrintEx(0, 0, "[Minifilter] File Operation on: %wZ\n", &fileNameInfo->Name);
        }
        FltReleaseFileNameInformation(fileNameInfo);
    }
    else {
        DbgPrintEx(0, 0, "[Minifilter] Failed to get file name information: 0x%X\n", status);
    }

    // Get the file size
    status = FltQueryInformationFile(FltObjects->Instance, FltObjects->FileObject, &fileInfo, sizeof(fileInfo), FileStandardInformation, NULL);
    if (NT_SUCCESS(status)) {
        DbgPrintEx(0, 0, "[Minifilter] File Size: %llu bytes\n", fileInfo.EndOfFile.QuadPart);
    }
    else {
        DbgPrintEx(0, 0, "[Minifilter] Failed to query file size: 0x%X\n", status);
    }
}

// This is the pre-operation callback for the minifilter. It is invoked BEFORE a file system
extern "C"
FLT_PREOP_CALLBACK_STATUS PreOperationCallback(
    _Inout_ PFLT_CALLBACK_DATA Data,        // - I/O operation data structure (contains operation type, buffer, size, etc.)
    _In_ PCFLT_RELATED_OBJECTS FltObjects,  // - Context (Instance, Volume, FileObject, etc.)
    _Flt_CompletionContext_Outptr_ PVOID* CompletionContext
)
{
    // Suppress compiler warning since CompletionContext is not used
    UNREFERENCED_PARAMETER(CompletionContext);

    // Retrieve the process and thread IDs for logging purposes
    HANDLE processId = PsGetCurrentProcessId();
    HANDLE threadId = PsGetCurrentThreadId();

    // Handle read operation
    if (Data->Iopb->MajorFunction == IRP_MJ_READ) {
        DbgPrintEx(0, 0, "[Minifilter] Read Operation: Process ID: %d, Thread ID: %d\n",
            HandleToULong(processId), HandleToULong(threadId));
        GetFileInfo(Data, FltObjects);  // Get file name and size
    }

    // Handle write operation
    else if (Data->Iopb->MajorFunction == IRP_MJ_WRITE) {
        DbgPrintEx(0, 0, "[Minifilter] Write Operation: Process ID: %d, Thread ID: %d\n",
            HandleToULong(processId), HandleToULong(threadId));
        GetFileInfo(Data, FltObjects);  // Get file name and size
    }

    // Handle file delete or rename (FileDispositionInformation)
    else if (Data->Iopb->MajorFunction == IRP_MJ_SET_INFORMATION) {
        FILE_INFORMATION_CLASS fileInfoClass = Data->Iopb->Parameters.SetFileInformation.FileInformationClass;
        if (fileInfoClass == FileDispositionInformation || fileInfoClass == FileRenameInformation) {
            DbgPrintEx(0, 0, "[Minifilter] Delete/Rename Operation: Process ID: %d, Thread ID: %d\n",
                HandleToULong(processId), HandleToULong(threadId));
            GetFileInfo(Data, FltObjects);  // Get file name and size
        }
    }

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}


// This callback is executed **after** an I/O operation has been completed.
extern "C"
FLT_POSTOP_CALLBACK_STATUS PostOperationCallback(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
)
{
    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);
    UNREFERENCED_PARAMETER(Flags);

    // Post-operation actions can be handled here if needed


    // Tell the Filter Manager we're done processing this I/O operation.
    return FLT_POSTOP_FINISHED_PROCESSING;
}

// This is the unload routine that gets called when the filter driver is being unloaded.
extern "C"
NTSTATUS Unload(_In_ FLT_FILTER_UNLOAD_FLAGS Flags)
{
    // We don't use the unload flags, so suppress compiler warnings.
    UNREFERENCED_PARAMETER(Flags);

    // Unregister the filter from the Filter Manager.
    if (g_FilterHandle != NULL) {
        FltUnregisterFilter(g_FilterHandle);
        DbgPrintEx(0, 0, "[Minifilter] Unloaded Successfully\n");
    }

    // Return success to indicate the driver has unloaded cleanly.
    return STATUS_SUCCESS;
}

// DriverEntry - Entry point for the minifilter driver.
// This is the first function called by the I/O Manager when the driver is loaded.
extern "C"
NTSTATUS DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    UNREFERENCED_PARAMETER(RegistryPath);

    NTSTATUS status;

    // Register the minifilter driver with the Filter Manager.
        // 'FilterRegistration' contains all the necessary callback information.
        // 'g_FilterHandle' is a global handle to the registered filter, used later for unregistring the minifilter.
    status = FltRegisterFilter(DriverObject, &FilterRegistration, &g_FilterHandle);
    if (!NT_SUCCESS(status)) {
        DbgPrintEx(0, 0, "[Minifilter] Failed to Register Filter: 0x%X\n", status);
        return status;
    }

    // Begin filtering I/O operations.
    // This tells the Filter Manager to start calling the minifilter's callback routines.
    status = FltStartFiltering(g_FilterHandle);
    if (!NT_SUCCESS(status)) {
        // If filtering fails to start, clean up by unregistering the filter
        FltUnregisterFilter(g_FilterHandle);
        DbgPrintEx(0, 0, "[Minifilter] Failed to Start Filtering: 0x%X\n", status);
        return status;
    }

    // If we've reached this point, the minifilter driver loaded and initialized successfully
    DbgPrintEx(0, 0, "[Minifilter] Driver Loaded Successfully\n");

    return STATUS_SUCCESS;
}
