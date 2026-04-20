@echo off
setlocal
set ROOT=%~dp0..
set EXE=%ROOT%\build\svc\switchprosvc.exe

net session >nul 2>&1
if errorlevel 1 (
  echo Elevating...
  powershell -NoProfile -Command "Start-Process -Verb RunAs -FilePath '%~f0' -ArgumentList '' -Wait"
  exit /b 0
)

if exist "%EXE%" (
  "%EXE%" --uninstall
) else (
  echo switchprosvc.exe missing; falling back to sc delete.
  sc stop switchprosvc 2>nul
  sc delete switchprosvc
)

rem Remove any orphaned ViGEm Xbox 360 devices that our client left behind.
echo === Sweeping orphaned virtual Xbox 360 pads (ROOT\SYSTEM children) ===
powershell -NoProfile -Command "Get-PnpDevice -PresentOnly | Where-Object { $_.InstanceId -like 'USB\VID_045E&PID_028E*' } | ForEach-Object { $p = $_ | Get-PnpDeviceProperty -KeyName DEVPKEY_Device_Parent -ErrorAction SilentlyContinue; if ($p -and $p.Data -like 'ROOT\SYSTEM*') { Write-Host ('Removing ' + $_.InstanceId); pnputil /remove-device $_.InstanceId } }"

echo === Restore joy.cpl OEMName (optional) ===
reg add "HKCU\System\CurrentControlSet\Control\MediaProperties\PrivateProperties\Joystick\OEM\VID_045E&PID_028E" /v OEMName /t REG_SZ /d "Controller (XBOX 360 For Windows)" /f

echo.
echo Uninstalled.
exit /b 0
