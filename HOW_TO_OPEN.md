# 🚀 How to Open / Hvordan Åbner Man Det

[🇬🇧 English](#english) | [🇩🇰 Dansk](#dansk)

---

## English

### Is This the Latest Version?

To check if you have the latest version:

```bash
# Check your current version
cat VERSION.txt

# Check your current commit
git log -1 --oneline

# Check if there are updates
git fetch origin
git status

# Update to latest version
git pull origin main
```

**In Codespaces:** The version is displayed automatically when you create a new Codespace!

---

### 3 Ways to Get Started

#### ⭐ Method 1: GitHub Codespaces (Easiest - No Installation!)

**This is the BEST way to ensure you always have the latest version!**

1. Go to: https://github.com/pedziito/Skin-Changer
2. Click the green **"Code"** button
3. Click **"Codespaces"** tab
4. Click **"Create codespace on main"** (or your branch)

**Or click this badge in the README:**

[![Open in GitHub Codespaces](https://github.com/codespaces/badge.svg)](https://codespaces.new/pedziito/Skin-Changer?quickstart=1)

**What happens automatically:**
- ✅ Latest code is pulled from GitHub
- ✅ VS Code opens in your browser
- ✅ All extensions install
- ✅ Dependencies install
- ✅ Server starts
- ✅ Version information is displayed
- ✅ You're ready to code in 30 seconds!

**Version Check:**
When a Codespace starts, you'll see:
```
📦 Version: 1.0.0
🌿 Branch: main
📝 Commit: a1b2c3d
✅ You have the LATEST version! (up to date with origin/main)
```

#### Method 2: VS Code Desktop (Local)

**Step 1:** Clone or update repository
```bash
# First time - clone
git clone https://github.com/pedziito/Skin-Changer.git
cd Skin-Changer

# Already cloned - update to latest
git pull origin main
```

**Step 2:** Open workspace
```bash
# Windows
open-workspace.bat

# Mac/Linux
./open-workspace.sh

# Or manually
code CS2-SkinChanger.code-workspace
```

**Step 3:** Check version
```bash
cat VERSION.txt
git log -1
```

#### Method 3: From VS Code Menu

1. Open VS Code
2. `File → Open Workspace from File...`
3. Navigate to: `Skin-Changer/CS2-SkinChanger.code-workspace`
4. Click "Open"

---

### Where to Find Everything

```
📁 Skin-Changer/
├── 📄 CS2-SkinChanger.code-workspace  ← Main workspace file
├── 📄 VERSION.txt                     ← Version number
├── 📄 HOW_TO_OPEN.md                  ← This file
├── 📄 open-workspace.bat              ← Quick open (Windows)
├── 📄 open-workspace.sh               ← Quick open (Mac/Linux)
└── 📁 .devcontainer/                  ← Codespaces configuration
    ├── devcontainer.json
    └── setup.sh                       ← Version check script
```

---

### How to Update to Latest Version

#### In Codespaces:
1. Click **"Codespaces"** menu in VS Code
2. Click **"Rebuild Container"**
   - This pulls the latest code and rebuilds
   - OR create a new Codespace (always latest)

#### In Local VS Code:
```bash
# Pull latest changes
git pull origin main

# If there are conflicts
git stash           # Save your changes
git pull origin main
git stash pop       # Restore your changes
```

---

### Troubleshooting

**Q: How do I know if I have the latest version?**
- A: Run `git status` and `git log -1`
- In Codespaces: It's displayed on startup
- Check VERSION.txt

**Q: I want to switch branches**
```bash
git fetch origin
git checkout <branch-name>
git pull origin <branch-name>
```

**Q: Codespace won't start**
- Delete old Codespace and create a new one
- New Codespaces always have the latest code

---

## Dansk

### Er Dette Den Nyeste Version?

For at tjekke om du har den nyeste version:

```bash
# Tjek din nuværende version
cat VERSION.txt

# Tjek dit nuværende commit
git log -1 --oneline

# Tjek om der er opdateringer
git fetch origin
git status

# Opdater til nyeste version
git pull origin main
```

**I Codespaces:** Versionen vises automatisk når du opretter en ny Codespace!

---

### 3 Måder at Komme I Gang

#### ⭐ Metode 1: GitHub Codespaces (Nemmest - Ingen Installation!)

**Dette er den BEDSTE måde at sikre du altid har den nyeste version!**

1. Gå til: https://github.com/pedziito/Skin-Changer
2. Klik på den grønne **"Code"** knap
3. Klik på **"Codespaces"** fanen
4. Klik **"Create codespace on main"** (eller din branch)

**Eller klik på dette badge i README:**

[![Open in GitHub Codespaces](https://github.com/codespaces/badge.svg)](https://codespaces.new/pedziito/Skin-Changer?quickstart=1)

**Hvad sker der automatisk:**
- ✅ Nyeste kode hentes fra GitHub
- ✅ VS Code åbnes i din browser
- ✅ Alle extensions installeres
- ✅ Afhængigheder installeres
- ✅ Server startes
- ✅ Versionsinformation vises
- ✅ Du er klar til at kode på 30 sekunder!

**Versionstjek:**
Når en Codespace starter, ser du:
```
📦 Version: 1.0.0
🌿 Branch: main
📝 Commit: a1b2c3d
✅ Du har den NYESTE version! (up to date med origin/main)
```

#### Metode 2: VS Code Desktop (Lokal)

**Trin 1:** Klon eller opdater repository
```bash
# Første gang - klon
git clone https://github.com/pedziito/Skin-Changer.git
cd Skin-Changer

# Allerede klonet - opdater til nyeste
git pull origin main
```

**Trin 2:** Åbn workspace
```bash
# Windows
open-workspace.bat

# Mac/Linux
./open-workspace.sh

# Eller manuelt
code CS2-SkinChanger.code-workspace
```

**Trin 3:** Tjek version
```bash
cat VERSION.txt
git log -1
```

#### Metode 3: Fra VS Code Menu

1. Åbn VS Code
2. `File → Open Workspace from File...`
3. Naviger til: `Skin-Changer/CS2-SkinChanger.code-workspace`
4. Klik "Open"

---

### Hvor Finder Jeg Alt

```
📁 Skin-Changer/
├── 📄 CS2-SkinChanger.code-workspace  ← Hoved workspace fil
├── 📄 VERSION.txt                     ← Versionsnummer
├── 📄 HOW_TO_OPEN.md                  ← Denne fil
├── 📄 open-workspace.bat              ← Hurtig åbning (Windows)
├── 📄 open-workspace.sh               ← Hurtig åbning (Mac/Linux)
└── 📁 .devcontainer/                  ← Codespaces konfiguration
    ├── devcontainer.json
    └── setup.sh                       ← Versionstjek script
```

---

### Hvordan Opdaterer Jeg til Nyeste Version

#### I Codespaces:
1. Klik **"Codespaces"** menu i VS Code
2. Klik **"Rebuild Container"**
   - Dette henter den nyeste kode og rebuilder
   - ELLER opret en ny Codespace (altid nyeste)

#### I Lokal VS Code:
```bash
# Hent nyeste ændringer
git pull origin main

# Hvis der er konflikter
git stash           # Gem dine ændringer
git pull origin main
git stash pop       # Gendan dine ændringer
```

---

### Problemløsning

**Sp: Hvordan ved jeg om jeg har den nyeste version?**
- Sv: Kør `git status` og `git log -1`
- I Codespaces: Det vises ved opstart
- Tjek VERSION.txt

**Sp: Jeg vil skifte branch**
```bash
git fetch origin
git checkout <branch-navn>
git pull origin <branch-navn>
```

**Sp: Codespace vil ikke starte**
- Slet gammel Codespace og opret en ny
- Nye Codespaces har altid den nyeste kode

---

## Quick Reference / Hurtig Reference

### Check Version / Tjek Version
```bash
cat VERSION.txt
git log -1 --oneline
git status
```

### Update / Opdater
```bash
git pull origin main
```

### Open Workspace / Åbn Workspace
```bash
code CS2-SkinChanger.code-workspace
```

---

## 📚 More Documentation / Mere Dokumentation

- **VSCODE_SETUP.md** - Complete VS Code setup guide
- **WORKSPACE_SUMMARY.md** - Quick reference
- **README.md** - Main documentation
- **FAQ.md** - Frequently asked questions

---

**Remember / Husk:** GitHub Codespaces always gives you the latest version! 🚀
