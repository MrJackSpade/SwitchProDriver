@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" > NUL 2>&1
if errorlevel 1 (
  echo vcvars64.bat failed
  exit /b 1
)
set ROOT=c:\git\SwitchProDriver
set VIGEM_INC=%ROOT%\external\ViGEmClient\include
set VIGEM_LIB=%ROOT%\external\ViGEmClient\lib\release\x64\ViGEmClient.lib
set OUT=%ROOT%\build\test
if not exist "%OUT%" mkdir "%OUT%"

echo === vigem_smoke.exe ===
cl.exe /nologo /W3 /O2 /MT /I"%VIGEM_INC%" ^
  "%ROOT%\test\vigem_smoke.c" ^
  /Fe:"%OUT%\vigem_smoke.exe" /Fo:"%OUT%\\" ^
  /link "%VIGEM_LIB%" SetupAPI.lib
if errorlevel 1 exit /b 1

echo === input_test.exe ===
cl.exe /nologo /W3 /O2 /MT ^
  "%ROOT%\test\input_test.c" ^
  /Fe:"%OUT%\input_test.exe" /Fo:"%OUT%\\"
if errorlevel 1 exit /b 1

dir "%OUT%" | findstr /R /C:"\.exe"
exit /b 0
