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
}