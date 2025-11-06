@echo off
echo ========================================
echo StegaSaur MVP - Windows Setup Script
echo ========================================
echo.

REM Check if WSL is installed
echo [1/6] Checking WSL installation...
wsl --status >nul 2>&1
if %errorlevel% neq 0 (
    echo WSL is not installed. Installing WSL with Ubuntu...
    wsl --install -d Ubuntu
    echo.
    echo WSL has been installed. Please restart your computer and run this script again.
    pause
    exit /b 1
)
echo WSL is installed.
echo.

REM Check if any WSL distribution is available
echo [2/6] Checking WSL distribution...
wsl -l -v >nul 2>&1
if %errorlevel% neq 0 (
    echo No WSL distribution found. Installing Ubuntu...
    wsl --install -d Ubuntu
    echo.
    echo Please complete Ubuntu setup and run this script again.
    pause
    exit /b 1
)

REM Check specifically for Ubuntu (case insensitive)
wsl -l -v | findstr /i "Ubuntu" >nul 2>&1
if %errorlevel% equ 0 (
    echo Ubuntu distribution found and ready.
) else (
    echo Ubuntu not found, but another WSL distribution is available.
    echo Listing available distributions:
    wsl -l -v
    echo.
    echo The setup will continue using the default distribution.
    echo If you prefer Ubuntu specifically, install it with: wsl --install -d Ubuntu
)
echo.

REM Update and install dependencies in WSL
echo [3/6] Installing C++ dependencies in WSL...
wsl bash -c "sudo apt-get update && sudo apt-get install -y build-essential libpng-dev libjpeg-dev zlib1g-dev pkg-config"
if %errorlevel% neq 0 (
    echo Failed to install dependencies. Please check your WSL installation.
    pause
    exit /b 1
)
echo Dependencies installed.
echo.

REM Install Python dependencies on Windows
echo [4/6] Installing Python dependencies...
python -m pip install --upgrade pip
python -m pip install -r requirements.txt
if %errorlevel% neq 0 (
    echo Failed to install Python dependencies.
    echo Please ensure Python is installed and in your PATH.
    pause
    exit /b 1
)
echo Python dependencies installed.
echo.

REM Compile the C++ backend in WSL
echo [5/6] Compiling C++ backend in WSL...
REM Convert current directory to WSL path
set "CURRENT_DIR=%CD%"
set "DRIVE_LETTER=%CURRENT_DIR:~0,1%"
set "WSL_PATH=%CURRENT_DIR:~3%"
set "WSL_PATH=%WSL_PATH:\=/%"
call set "WSL_PATH=/mnt/%%DRIVE_LETTER:~0,1%%/%%WSL_PATH%%"

wsl bash -c "cd '%WSL_PATH%' && make clean && make"
if %errorlevel% neq 0 (
    echo Compilation failed. Using precompiled binary...
    if exist stegasaur.precompiled (
        copy stegasaur.precompiled stegasaur.exe
        echo Using precompiled binary.
    ) else (
        echo ERROR: Compilation failed and no precompiled binary found.
        echo Please check the error messages above.
        pause
        exit /b 1
    )
) else (
    echo Compilation successful.
)
echo.

REM Make stegasaur executable in WSL
wsl bash -c "cd '%WSL_PATH%' && chmod +x stegasaur"

echo [6/6] Setup complete!
echo.
echo ========================================
echo Setup completed successfully!
echo ========================================
echo.
echo To start the application, run: run.bat
echo.
pause
