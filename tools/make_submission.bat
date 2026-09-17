@echo off
setlocal
rem ------------------------------------------------------------------
rem  Builds the submission package (repo root \ "teishutsu-you" folder).
rem
rem  Just double-click this file. All of the real work - and all of the
rem  Japanese output - lives in make_submission.ps1 next to this batch,
rem  so that this launcher can stay pure ASCII and not depend on the
rem  console code page.
rem
rem  From a shell you can pass the script's own switches through, e.g.
rem      tools\make_submission.bat -Force -Zip
rem ------------------------------------------------------------------

set "SCRIPT=%~dp0make_submission.ps1"

if not exist "%SCRIPT%" (
    echo [ERROR] make_submission.ps1 was not found next to this batch file.
    pause
    exit /b 1
)

rem Prefer PowerShell 7 when it is installed, fall back to Windows PowerShell.
set "PS=powershell"
where pwsh >nul 2>nul && set "PS=pwsh"

"%PS%" -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT%" %*
set "RC=%ERRORLEVEL%"

echo.
if not "%RC%"=="0" echo [ERROR] Failed. Exit code = %RC%
pause
exit /b %RC%
