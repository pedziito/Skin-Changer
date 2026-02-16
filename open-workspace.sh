#!/bin/bash
# CS2 Skin Changer - Quick Workspace Opener (Unix/Linux/Mac)

echo ""
echo "===================================="
echo "  CS2 Skin Changer"
echo "  Opening Workspace..."
echo "===================================="
echo ""

# Check if VS Code is installed
if ! command -v code &> /dev/null; then
    echo "ERROR: VS Code 'code' command not found!"
    echo ""
    echo "Please install VS Code or add it to your PATH:"
    echo "https://code.visualstudio.com/download"
    echo ""
    exit 1
fi

# Check if workspace file exists
if [ ! -f "CS2-SkinChanger.code-workspace" ]; then
    echo "ERROR: Workspace file not found!"
    echo ""
    echo "Expected file: CS2-SkinChanger.code-workspace"
    echo "Current directory: $(pwd)"
    echo ""
    exit 1
fi

# Display version info
if [ -f "VERSION.txt" ]; then
    echo "Version:"
    cat VERSION.txt
    echo ""
fi

# Open workspace in VS Code
echo "Opening workspace in VS Code..."
code CS2-SkinChanger.code-workspace

echo ""
echo "Workspace opened successfully!"
echo ""
echo "Next steps:"
echo "1. Install recommended extensions when prompted"
echo "2. Press Ctrl+Shift+B to run Full Setup"
echo "3. Start coding!"
echo ""
echo "For help, see: HOW_TO_OPEN.md"
echo ""

sleep 2
