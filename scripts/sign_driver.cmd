@echo off
setlocal
set ROOT=c:\git\SwitchProDriver
set PKG=%ROOT%\build\driver
set KITS=C:\Program Files (x86)\Windows Kits\10
set SDKVER=10.0.26100.0
set SIGNTOOL="%KITS%\bin\%SDKVER%\x64\signtool.exe"
set INF2CAT="%KITS%\bin\%SDKVER%\x86\Inf2Cat.exe"

for /f "usebackq" %%t in ("%PKG%\cert_thumbprint.txt") do set THUMBPRINT=%%t
if "%THUMBPRINT%"=="" (
  echo No thumbprint found in %PKG%\cert_thumbprint.txt
  exit /b 1
)
echo Using cert thumbprint: %THUMBPRINT%

echo === Sign SwitchProDriver.dll ===
%SIGNTOOL% sign /sm /s My /fd SHA256 /sha1 %THUMBPRINT% /t http://timestamp.digicert.com "%PKG%\SwitchProDriver.dll"
if errorlevel 1 (
  echo signtool sign dll failed
  exit /b 1
)

echo === Build catalog ===
%INF2CAT% /driver:"%PKG%" /os:10_X64
if errorlevel 1 (
  echo inf2cat failed
  exit /b 1
)

echo === Sign catalog ===
%SIGNTOOL% sign /sm /s My /fd SHA256 /sha1 %THUMBPRINT% /t http://timestamp.digicert.com "%PKG%\SwitchProDriver.cat"
if errorlevel 1 (
  echo signtool sign cat failed
  exit /b 1
)

echo === Verify ===
%SIGNTOOL% verify /pa /v "%PKG%\SwitchProDriver.dll" | findstr /I "Verified Successfully"
%SIGNTOOL% verify /pa /v "%PKG%\SwitchProDriver.cat" | findstr /I "Verified Successfully"
dir "%PKG%\SwitchProDriver.*"
exit /b 0
