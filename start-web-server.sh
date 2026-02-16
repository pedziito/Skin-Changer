#!/bin/bash

# CS2 Skin Changer Web Server Startup Script

echo "🚀 CS2 Skin Changer Web Server"
echo "================================"
echo ""

# Navigate to backend directory
cd "$(dirname "$0")/web/backend"

# Check if node is installed
if ! command -v node &> /dev/null; then
    echo "❌ Node.js is not installed!"
    echo "Please install Node.js from https://nodejs.org/"
    exit 1
fi

echo "✅ Node.js found: $(node --version)"
echo ""

# Check if dependencies are installed
if [ ! -d "node_modules" ]; then
    echo "📦 Installing dependencies..."
    npm install
    echo ""
fi

# Create .env file if it doesn't exist
if [ ! -f ".env" ]; then
    echo "🔧 Creating .env file..."
    cp .env.example .env
    echo ""
fi

# Start the server
echo "🌐 Starting web server..."
echo "   Access the dashboard at: http://localhost:3000"
echo ""
echo "Press Ctrl+C to stop the server"
echo ""

npm start
