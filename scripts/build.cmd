@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" > NUL 2>&1
if errorlevel 1 (
  echo vcvars64.bat failed
  exit /b 1
)
set > "%~dp0..\\.vcenv"
echo --- tools found:
where msbuild
where cl
echo ---
