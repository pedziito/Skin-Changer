@echo off
REM CS2 Skin Changer Web Server Startup Script for Windows

echo ========================================
echo  CS2 Skin Changer Web Server
echo ========================================
echo.

cd /d "%~dp0web\backend"

REM Check if node is installed
where node >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Node.js is not installed!
    echo Please install Node.js from https://nodejs.org/
    pause
    exit /b 1
)

echo [OK] Node.js found
node --version
echo.

REM Check if dependencies are installed
if not exist "node_modules" (
    echo [INFO] Installing dependencies...
    call npm install
    echo.
)

REM Create .env file if it doesn't exist
if not exist ".env" (
    echo [INFO] Creating .env file...
    copy .env.example .env
    echo.
)

REM Start the server
echo [INFO] Starting web server...
echo.
echo   Access the dashboard at: http://localhost:3000
echo.
echo Press Ctrl+C to stop the server
echo.

call npm start
