@echo off
setlocal
set PS=%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe
"%PS%" -NoProfile -ExecutionPolicy Bypass -File "%~dp0build_console_win.ps1"
exit /b %ERRORLEVEL%
