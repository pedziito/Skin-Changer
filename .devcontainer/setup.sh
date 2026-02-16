#!/bin/bash

# CS2 Skin Changer - Codespace Setup Script
# This script runs automatically when a Codespace is created

echo ""
echo "🎮 ═══════════════════════════════════════════════════════════════"
echo "   CS2 Skin Changer - Development Environment"
echo "═══════════════════════════════════════════════════════════════ 🎮"
echo ""

# Get version and git info
VERSION=$(cat VERSION.txt 2>/dev/null || echo "1.0.0")
BRANCH=$(git branch --show-current)
COMMIT=$(git rev-parse --short HEAD)
REMOTE_COMMIT=$(git rev-parse --short origin/$BRANCH 2>/dev/null || echo "unknown")

echo "📦 Version: $VERSION"
echo "🌿 Branch: $BRANCH"
echo "📝 Commit: $COMMIT"
echo ""

# Check if we're on the latest commit
if [ "$COMMIT" = "$REMOTE_COMMIT" ]; then
    echo "✅ You have the LATEST version! (up to date with origin/$BRANCH)"
else
    echo "⚠️  Your version: $COMMIT"
    echo "⚠️  Latest version: $REMOTE_COMMIT"
    echo ""
    echo "💡 To update, run: git pull origin $BRANCH"
fi

echo ""
echo "🔧 Setting up development environment..."
echo ""

# Install backend dependencies
if [ -f "web/backend/package.json" ]; then
    echo "📦 Installing backend dependencies..."
    cd web/backend
    npm install --silent
    cd ../..
    echo "✅ Backend dependencies installed"
else
    echo "⚠️  No backend package.json found"
fi

echo ""
echo "🚀 Starting web server..."
echo ""

# Start the server in the background
if [ -f "web/backend/server.js" ]; then
    cd web/backend
    node server.js &
    SERVER_PID=$!
    cd ../..
    
    # Wait a moment for server to start
    sleep 3
    
    # Check if server is running
    if kill -0 $SERVER_PID 2>/dev/null; then
        echo "✅ Web server started successfully (PID: $SERVER_PID)"
        echo ""
        echo "🌐 Server running at: http://localhost:3000"
        echo "👤 Default admin: admin / admin123"
    else
        echo "❌ Failed to start web server"
    fi
else
    echo "⚠️  Server file not found"
fi

echo ""
echo "📚 Quick Start:"
echo "   • Main App:     http://localhost:3000"
echo "   • Admin Panel:  http://localhost:3000/admin.html"
echo "   • Downloads:    http://localhost:3000/downloads.html"
echo ""
echo "💬 GitHub Copilot:"
echo "   • Press Ctrl+Alt+I to chat with Copilot"
echo "   • Ask questions in Danish or English!"
echo ""
echo "📖 Documentation:"
echo "   • HOW_TO_OPEN.md      - Getting started"
echo "   • VSCODE_SETUP.md     - VS Code guide"
echo "   • WORKSPACE_SUMMARY.md - Quick reference"
echo ""
echo "═══════════════════════════════════════════════════════════════"
echo "🎉 Ready to code! Happy hacking! 🚀"
echo "═══════════════════════════════════════════════════════════════"
echo ""

# Open the workspace file automatically
if [ -f "CS2-SkinChanger.code-workspace" ]; then
    code CS2-SkinChanger.code-workspace
fi
