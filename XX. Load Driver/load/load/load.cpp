#include <windows.h>
#include <stdio.h>

void LoadDriver(const wchar_t* driverPath, const wchar_t* driverName);

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <full_path_to_driver.sys>\n", argv[0]);
        return 1;
    }

    // Convert path argument to wide string
    int len = MultiByteToWideChar(CP_ACP, 0, argv[1], -1, NULL, 0);
    wchar_t* driverPath = (wchar_t*)malloc(len * sizeof(wchar_t));
    MultiByteToWideChar(CP_ACP, 0, argv[1], -1, driverPath, len);

    // Extract driver name from path (filename without extension)
    wchar_t driverName[256] = { 0 };
    wchar_t* lastSlash = wcsrchr(driverPath, L'\\');
    wchar_t* fileName = lastSlash ? lastSlash + 1 : driverPath;
    wcsncpy(driverName, fileName, wcslen(fileName) - 4); // strip .sys

    LoadDriver(driverPath, driverName);

    free(driverPath);
    return 0;
}

void LoadDriver(const wchar_t* driverPath, const wchar_t* driverName) {
    SC_HANDLE scmHandle = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (scmHandle == NULL) {
        printf("Failed to open Service Control Manager.\n");
        return;
    }

    SC_HANDLE serviceHandle = CreateService(
        scmHandle,
        driverName,
        driverName,
        SERVICE_START | DELETE | SERVICE_STOP,
        SERVICE_KERNEL_DRIVER,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_IGNORE,
        driverPath,
        NULL, NULL, NULL, NULL, NULL
    );

    if (serviceHandle == NULL) {
        if (GetLastError() == ERROR_SERVICE_EXISTS) {
            printf("Service already exists, attempting to start.\n");
            serviceHandle = OpenService(scmHandle, driverName, SERVICE_START);
        }
        else {
            printf("Failed to create service. Error: %lu\n", GetLastError());
            CloseServiceHandle(scmHandle);
            return;
        }
    }

    if (StartService(serviceHandle, 0, NULL)) {
        printf("Driver loaded successfully!\n");
    }
    else {
        printf("Failed to start service. Error: %lu\n", GetLastError());
    }

    CloseServiceHandle(serviceHandle);
    CloseServiceHandle(scmHandle);
}