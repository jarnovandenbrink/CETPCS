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

#include <Windows.h>
#include <stdio.h>

// Define the IOCTL code that matches the driver's definition
#define IOCTL_GET_DATA CTL_CODE(0x8000, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS)

int main() {
    // Attempt to open a handle to the driver using its symbolic link
    HANDLE hFile = CreateFileW(
        L"\\\\.\\GetDriver",    // Symbolic link to communicate with the driver
        GENERIC_READ,              // Request read access
        0,                         // No sharing (exclusive access)
        NULL,                      // Default security attributes
        OPEN_EXISTING,             // Open the existing device
        0,                         // No special file attributes
        NULL                       // No template file
    );

    // Check if the handle was successfully obtained
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Failed to open driver. Error code: %lu\n", GetLastError());
        return 1; // Exit with failure
    }

    // Buffer to receive data from the driver
    char buffer[256] = { 0 }; // Allocate a 256-byte buffer, initialized to zero
    DWORD bytesRead; // Variable to store the number of bytes read

    // Read data from the driver
    if (!ReadFile(hFile, buffer, sizeof(buffer), &bytesRead, NULL)) {
        printf("Failed to read from driver. Error code: %lu\n", GetLastError());
        CloseHandle(hFile); // Close device handle before exiting
        return 1; // Exit with failure
    }

    // Print the received data
    printf("Data from driver: %s\n", buffer);

    // Close the handle to the driver to release resources
    CloseHandle(hFile);
    return 0; // Exit successfully
}
