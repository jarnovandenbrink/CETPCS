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

#include <windows.h>
#include <stdio.h>
#include <comdef.h>
#include <Wbemidl.h>

#pragma comment(lib, "wbemuuid.lib")

 // Function to check the status of HVCI (Hypervisor-protected Code Integrity) 
 // and VBS (Virtualization-Based Security)
void CheckHVCIandVBSStatus() {
    HRESULT hres;

    // Initialize COM library for use in multi-threaded applications
    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) {
        printf("Failed to initialize COM library. Error code = 0x%X\n", hres);
        return;
    }

    // Set COM security levels for proper communication
    hres = CoInitializeSecurity(NULL, -1, NULL, NULL,
        RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL, EOAC_NONE, NULL);
    if (FAILED(hres)) {
        printf("Failed to initialize security. Error code = 0x%X\n", hres);
        CoUninitialize();
        return;
    }

    // Obtain the initial WMI locator object
    IWbemLocator* pLoc = NULL;
    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (LPVOID*)&pLoc);
    if (FAILED(hres)) {
        printf("Failed to create IWbemLocator object. Error code = 0x%X\n", hres);
        CoUninitialize();
        return;
    }

    // Connect to the Device Guard namespace in WMI
    IWbemServices* pSvc = NULL;
    hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\Microsoft\\Windows\\DeviceGuard"),
        NULL, NULL, 0, NULL, 0, 0, &pSvc);
    if (FAILED(hres)) {
        printf("Could not connect to WMI namespace. Error code = 0x%X\n", hres);
        pLoc->Release();
        CoUninitialize();
        return;
    }

    // Set security levels for the proxy connection to WMI to ensure proper authentication
    hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
        NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL, EOAC_NONE);
    if (FAILED(hres)) {
        printf("Could not set proxy blanket. Error code = 0x%X\n", hres);
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return;
    }

    // Execute a WQL query to retrieve HVCI and VBS status
    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(bstr_t("WQL"), bstr_t("SELECT SecurityServicesConfigured, VirtualizationBasedSecurityStatus FROM Win32_DeviceGuard"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
    if (FAILED(hres)) {
        printf("Query for HVCI status failed. Error code = 0x%X\n", hres);
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return;
    }

    // Process the results of the query
    IWbemClassObject* pclsObj = NULL;
    ULONG uReturn = 0;
    while (pEnumerator) {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        if (0 == uReturn) {
            break;
        }

        VARIANT vtProp;

        // Extract SecurityServicesConfigured property (HVCI status)
        hr = pclsObj->Get(L"SecurityServicesConfigured", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr) && (vtProp.vt == (VT_ARRAY | VT_I4))) {
            SAFEARRAY* psa = vtProp.parray;
            long lLower, lUpper;
            SafeArrayGetLBound(psa, 1, &lLower);
            SafeArrayGetUBound(psa, 1, &lUpper);
            for (long i = lLower; i <= lUpper; i++) {
                long lValue;
                SafeArrayGetElement(psa, &i, &lValue);
                if (lValue == 2) {
                    printf("Hypervisor-protected Code Integrity (HVCI): Enabled\n");
                }
                else {
                    printf("Hypervisor-protected Code Integrity (HVCI): Disabled\n");
                }
            }
        }
        VariantClear(&vtProp);

        // Extract VirtualizationBasedSecurityStatus property (VBS status)
        hr = pclsObj->Get(L"VirtualizationBasedSecurityStatus", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr) && (vtProp.vt == VT_I4)) {
            if (vtProp.intVal == 2) {
                printf("Virtualization-based Security (VBS): Enabled\n");
            }
            else {
                printf("Virtualization-based Security (VBS): Disabled\n");
            }
        }
        VariantClear(&vtProp);

        // Release the current WMI object
        pclsObj->Release();
    }

    // Cleanup resources
    pSvc->Release();
    pLoc->Release();
    pEnumerator->Release();
    CoUninitialize();
}

// Main function to initiate the HVCI and VBS status check
int main() {
    CheckHVCIandVBSStatus();
    return 0;
}
