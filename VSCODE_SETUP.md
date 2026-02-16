# VS Code Workspace Setup Guide

## 🎯 Welcome to CS2 Skin Changer Workspace!

This workspace is configured to provide a complete development environment in VS Code with integrated chat agent (GitHub Copilot) support.

## 📋 Quick Start

### 1. Open the Workspace

**Option A: From VS Code**
```
File → Open Workspace from File → Select "CS2-SkinChanger.code-workspace"
```

**Option B: From Command Line**
```bash
code CS2-SkinChanger.code-workspace
```

### 2. Install Recommended Extensions

When you open the workspace, VS Code will prompt you to install recommended extensions. Click **"Install All"** or install them individually:

- **Essential:**
  - ESLint (code linting)
  - Prettier (code formatting)
  - GitHub Copilot (AI assistant)
  - GitHub Copilot Chat (chat interface)

- **Node.js Development:**
  - npm Intellisense
  - npm Scripts Explorer

- **C++ Development:**
  - C/C++ Extension Pack
  - CMake Tools

- **Utilities:**
  - GitLens (Git supercharged)
  - Live Server (frontend preview)
  - SQLite Viewer (database inspection)

### 3. Start the Development Server

**Method 1: Using Tasks (Recommended)**

Press `Ctrl+Shift+P` (or `Cmd+Shift+P` on Mac) and type:
```
Tasks: Run Task → 📊 Full Setup
```

This will:
1. Install all npm dependencies
2. Start the web server
3. Test the connection

**Method 2: Using Debug**

Press `F5` or go to Debug panel and select:
```
🚀 Launch Web Server
```

**Method 3: Using Terminal**

Open terminal in VS Code and run:
```bash
cd web/backend
npm install
node server.js
```

### 4. Verify Everything Works

Once the server starts, you should see:
```
✅ Database initialized
✅ Server running on http://localhost:3000
```

Test the connection:
- Run task: `🧪 Test Connection`
- Or open: http://localhost:3000

## 🔧 Available Tasks

Access tasks via `Ctrl+Shift+P` → `Tasks: Run Task` or `Ctrl+Shift+B`:

| Task | Description | Shortcut |
|------|-------------|----------|
| 📦 Install Dependencies | Install npm packages | - |
| 🚀 Start Server | Launch web server | - |
| 🧪 Test Connection | Check if server responds | - |
| 🌐 Open in Browser | Open http://localhost:3000 | - |
| 📊 Full Setup | Complete setup sequence | `Ctrl+Shift+B` |

## 🐛 Debugging

### Debug Configurations Available:

1. **🚀 Launch Web Server** - Start server with debugger attached
2. **🔗 Attach to Server** - Connect to running server
3. **🐛 Debug Current File** - Debug any JavaScript file

**How to Debug:**
1. Set breakpoints by clicking left of line numbers
2. Press `F5` or go to Debug panel
3. Select configuration
4. Click Start Debugging

## 💬 GitHub Copilot Chat

If you have GitHub Copilot installed:

### Using Chat:
- Press `Ctrl+Alt+I` to open chat sidebar
- Type your questions or code requests
- Copilot will help with code generation, explanations, and debugging

### Copilot Features in this Workspace:
- ✅ Code completion
- ✅ Chat assistance
- ✅ Code explanations
- ✅ Bug fixing suggestions
- ✅ Test generation

### Example Prompts:
```
"Explain this function"
"Add error handling to this code"
"Generate tests for this API endpoint"
"How do I connect to the database?"
```

## 📁 Workspace Structure

The workspace is organized into 4 folders:

```
🎮 CS2 Skin Changer (Root)    - Main project root
├── 🌐 Web Backend             - Node.js API server
├── 🎨 Web Frontend            - HTML/CSS/JS frontend
└── ⚙️ C++ Client Source       - Desktop client source
```

Each folder has its own explorer view for easy navigation.

## 🔌 Connection Issues

### Problem: "Can't establish connection"

**Solution:**

1. **Check if server is running:**
   ```
   Tasks: Run Task → 🧪 Test Connection
   ```

2. **Install dependencies:**
   ```
   Tasks: Run Task → 📦 Install Dependencies
   ```

3. **Start the server:**
   ```
   Tasks: Run Task → 🚀 Start Server
   ```

4. **Check the port:**
   - Default: http://localhost:3000
   - Make sure no other app is using port 3000

