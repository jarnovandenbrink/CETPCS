#include <windows.h>
#include <stdio.h>
#include <comdef.h>
#include <Wbemidl.h>

#pragma comment(lib, "wbemuuid.lib")

#define SystemCodeIntegrityInformation 103
#define CODEINTEGRITY_OPTION_ENABLED   0x01
#define CODEINTEGRITY_OPTION_TESTSIGN  0x02

typedef struct _MY_CODEINTEGRITY_INFORMATION {
    ULONG Length;
    ULONG CodeIntegrityOptions;
} MY_CODEINTEGRITY_INFORMATION;

typedef NTSTATUS(NTAPI* PFN_NtQuerySystemInformation)(
    ULONG, PVOID, ULONG, PULONG);

void CheckDSE() {
    PFN_NtQuerySystemInformation NtQSI =
        (PFN_NtQuerySystemInformation)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQuerySystemInformation");
    if (!NtQSI) return;

    MY_CODEINTEGRITY_INFORMATION ci = { sizeof(MY_CODEINTEGRITY_INFORMATION) };
    if (NtQSI(SystemCodeIntegrityInformation, &ci, sizeof(ci), NULL) != 0) return;

    printf("[+] DSE Enabled:    %s\n", (ci.CodeIntegrityOptions & CODEINTEGRITY_OPTION_ENABLED) ? "Yes" : "No");
    printf("[+] Test Signing:   %s\n", (ci.CodeIntegrityOptions & CODEINTEGRITY_OPTION_TESTSIGN) ? "Yes" : "No");
}

void CheckHVCIandVBS() {
    CoInitializeEx(0, COINIT_MULTITHREADED);
    CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);

    IWbemLocator* pLoc = NULL;
    CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);

    IWbemServices* pSvc = NULL;
    if (FAILED(pLoc->ConnectServer(_bstr_t(L"ROOT\\Microsoft\\Windows\\DeviceGuard"), NULL, NULL, 0, NULL, 0, 0, &pSvc))) {
        pLoc->Release();
        CoUninitialize();
        return;
    }

    CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);

    IEnumWbemClassObject* pEnum = NULL;
    pSvc->ExecQuery(bstr_t("WQL"),
        bstr_t("SELECT SecurityServicesConfigured, SecurityServicesRunning, VirtualizationBasedSecurityStatus FROM Win32_DeviceGuard"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnum);

    IWbemClassObject* pObj = NULL;
    ULONG uReturn = 0;
    pEnum->Next(WBEM_INFINITE, 1, &pObj, &uReturn);
    if (uReturn == 0) return;

    VARIANT v;
    BOOL hvciConfigured = FALSE, hvciRunning = FALSE;

    // Check SecurityServicesConfigured for HVCI (value 2)
    pObj->Get(L"SecurityServicesConfigured", 0, &v, 0, 0);
    if (v.vt == (VT_ARRAY | VT_I4)) {
        SAFEARRAY* psa = v.parray;
        long lo, hi;
        SafeArrayGetLBound(psa, 1, &lo);
        SafeArrayGetUBound(psa, 1, &hi);
        for (long i = lo; i <= hi; i++) {
            long val; SafeArrayGetElement(psa, &i, &val);
            if (val == 2) hvciConfigured = TRUE;
        }
    }
    VariantClear(&v);

    // Check SecurityServicesRunning for HVCI (value 2)
    pObj->Get(L"SecurityServicesRunning", 0, &v, 0, 0);
    if (v.vt == (VT_ARRAY | VT_I4)) {
        SAFEARRAY* psa = v.parray;
        long lo, hi;
        SafeArrayGetLBound(psa, 1, &lo);
        SafeArrayGetUBound(psa, 1, &hi);
        for (long i = lo; i <= hi; i++) {
            long val; SafeArrayGetElement(psa, &i, &val);
            if (val == 2) hvciRunning = TRUE;
        }
    }
    VariantClear(&v);

    // Check VBS status
    int vbsStatus = 0;
    pObj->Get(L"VirtualizationBasedSecurityStatus", 0, &v, 0, 0);
    if (v.vt == VT_I4) vbsStatus = v.intVal;
    VariantClear(&v);

    printf("[+] HVCI Configured: %s\n", hvciConfigured ? "Yes" : "No");
    printf("[+] HVCI Running:    %s\n", hvciRunning ? "Yes" : "No");
    printf("[+] VBS Status:      %s\n", vbsStatus == 2 ? "Running" : vbsStatus == 1 ? "Enabled but not running" : "Not enabled");

    pObj->Release();
    pEnum->Release();
    pSvc->Release();
    pLoc->Release();
    CoUninitialize();
}

int main() {
    CheckDSE();
    CheckHVCIandVBS();
    return 0;
}