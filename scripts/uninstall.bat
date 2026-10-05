@echo off
cd /d "%~dp0"
echo ========================================================
echo   desktop-folder-tags - Desinstallation
echo ========================================================
echo.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0uninstall.ps1"
echo.
pause
