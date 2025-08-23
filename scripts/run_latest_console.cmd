@echo off
setlocal
set EXE=%~dp0..\out\liveops_sentinel.exe
if not exist "%EXE%" (
  echo [ERROR] %EXE% not found. Run build first: scripts\build_console_win.cmd
  exit /b 1
)
echo Running "%EXE%"
"%EXE%"