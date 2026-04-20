#include "worker.h"

#include <windows.h>
#include <stdio.h>
#include <string.h>

#define SVC_NAME_W   L"SwitchProSvc"
#define SVC_DISPLAY  L"Switch Pro Controller XInput Service"

// ---------- shared state ----------

static volatile LONG g_StopFlag = 0;
static SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
static SERVICE_STATUS g_Status = { 0 };

// ---------- status callback (console + svc logging) ----------

static void ConsoleStatus(const char* msg, void* ctx)
{
    (void)ctx;
    fprintf(stderr, "[svc] %s\n", msg);
}

static void ReportSvcStatus(DWORD state, DWORD exitCode, DWORD waitHint)
{
    g_Status.dwCurrentState = state;
    g_Status.dwWin32ExitCode = exitCode;
    g_Status.dwWaitHint = waitHint;
    g_Status.dwControlsAccepted = (state == SERVICE_START_PENDING)
        ? 0 : SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    g_Status.dwCheckPoint = (state == SERVICE_RUNNING || state == SERVICE_STOPPED)
        ? 0 : ++g_Status.dwCheckPoint;
    if (g_StatusHandle) SetServiceStatus(g_StatusHandle, &g_Status);
}

// ---------- SCM handlers ----------

static DWORD WINAPI SvcCtrlHandler(DWORD ctl, DWORD evType, LPVOID evData, LPVOID ctx)
{
    (void)evType; (void)evData; (void)ctx;
    switch (ctl) {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 3000);
        InterlockedExchange(&g_StopFlag, 1);
        return NO_ERROR;
    case SERVICE_CONTROL_INTERROGATE:
        return NO_ERROR;
    }
    return ERROR_CALL_NOT_IMPLEMENTED;
}

static void WINAPI ServiceMain(DWORD argc, LPWSTR* argv)
{
    (void)argc; (void)argv;
    g_StatusHandle = RegisterServiceCtrlHandlerExW(SVC_NAME_W, SvcCtrlHandler, NULL);
    if (!g_StatusHandle) return;

    g_Status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);
    ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);

    // Note: service-mode status messages go nowhere visible right now.
    // (Future: write to Application event log via ReportEventW.)
    WorkerRun(&g_StopFlag, NULL, NULL);

    ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
}

// ---------- install / uninstall ----------

static int InstallService(void)
{
    wchar_t exePath[MAX_PATH];
    if (!GetModuleFileNameW(NULL, exePath, MAX_PATH)) {
        fprintf(stderr, "GetModuleFileName failed\n");
        return 1;
    }

    SC_HANDLE scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (!scm) {
        fprintf(stderr, "OpenSCManager failed (run as admin?): %lu\n", GetLastError());
        return 2;
    }

    SC_HANDLE svc = CreateServiceW(
        scm, SVC_NAME_W, SVC_DISPLAY,
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START,
        SERVICE_ERROR_NORMAL,
        exePath,
        NULL, NULL, NULL, NULL, NULL);

    if (!svc) {
        DWORD e = GetLastError();
        CloseServiceHandle(scm);
        if (e == ERROR_SERVICE_EXISTS) {
            fprintf(stderr, "Service already installed.\n");
            return 0;
        }
        fprintf(stderr, "CreateService failed: %lu\n", e);
        return 3;
    }

    // Set description.
    SERVICE_DESCRIPTIONW desc = { L"Translates Nintendo Switch Pro Controller HID input into a virtual XInput (Xbox 360) pad via ViGEmBus." };
    ChangeServiceConfig2W(svc, SERVICE_CONFIG_DESCRIPTION, &desc);

    // Kick it off now as well.
    if (!StartServiceW(svc, 0, NULL)) {
        DWORD e = GetLastError();
        if (e != ERROR_SERVICE_ALREADY_RUNNING) {
            fprintf(stderr, "Installed but StartService failed: %lu\n", e);
        }
    }

    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    fprintf(stderr, "Installed and started %ls.\n", SVC_NAME_W);
    return 0;
}

static int UninstallService(void)
{
    SC_HANDLE scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scm) {
        fprintf(stderr, "OpenSCManager failed: %lu\n", GetLastError());
        return 2;
    }
    SC_HANDLE svc = OpenServiceW(scm, SVC_NAME_W, SERVICE_ALL_ACCESS);
    if (!svc) {
        DWORD e = GetLastError();
        CloseServiceHandle(scm);
        if (e == ERROR_SERVICE_DOES_NOT_EXIST) {
            fprintf(stderr, "Service not installed.\n");
            return 0;
        }
        fprintf(stderr, "OpenService failed: %lu\n", e);
        return 3;
    }

    SERVICE_STATUS st;
    ControlService(svc, SERVICE_CONTROL_STOP, &st);
    // Give it a moment to stop; don't block forever.
    for (int i = 0; i < 10; ++i) {
        if (QueryServiceStatus(svc, &st) && st.dwCurrentState == SERVICE_STOPPED) break;
        Sleep(300);
    }

    BOOL ok = DeleteService(svc);
    CloseServiceHandle(svc);
    CloseServiceHandle(scm);

    if (!ok) {
        fprintf(stderr, "DeleteService failed: %lu\n", GetLastError());
        return 4;
    }
    fprintf(stderr, "Uninstalled %ls.\n", SVC_NAME_W);
    return 0;
}

// ---------- console mode ----------

static BOOL WINAPI ConsoleCtrlHandler(DWORD ctrl)
{
    switch (ctrl) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
        fprintf(stderr, "\n[svc] Stop requested.\n");
        InterlockedExchange(&g_StopFlag, 1);
        return TRUE;
    }
    return FALSE;
}

static int RunConsole(void)
{
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
    fprintf(stderr, "Running in console mode. Ctrl-C to stop.\n");
    return WorkerRun(&g_StopFlag, ConsoleStatus, NULL);
}

// ---------- entry ----------

static void PrintUsage(void)
{
    fprintf(stderr,
        "Usage:\n"
        "  switchprosvc --console       Run in the foreground with status output.\n"
        "  switchprosvc --install       Install as a Windows Service (auto-start).\n"
        "  switchprosvc --uninstall     Stop and remove the Windows Service.\n"
        "  switchprosvc                 (invoked by SCM) — run as a service.\n");
}

int main(int argc, char** argv)
{
    if (argc > 1) {
        if (_stricmp(argv[1], "--console") == 0)  return RunConsole();
        if (_stricmp(argv[1], "-c") == 0)         return RunConsole();
        if (_stricmp(argv[1], "--install") == 0)  return InstallService();
        if (_stricmp(argv[1], "--uninstall") == 0) return UninstallService();
        if (_stricmp(argv[1], "--help") == 0 || _stricmp(argv[1], "-h") == 0) {
            PrintUsage();
            return 0;
        }
    }

    SERVICE_TABLE_ENTRYW entries[] = {
        { (LPWSTR)SVC_NAME_W, ServiceMain },
        { NULL, NULL }
    };
    if (!StartServiceCtrlDispatcherW(entries)) {
        // Not invoked by SCM; probably run from a shell without args.
        fprintf(stderr, "Not launched by Service Control Manager. Pass --console, --install, or --uninstall.\n");
        PrintUsage();
        return 1;
    }
    return 0;
}
