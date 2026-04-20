@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" > NUL 2>&1
if errorlevel 1 (
  echo vcvars64.bat failed
  exit /b 1
)
set SRC=c:\git\SwitchProDriver\external\ViGEmClient\src
set INC=c:\git\SwitchProDriver\external\ViGEmClient\include
set OUTDIR=c:\git\SwitchProDriver\external\ViGEmClient\lib\release\x64
if not exist "%OUTDIR%" mkdir "%OUTDIR%"
pushd "%SRC%"
cl.exe /nologo /c /MT /O2 /W3 /EHsc /std:c++17 /DWIN32_LEAN_AND_MEAN /D_UNICODE /DUNICODE /I"%INC%" /I"%SRC%" ViGEmClient.cpp /Fo:"%OUTDIR%\ViGEmClient.obj"
if errorlevel 1 (
  popd
  echo cl.exe failed
  exit /b 1
)
lib.exe /nologo /OUT:"%OUTDIR%\ViGEmClient.lib" "%OUTDIR%\ViGEmClient.obj"
popd
if errorlevel 1 (
  echo lib.exe failed
  exit /b 1
)
dir "%OUTDIR%\ViGEmClient.lib"
exit /b 0
