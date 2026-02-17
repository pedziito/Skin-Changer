# 🚀 CS2 Skin Changer - Setup Guide

## Quick Start

### 1. Download Loader
- Go to [GitHub Actions](https://github.com/pedziito/Skin-Changer/actions/workflows/build-loader.yml)
- Click the latest "Build CS2 Loader" run
- Download **CS2Changer-Complete** artifact
- Extract all files to a folder

### 2. Generate License (Admin)
- Open `admin-portal/index.html` in your browser
- Login with password: **`Mao770609`**
- Click "Generate License"
- Enter username and validity days
- Copy the generated license key
- Save as `license.key` in the same folder as `cs2loader.exe`

### 3. Run Loader
- Place `license.key` in same folder as `cs2loader.exe`
- Run `cs2loader.exe`
- Read and accept the ⚠️ **VAC WARNING**
- Loader will wait for CS2 to launch

### 4. Start Game & Use Menu
- Launch Counter-Strike 2 normally
- Loader automatically injects DLL
- Press **INS** key in-game to open menu
- Select weapon → Choose skin → Apply

---

## 📦 Downloaded Files

```
CS2Changer/
├── cs2loader.exe          ← Main application (RUN THIS)
├── CS2Changer.dll         ← Game hook (auto-loaded)
├── cs2admin.exe           ← License management CLI (optional)
└── license.key            ← Place user license here
```

---

## 🔐 Admin Panel Usage

### Access Admin Portal
**Method 1: Local**
- Open `admin-portal/index.html` in browser
- Password: `Mao770609`

**Method 2: Deploy Online**
- Upload `admin-portal/` to your web server
- Access via Web Browser

### Generate New Licenses

1. **Click "Generate License"**
2. **Enter:**
   - Username: `player_name`
   - Validity: Number of days (1-365)
   - Notes: Optional description
3. **Click "Generate License"**
4. **Copy the license content**
5. **Send to user**

### Validate Licenses

1. **Click "Validate License"**
2. **Paste license content**
3. **Click "Validate"**
4. See status: Active/Revoked, Expired check

### Manage Licenses

1. **Click "Manage Licenses"**
2. See all generated licenses
3. Status: ACTIVE / REVOKED
4. Click "Revoke" to disable a license

---

## 📋 License Format

When you generate a license, it looks like:

```
CS2-2026-A1B2C3D4-02-E5F6G7H8
USERNAME=player_name
CREATED=2026-02-17 12:34:56
EXPIRY=2026-03-19 12:34:56
ACTIVE=true
```

**Components:**
- **Key:** Unique identifier (CS2-YEAR-RANDOM-MONTH-RANDOM)
- **USERNAME:** Player's name
- **CREATED:** Generation timestamp
- **EXPIRY:** License expiration date
- **ACTIVE:** true = valid, false = revoked

---

## ⚙️ System Requirements

### For Users
- Windows 10/11 x64
- Valid license.key file
- ~100MB free space
- Administrator privileges

### For Admin
- Modern web browser (Chrome, Firefox, Edge)
- No server required (works offline)

---

## ⚠️ Important Notes

### VAC Warning
```
╔════════════════════════════════════════════════════════════════╗
║                    ⚠️  VAC WARNING  ⚠️                          ║
║                                                                ║
║  This tool WILL trigger VAC and result in permanent ban       ║
║  DO NOT USE ON YOUR MAIN ACCOUNT                              ║
║  Use at your own risk - we are not responsible for bans       ║
╚════════════════════════════════════════════════════════════════╝
```

- Game modifications detected by VAC
- Permanent ban applied when detected
- Cannot be appealed
- Affects all games on account

### License Management
- Keep `license.key` safe
- Don't share licenses publicly
- Expired licenses won't work
- Revoked licenses can't be reactivated
- Data stored locally in browser

---

## 🔧 Troubleshooting

### "DLL not found"
- Ensure `CS2Changer.dll` is in same folder as `cs2loader.exe`
- Check file permissions

### "License file not found"
- Place `license.key` in loader directory
- Generate new license via admin panel
- Check filename spelling

### Menu doesn't appear
- Press INS key (case sensitive)
- Check DLL injection was successful
- Verify license.key is valid

### DLL injection fails
- Run as Administrator
- Disable any antivirus temporarily
- Check Windows version (Need Windows 10+)
- Try starting CS2 first, then loader

### Admin panel won't save licenses
- Enable cookies/localStorage in browser
- Try Firefox if Chrome doesn't work
- Clear browser cache and reload

---

## 📊 Default Credentials

| Component | Username | Password |
|-----------|----------|----------|
| Admin Portal | - | `Mao770609` |
| Sample License | Owner | - |

---

## 🌐 Advanced Setup

### Deploy Admin Portal Online

**Using GitHub Pages:**
1. Fork this repository
2. Enable GitHub Pages on `admin-portal/` folder
3. Access via: `https://yourusername.github.io/Skin-Changer/admin-portal/`

**Using Node.js:**
```bash
cd admin-portal
npx http-server
# Open: http://localhost:8080
```

**Using Python:**
```bash
cd admin-portal
python -m http.server 8000
# Open: http://localhost:8000
```

---

## 📝 License Distribution Workflow

### Step 1: Admin Generates
```
1. Open admin-portal/index.html
2. Login (password: Mao770609)
3. Generate License → Copy Key
```

### Step 2: Send to User
```
Email/Discord the license.key content to player
```

### Step 3: User Setup
```
1. Create license.key file
2. Paste content from email
3. Place in loader folder
4. Run cs2loader.exe
```

### Step 4: In-Game
```
1. Accept VAC warning
2. Wait for DLL injection success
3. Launch CS2 normally
4. Press INS for menu
```

---

## 🛡️ Security Best Practices

1. **Protect Admin Password**
   - Use strong password in production
   - Don't share with users
   - Change if compromised

2. **License Distribution**
   - Don't upload licenses to GitHub
   - Use secure channels (Email, Discord DMs)
   - Track who has what license

3. **Revoke as Needed**
   - Disable licenses when users leave
   - Revoke if license is leaked
   - Keep admin panel updated

4. **Backup Licenses**
   - Export generated licenses
   - Keep list of active users
   - Print/save as PDF

---

## 📞 Support

### Common Issues
- Check `ARCHITECTURE.md` for technical details
- See `README.md` for full documentation
- Review troubleshooting section above

### Reporting Bugs
- Open GitHub issue with details
- Attach error screenshots
- Include Windows version

---

## ✅ Checklist

Before distributing to users:

- [ ] Downloaded latest build from Actions
- [ ] Generated test license via admin panel
- [ ] Tested with valid license.key
- [ ] Verified DLL injection works
- [ ] Menu opens with INS key
- [ ] VAC warning displays properly
- [ ] License revocation tested
- [ ] Documented your admin password

---

**Last Updated:** February 17, 2026  
**Version:** 1.0.0  
**Status:** Ready for Deployment ✅
