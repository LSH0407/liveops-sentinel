@echo off
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0clean_repo.ps1" %*
