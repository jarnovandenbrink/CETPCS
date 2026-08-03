#include <windows.h>
#include <stdio.h>

// Define the IOCTL code that matches the driver's definition
#define IOCTL_SEND_DATA CTL_CODE(0x8000, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS)

int main() {
    // Attempt to open a handle to the driver using its symbolic link
    HANDLE hDevice = CreateFile(
        L"\\\\.\\SendDriver",        // Symbolic link to communicate with the driver
        GENERIC_WRITE,                // Request write access
        FILE_SHARE_WRITE,             // Allow write sharing
        NULL,                         // Default security attributes
        OPEN_EXISTING,                // Open the existing device
        0,                            // No special file attributes
        NULL                          // No template file
    );

    // Check if the handle was successfully obtained
    if (hDevice == INVALID_HANDLE_VALUE) {
        printf("Failed to open the device. Error code: %d\n", GetLastError());
        return 1; // Exit with failure
    }

    // Define the data to be sent to the driver
    char data[] = "Hello World!";
    DWORD bytesReturned; // Variable to store the number of bytes returned

    // Send the data to the driver using the DeviceIoControl function
    BOOL success = DeviceIoControl(
        hDevice,                      // Handle to the device
        IOCTL_SEND_DATA,              // IOCTL code that specifies the operation
        data,                         // Pointer to the input buffer (data to send)
        sizeof(data),                 // Size of the input buffer
        NULL,                         // Output buffer (not required in this case)
        0,                            // Size of the output buffer (0 since not used)
        &bytesReturned,               // Pointer to variable receiving output size (unused here)
        NULL                          // Overlapped structure (not used in synchronous calls)
    );

    // Check if the operation was successful
    if (!success) {
        printf("DeviceIoControl failed. Error code: %d\n", GetLastError());
        CloseHandle(hDevice); // Close device handle before exiting
        return 1; // Exit with failure
    }

    // Confirm successful data transmission
    printf("Data sent successfully: %s\n", data);

    // Close the handle to the device to release resources
    CloseHandle(hDevice);
    return 0; // Exit successfully
}
