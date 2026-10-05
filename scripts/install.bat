@echo off
cd /d "%~dp0"
echo ========================================================
echo   FolderTags - Installation
echo ========================================================
echo.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0install.ps1"
echo.
pause
