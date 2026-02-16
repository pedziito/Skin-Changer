const express = require('express');
const path = require('path');
const archiver = require('archiver');
const fs = require('fs');

const router = express.Router();

// Download desktop client (.exe)
router.get('/client', (req, res) => {
  const exePath = path.join(__dirname, '../../build/bin/Release/CS2SkinChanger.exe');
  
  // Check if exe exists
  if (fs.existsSync(exePath)) {
    res.download(exePath, 'CS2SkinChanger.exe', (err) => {
      if (err) {
        console.error('Error downloading exe:', err);
        res.status(500).json({ error: 'Failed to download client' });
      }
    });
  } else {
    // If exe doesn't exist, provide instructions
    res.status(404).json({ 
      error: 'Desktop client not built yet',
      message: 'Please build the project first using CMake. See BUILD.md for instructions.',
      buildInstructions: 'Run: mkdir build && cd build && cmake .. -G "Visual Studio 17 2022" -A x64 && cmake --build . --config Release'
    });
  }
});

// Download web server package as zip
router.get('/web-server', async (req, res) => {
  try {
    const archive = archiver('zip', {
      zlib: { level: 9 }
    });

    res.attachment('CS2-SkinChanger-WebServer.zip');
    archive.pipe(res);

    // Add web directory contents
    const webDir = path.join(__dirname, '../../');
    
    // Add backend files
    archive.directory(path.join(webDir, 'backend'), 'web/backend', {
      ignore: ['node_modules/**', 'skinchanger.db', '.env']
    });
    
    // Add frontend files
    archive.directory(path.join(webDir, 'frontend'), 'web/frontend');
    
    // Add documentation
    const rootDir = path.join(__dirname, '../../../');
    archive.file(path.join(webDir, 'README.md'), { name: 'README.md' });
    archive.file(path.join(webDir, 'DEPLOYMENT.md'), { name: 'DEPLOYMENT.md' });
    archive.file(path.join(rootDir, 'LICENSE'), { name: 'LICENSE' });
    
    // Add startup scripts
    archive.file(path.join(rootDir, 'start-web-server.bat'), { name: 'start-web-server.bat' });
    archive.file(path.join(rootDir, 'start-web-server.sh'), { name: 'start-web-server.sh' });
    
    // Add installation instructions
    const instructions = `
# CS2 Skin Changer - Web Server Package

## Quick Start

### Windows:
1. Double-click start-web-server.bat
2. Wait for dependencies to install
3. Open browser to http://localhost:3000

### Linux/Mac:
1. Run: chmod +x start-web-server.sh
2. Run: ./start-web-server.sh
3. Open browser to http://localhost:3000

## Default Admin Login
- Username: admin
- Password: admin123
- ⚠️ Change this immediately!

## Documentation
- See README.md for detailed setup
- See DEPLOYMENT.md for deployment options

## Support
- GitHub: https://github.com/pedziito/Skin-Changer
- For issues, open a GitHub issue

## Warning
⚠️ This tool violates Valve's ToS and will result in VAC ban.
⚠️ Educational purposes only.
⚠️ Use at your own risk.
`;
    
    archive.append(instructions, { name: 'INSTALL.txt' });
    
    await archive.finalize();
    
  } catch (error) {
    console.error('Error creating web server package:', error);
    res.status(500).json({ error: 'Failed to create package' });
  }
});

// Get download statistics (for future use)
router.get('/stats', (req, res) => {
  // This could track download counts, etc.
  res.json({
    clientDownloads: 0,
    webServerDownloads: 0,
    lastUpdated: new Date().toISOString()
  });
});

module.exports = router;
