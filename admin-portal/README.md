# CS2 Admin Portal

En web-baseret admin panel til at administrere CS2 Skin Changer licenser.

## 🔐 Login

**Admin Password:** `Mao770609`

## Features

### 1. Generate License
- Opret nye brugerlicenser
- Indstil gyldighed (1-365 dage)
- Tilføj noter til licenserne
- Kopier til clipboard

### 2. Validate License
- Valider eksisterende licensfiler
- Kontroller format og gyldighed
- Se licensdetaljer (bruger, oprettelsesdato, udløbsdato)

### 3. Manage Licenses
- Oversigt over alle licensener
- Status: ACTIVE / REVOKED
- Genopkald (revoke) licenserner
- Statistik over licensner

## Brug

### Lokalt (Offline)
1. Åbn `index.html` i en webbrowser
2. Indtast admin password: `Mao770609`
3. Administrer licensener

### Deployment (Online)
Kan deployes på en web server (Apache, Nginx, Node.js osv.).

**Note:** Alt data gemmes lokalt i browsers localStorage - ingen server-forbindelse nødvendig.

## License Format

```
CS2-2026-A1B2C3D4-02-E5F6G7H8
USERNAME=player_name
CREATED=2026-02-17 12:34:56
EXPIRY=2026-03-19 12:34:56
ACTIVE=true
```

## Workflow

### For Admin
1. Åbn admin portal
2. Login med `Mao770609`
3. Gå til "Generate License"
4. Indtast brugerens navn
5. Indstil gyldighed (dage)
6. Klik "Generate License"
7. Kopier licensnøglen
8. Send til brugeren

### For Bruger
1. Modtag license.key fil fra admin
2. Placer i samme mappe som `cs2loader.exe`
3. Kør `cs2loader.exe`
4. Accepter VAC-advarsel
5. Start CS2
6. Menu åbnes automatisk

## Security

- Admin password: `Mao770609`
- Lokalt lagrede licenses (localStorage)
- Ingen eksterne forbindelser
- Simpel format-validering

## Tips

- **Backup:** Licenses gemmes i browser localStorage - husk at backuppe dine data
- **Eksport:** Kopier license-nøglen for at gemme den sikkert
- **Offline:** Fungerer helt uden internetforbindelse
- **Multi-device:** Kan tilgås fra flere enheder (hvert med sin localStorage)
