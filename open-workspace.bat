@echo off
REM CS2 Skin Changer - Quick Workspace Opener (Windows)

echo.
echo ====================================
echo   CS2 Skin Changer
echo   Opening Workspace...
echo ====================================
echo.

REM Check if VS Code is installed
where code >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: VS Code 'code' command not found!
    echo.
    echo Please install VS Code or add it to your PATH:
    echo https://code.visualstudio.com/download
    echo.
    pause
    exit /b 1
)

REM Check if workspace file exists
if not exist "CS2-SkinChanger.code-workspace" (
    echo ERROR: Workspace file not found!
    echo.
    echo Expected file: CS2-SkinChanger.code-workspace
    echo Current directory: %CD%
    echo.
    pause
    exit /b 1
)

REM Display version info
if exist "VERSION.txt" (
    echo Version:
    type VERSION.txt
    echo.
)

REM Open workspace in VS Code
echo Opening workspace in VS Code...
code CS2-SkinChanger.code-workspace

echo.
echo Workspace opened successfully!
echo.
echo Next steps:
echo 1. Install recommended extensions when prompted
echo 2. Press Ctrl+Shift+B to run Full Setup
echo 3. Start coding!
echo.
echo For help, see: HOW_TO_OPEN.md
echo.

timeout /t 3 >nul
