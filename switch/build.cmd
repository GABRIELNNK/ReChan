@echo off
setlocal

set BASH=C:\Program Files\Git\bin\bash.exe
if not exist "%BASH%" (
    echo Git Bash not found at "%BASH%".
    echo Install Git for Windows, or run switch\build.sh directly from your own Git Bash.
    exit /b 1
)

set MODE=%1
if not "%MODE%"=="" goto :run

echo Select build mode:
echo   1) release  (optimized, for normal play)
echo   2) debug    (unoptimized, LOG/assert active)
echo   3) shipping (release + packs shipping\rechan-switch.zip)
set /p CHOICE=Enter choice [1]:
if "%CHOICE%"=="" set CHOICE=1
if "%CHOICE%"=="1" set MODE=release
if "%CHOICE%"=="2" set MODE=debug
if "%CHOICE%"=="3" set MODE=shipping
if "%MODE%"=="" (
    echo Invalid choice: %CHOICE%
    pause
    exit /b 1
)

:run
"%BASH%" -lc "'%~dp0build.sh' %MODE%"
if errorlevel 1 (
    pause
    exit /b 1
)
pause
