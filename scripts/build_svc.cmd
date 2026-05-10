@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" > NUL 2>&1
if errorlevel 1 (
  echo vcvars64.bat failed
  exit /b 1
)

set ROOT=%~dp0..
set VIGEM_INC=%ROOT%\external\ViGEmClient\include
set VIGEM_LIB=%ROOT%\external\ViGEmClient\lib\release\x64\ViGEmClient.lib
set OUT=%ROOT%\build\svc
if not exist "%OUT%" mkdir "%OUT%"

set SRCS=%ROOT%\svc\main.c %ROOT%\svc\worker.c %ROOT%\svc\hid_io.c %ROOT%\driver\input.c %ROOT%\driver\mapping.c

cl.exe /nologo /W3 /O2 /MT /D_CRT_SECURE_NO_WARNINGS ^
  /I"%VIGEM_INC%" /I"%ROOT%\svc" /I"%ROOT%\driver" ^
  %SRCS% ^
  /Fe:"%OUT%\switchprosvc.exe" /Fo:"%OUT%\\" ^
  /link "%VIGEM_LIB%" SetupAPI.lib hid.lib Advapi32.lib User32.lib
if errorlevel 1 exit /b 1

dir "%OUT%\switchprosvc.exe"
exit /b 0
