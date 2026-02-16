# 🚀 Quick Reference - VS Code Workspace

## Open Workspace
```bash
code CS2-SkinChanger.code-workspace
```

## Start Server
**Task:** `Ctrl+Shift+B` → Full Setup  
**Debug:** `F5` → Launch Web Server  
**Manual:** Terminal → `cd web/backend && node server.js`

## Test Connection
```bash
curl http://localhost:3000/api/health
```

## URLs
- Main: http://localhost:3000/
- Admin: http://localhost:3000/admin.html
- Downloads: http://localhost:3000/downloads.html

## Default Login
- Username: `admin`
- Password: `admin123`

## Key Shortcuts
- Command Palette: `Ctrl+Shift+P`
- Tasks: `Ctrl+Shift+B`
- Terminal: `` Ctrl+` ``
- Debug: `F5`
- Copilot Chat: `Ctrl+Alt+I`

## Troubleshooting
1. Install deps: `cd web/backend && npm install`
2. Check server: `curl localhost:3000/api/health`
3. View logs: Check Terminal panel
4. Restart: Close and reopen workspace

## Need Help?
- See VSCODE_SETUP.md for full guide
- Use Copilot Chat: `Ctrl+Alt+I`
- Check FAQ.md
