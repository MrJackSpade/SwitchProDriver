@echo off
setlocal
set ROOT=%~dp0..
set EXE=%ROOT%\build\svc\switchprosvc.exe
set SAMPLE_INI=%ROOT%\svc\switchprosvc.ini.sample
set CFG_DIR=%ProgramData%\SwitchProSvc
set CFG_INI=%CFG_DIR%\switchprosvc.ini

if not exist "%EXE%" (
  echo switchprosvc.exe not found at: %EXE%
  echo Run scripts\setup.cmd first to build.
  exit /b 1
)

rem Check for elevation and self-elevate if needed.
net session >nul 2>&1
if errorlevel 1 (
  echo Elevating...
  powershell -NoProfile -Command "Start-Process -Verb RunAs -FilePath '%~f0' -Wait"
  exit /b 0
)

echo === Seed config (%CFG_INI%) ===
if not exist "%CFG_DIR%" mkdir "%CFG_DIR%"
if not exist "%CFG_INI%" (
  if exist "%SAMPLE_INI%" (
    copy /Y "%SAMPLE_INI%" "%CFG_INI%" >nul
    echo Seeded default config from sample.
  ) else (
    echo WARNING: sample INI not found at %SAMPLE_INI%; service will run on built-in defaults.
  )
) else (
  echo Config already exists; leaving user edits intact.
)

echo === Rename virtual pad in joy.cpl (per-user, HKCU) ===
reg add "HKCU\System\CurrentControlSet\Control\MediaProperties\PrivateProperties\Joystick\OEM\VID_045E&PID_028E" /v OEMName /t REG_SZ /d "Switch Pro Controller" /f

echo === Install and start service ===
"%EXE%" --install
if errorlevel 1 exit /b 1

echo.
echo Installed. Service will auto-start on boot.
echo Config:  %CFG_INI%
echo Manage with: sc query switchprosvc  /  sc start switchprosvc  /  sc stop switchprosvc
echo Uninstall with: scripts\uninstall_service.cmd
exit /b 0
