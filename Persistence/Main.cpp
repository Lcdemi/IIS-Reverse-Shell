<<<<<<< HEAD
#include "Service.h"

int main() {
    SERVICE_TABLE_ENTRYW ServiceTable[] = {
        { (LPWSTR)SERVICE_NAME, ServiceController::ServiceMain },
        { NULL, NULL }
    };

    if (!StartServiceCtrlDispatcherW(ServiceTable)) {
        OutputDebugStringW(L"Failed to start service control dispatcher");
        return GetLastError();
    }
    return 0;
=======
#include "Service.h"

int main() {
    SERVICE_TABLE_ENTRYW ServiceTable[] = {
        { (LPWSTR)SERVICE_NAME, ServiceController::ServiceMain },
        { NULL, NULL }
    };

    if (!StartServiceCtrlDispatcherW(ServiceTable)) {
        OutputDebugStringW(L"Failed to start service control dispatcher");
        return GetLastError();
    }
    return 0;
>>>>>>> f630eceab4685153ce53579793f0422b80df086e
}