5. **View server logs:**
   - Look in the Terminal panel
   - Check for error messages

### Problem: Dependencies not installing

**Solution:**
```bash
cd web/backend
rm -rf node_modules package-lock.json
npm install
```

### Problem: Server starts but can't connect

**Solution:**
```bash
# Check if firewall is blocking
# Windows: Allow Node.js through Windows Firewall
# Mac: System Preferences → Security → Allow Node

# Test connection
curl http://localhost:3000/api/health
```

## 🎨 Customization

### Change Editor Theme:
```
Ctrl+K Ctrl+T → Select theme
```

Recommended: GitHub Dark Theme (included in workspace)

### Change Font Size:
```
Settings → Editor: Font Size
```

### Configure Prettier:
Edit `.vscode/settings.json`:
```json
{
  "prettier.singleQuote": true,
  "prettier.semi": true
}
```

## 📊 Useful Shortcuts

| Action | Windows/Linux | Mac |
|--------|--------------|-----|
| Command Palette | `Ctrl+Shift+P` | `Cmd+Shift+P` |
| Quick Open File | `Ctrl+P` | `Cmd+P` |
| Toggle Terminal | `Ctrl+` ` | `Cmd+` ` |
| Run Task | `Ctrl+Shift+B` | `Cmd+Shift+B` |
| Start Debugging | `F5` | `F5` |
| Search in Files | `Ctrl+Shift+F` | `Cmd+Shift+F` |
| Git Panel | `Ctrl+Shift+G` | `Cmd+Shift+G` |
| Copilot Chat | `Ctrl+Alt+I` | `Cmd+Alt+I` |

## 🔐 Default Credentials

When accessing the web application:

**Admin Login:**
- Username: `admin`
- Password: `admin123`

⚠️ Change these in production!

## 📚 Additional Resources

### Documentation Files:
- `README.md` - Project overview
- `QUICKSTART.md` - Quick start guide
- `BUILD.md` - Build instructions
- `USAGE.md` - Usage guide
- `FAQ.md` - Frequently asked questions
- `SECURITY.md` - Security information

### Web Application URLs:
- **Main Dashboard:** http://localhost:3000/
- **Admin Panel:** http://localhost:3000/admin.html
- **Downloads:** http://localhost:3000/downloads.html
- **API Health:** http://localhost:3000/api/health

## 🆘 Getting Help

### In VS Code:
1. Use GitHub Copilot Chat (`Ctrl+Alt+I`)
2. Ask questions about code or errors
3. Get real-time assistance

### Documentation:
- Check the FAQ.md file
- Review TROUBLESHOOTING section below

### Community:
- Open an issue on GitHub
- Check existing issues for solutions

## 🐛 Troubleshooting

### Server won't start

**Check Node.js version:**
```bash
node --version  # Should be 18 or higher
```

**Reinstall dependencies:**
```bash
cd web/backend
rm -rf node_modules
npm install
```

### VS Code not recognizing workspace

1. Close VS Code completely
2. Reopen the workspace file:
   ```bash
   code CS2-SkinChanger.code-workspace
   ```

### Extensions not loading

1. Check Extensions panel (`Ctrl+Shift+X`)
2. Search for recommended extensions
3. Install manually if needed

### Copilot not working

1. Verify Copilot subscription is active
2. Sign in to GitHub in VS Code
3. Enable Copilot in settings:
   ```
   Settings → GitHub Copilot → Enable
   ```

### Tasks not showing up

1. Close and reopen workspace
2. Check `.vscode/tasks.json` exists
3. Reload window: `Ctrl+Shift+P` → `Reload Window`

## 🎯 Next Steps

1. ✅ Open workspace
2. ✅ Install extensions
3. ✅ Run "Full Setup" task
4. ✅ Test connection
5. ✅ Open browser to http://localhost:3000
6. 🎉 Start developing!

## 💡 Pro Tips

- Use `Ctrl+Shift+P` frequently for quick access to commands
- Set up keyboard shortcuts for your most-used tasks
- Use the integrated terminal instead of external ones
- Take advantage of Copilot Chat for learning and debugging
- Use Git integration for version control
- Enable auto-save for convenience

## 🔄 Updates

To update the workspace configuration:
1. Pull latest changes from repository
2. Reload VS Code window
3. Accept any extension update prompts

---

**Last Updated:** 2026-02-16

**Workspace Version:** 1.0

**Happy Coding! 🚀**
