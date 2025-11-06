@echo off
echo ========================================
echo StegaSaur MVP - Starting Application
echo ========================================
echo.

REM Check if stegasaur binary exists
if not exist stegasaur (
    if not exist stegasaur.exe (
        echo ERROR: stegasaur binary not found!
        echo Please run setup.bat first.
        pause
        exit /b 1
    )
)

REM Check if WSL is available
wsl --status >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: WSL is not available!
    echo Please run setup.bat first.
    pause
    exit /b 1
)

echo Starting Streamlit application...
echo.
echo The application will open in your default browser.
echo Press Ctrl+C to stop the server.
echo.

REM Start Streamlit
python -m streamlit run streamlit_app.py

pause
