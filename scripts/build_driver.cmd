@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" > NUL 2>&1
if errorlevel 1 (
  echo vcvars64.bat failed
  exit /b 1
)

set ROOT=c:\git\SwitchProDriver
set SDK=C:\Program Files (x86)\Windows Kits\10
set SDKVER=10.0.26100.0
set UMDFVER=2.33

set INCSDK=/I"%SDK%\Include\%SDKVER%\um" /I"%SDK%\Include\%SDKVER%\shared" /I"%SDK%\Include\%SDKVER%\winrt"
set INCWDF=/I"%SDK%\Include\wdf\umdf\%UMDFVER%" /I"%SDK%\Include\wdf\kmdf\%UMDFVER%"
set INCVIGEM=/I"%ROOT%\external\ViGEmClient\include"

set LIBSDK=/LIBPATH:"%SDK%\Lib\%SDKVER%\um\x64"
set LIBUCRT=/LIBPATH:"%SDK%\Lib\%SDKVER%\ucrt\x64"
set LIBWDF=/LIBPATH:"%SDK%\Lib\wdf\umdf\x64\%UMDFVER%"
set LIBVIGEM=%ROOT%\external\ViGEmClient\lib\release\x64\ViGEmClient.lib

set OUT=%ROOT%\build\driver
if not exist "%OUT%" mkdir "%OUT%"

set SRCS=%ROOT%\driver\driver.c %ROOT%\driver\device.c %ROOT%\driver\output.c %ROOT%\driver\readloop.c %ROOT%\driver\vigem.c %ROOT%\driver\input.c %ROOT%\driver\mapping.c

set DEFS=/DWIN32 /D_WINDOWS /D_USRDLL /DSWPRO_DRIVER /DUMDF_VERSION_MAJOR=2 /DUMDF_VERSION_MINOR=33 /DNTDDI_VERSION=0x0A000008 /D_WIN32_WINNT=0x0A00 /DWINVER=0x0A00 /DWIN32_LEAN_AND_MEAN /D_AMD64_ /DAMD64 /D_WIN64

cl.exe /nologo /LD /MT /W3 /O2 /Zi %DEFS% %INCWDF% %INCSDK% %INCVIGEM% ^
  /Fo:"%OUT%\\" /Fe:"%OUT%\SwitchProDriver.dll" /Fd:"%OUT%\SwitchProDriver.pdb" ^
  %SRCS% ^
  /link /DLL /SUBSYSTEM:WINDOWS /ENTRY:"_DllMainCRTStartup" ^
  %LIBWDF% %LIBSDK% %LIBUCRT% ^
  WdfDriverStubUm.lib "%LIBVIGEM%" SetupAPI.lib Kernel32.lib User32.lib Advapi32.lib ntdll.lib

if errorlevel 1 exit /b 1
dir "%OUT%\SwitchProDriver.dll"
exit /b 0
