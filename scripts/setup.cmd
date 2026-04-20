@echo off
setlocal
rem One-shot build: ViGEmClient.lib, test binaries, switchprosvc.exe
set ROOT=%~dp0..

echo === ViGEmClient.lib ===
call "%ROOT%\scripts\build_vigemclient_cl.cmd" || exit /b 1

echo === tests ===
call "%ROOT%\scripts\build_tests.cmd" || exit /b 1

echo === service ===
call "%ROOT%\scripts\build_svc.cmd" || exit /b 1

echo.
echo Built:
dir /b "%ROOT%\build\svc\switchprosvc.exe" "%ROOT%\build\test\input_test.exe" "%ROOT%\build\test\vigem_smoke.exe"
echo.
echo Install the service with: scripts\install_service.cmd  (runs elevated)
echo Uninstall with:           scripts\uninstall_service.cmd
exit /b 0
