@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" > NUL 2>&1
if errorlevel 1 (
  echo vcvars64.bat failed
  exit /b 1
)
msbuild "c:\git\SwitchProDriver\external\ViGEmClient\ViGEmClient.sln" ^
  -p:Configuration=Release_LIB ^
  -p:Platform=x64 ^
  -p:PlatformToolset=v180 ^
  -p:WindowsTargetPlatformVersion=10.0.26100.0 ^
  -v:minimal
exit /b %errorlevel%
