@echo off
setlocal DisableDelayedExpansion

if /I "%~1" NEQ "__keepopen__" (
    start "" cmd /k ""%~f0" __keepopen__"
    exit /b
)

shift
title rechan assets extractor
cd /d "%~dp0"

echo ==========================================
echo         rechan assets extractor
echo ==========================================
echo.

where py >nul 2>&1
if not errorlevel 1 goto found_py

where python >nul 2>&1
if not errorlevel 1 goto found_python

echo ERROR: Python 3 not found.
echo Install Python 3 and enable "Add Python to PATH".
goto end

:found_py
set "PYTHON_EXE=py"
set "PYTHON_ARG=-3"
goto after_python

:found_python
set "PYTHON_EXE=python"
set "PYTHON_ARG="

:after_python
if not exist "%~dp0extract_psx_bin.py" (
    echo ERROR: extract_psx_bin.py not found in:
    echo %~dp0
    goto end
)

echo Drag and drop your PSX .bin file into this window,
echo then press Enter.
echo.

set "BININPUT="
set /p "BININPUT=> "

if not defined BININPUT (
    echo.
    echo No file provided.
    goto end
)

call :process_file %BININPUT%
goto end

:process_file
set "BINFILE=%~1"

if not exist "%BINFILE%" (
    echo.
    echo ERROR: File not found:
    echo "%BINFILE%"
    goto :eof
)

set "BINNAME=%~n1"
set "BINDIR=%~dp1"
set "BINEXT=%~x1"

if /I not "%BINEXT%"==".bin" (
    echo.
    echo ERROR: File must be a .bin
    echo "%BINFILE%"
    goto :eof
)

set "OUTDIR="rechan_assets"

echo.
echo Input : "%BINFILE%"
echo Output: "%OUTDIR%"
echo.
echo Running extraction...
echo.

if defined PYTHON_ARG (
    call "%PYTHON_EXE%" %PYTHON_ARG% "%~dp0extract_psx_bin.py" "%BINFILE%" "%OUTDIR%"
) else (
    call "%PYTHON_EXE%" "%~dp0extract_psx_bin.py" "%BINFILE%" "%OUTDIR%"
)

set "EXITCODE=%ERRORLEVEL%"
echo.

if "%EXITCODE%"=="0" (
    echo Extraction SUCCESS.
) else (
    echo Extraction FAILED with code %EXITCODE%.
)

goto :eof

:end
echo.
echo Press any key to close...
pause >nul
exit /b