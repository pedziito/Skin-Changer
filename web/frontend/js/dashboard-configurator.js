// CS2 Skin Configurator Dashboard
let skinDatabase = null;
let currentConfigs = [];
let selectedWeapon = null;
let selectedWeaponType = null;

async function initDashboard() {
    // Set username
    const username = localStorage.getItem('username');
    document.getElementById('usernameDisplay').textContent = username || 'User';
    
    // Load skin database
    await loadSkinDatabase();
    
    // Render weapons grid
    renderWeaponsGrid();
    
    // Load user configurations
    await loadConfigurations();
    
    // Setup event listeners
    setupEventListeners();
}

// Load complete skin database
async function loadSkinDatabase() {
    try {
        const response = await fetch('/config/cs2-skins-complete.json');
        const data = await response.json();
        skinDatabase = data;
    } catch (error) {
        console.error('Failed to load skin database:', error);
        showError('Failed to load skin database');
    }
}

// Render weapons grid with + buttons
function renderWeaponsGrid() {
    const grid = document.getElementById('weaponsGrid');
    
    if (!skinDatabase || !skinDatabase.weapons) {
        grid.innerHTML = '<p class="grid-empty">No weapons available</p>';
        return;
    }
    
    const weapons = Object.entries(skinDatabase.weapons);
    
    grid.innerHTML = weapons.map(([weaponName, weaponData]) => {
        const hasConfig = currentConfigs.some(c => c.weapon_name === weaponName);
        const configDisplay = hasConfig 
            ? currentConfigs.find(c => c.weapon_name === weaponName).skin_name 
            : 'Not Selected';
        
        return `
            <div class="weapon-slot">
                <div class="weapon-display">
                    <div class="weapon-emoji">${getWeaponEmoji(weaponName)}</div>
                    <div class="weapon-info">
                        <div class="weapon-name">${weaponName}</div>
                        <div class="weapon-type">${weaponData.type}</div>
                        <div class="weapon-skin">${configDisplay}</div>
                    </div>
                </div>
                <button class="btn-add-weapon" onclick="openWeaponModal('${weaponName}')">+</button>
            </div>
        `;
    }).join('');
}

// Get emoji for weapon
function getWeaponEmoji(weaponName) {
    const emojis = {
        'Glock-18': '🔫',
        'USP-S': '🔴',
        'AK-47': '🟧',
        'M4A4': '🟦',
        'M4A1-S': '🟩',
        'AWP Dragon Lore': '🎯',
        'Karambit': '🔪',
        'M9 Bayonet': '⚔️',
        'Butterfly Knife': '🦋',
        'Gut Knife': '🗡️'
    };
    return emojis[weaponName] || '🎮';
}

// Open weapon selection modal
function openWeaponModal(currentWeaponName) {
    selectedWeapon = currentWeaponName;
    selectedWeaponType = skinDatabase.weapons[currentWeaponName].type;
    
    const modal = document.getElementById('weaponSelectModal');
    const weaponList = document.getElementById('weaponList');
    
    // Get all weapons of same type
    const sameTypeWeapons = Object.entries(skinDatabase.weapons)
        .filter(([name, data]) => data.type === selectedWeaponType)
        .map(([name, data]) => ({ name, type: data.type }));
    
    weaponList.innerHTML = sameTypeWeapons.map(weapon => `
        <button class="weapon-list-item ${weapon.name === selectedWeapon ? 'active' : ''}" 
                onclick="selectWeapon('${weapon.name}')">
            <span class="weapon-list-emoji">${getWeaponEmoji(weapon.name)}</span>
            <span class="weapon-list-name">${weapon.name}</span>
            <span class="weapon-list-type">${weapon.type}</span>
        </button>
    `).join('');
    
    modal.style.display = 'flex';
}

// Select a weapon and move to skin selection
function selectWeapon(weaponName) {
    selectedWeapon = weaponName;
    
    // Close weapon modal
    document.getElementById('weaponSelectModal').style.display = 'none';
    
    // Open skin modal
    openSkinModal(weaponName);
}

// Open skin selection modal
function openSkinModal(weaponName) {
    selectedWeapon = weaponName;
    const weaponData = skinDatabase.weapons[weaponName];
    
    const modal = document.getElementById('skinSelectModal');
    document.getElementById('skinModalTitle').textContent = `Select Skin for ${weaponName}`;
    
    const skinsList = document.getElementById('skinsList');
    skinsList.innerHTML = weaponData.skins.map((skin, index) => `
        <div class="skin-list-item" onclick="selectSkin(${index}, '${weaponName}')">
            <div class="skin-list-name">${skin.name}</div>
            <div class="skin-list-rarity" style="color: ${getRarityColor(skin.rarity)}">${skin.rarity}</div>
        </div>
    `).join('');
    
    // Setup phase selector
    const phaseSelector = document.getElementById('phaseSelector');
    const phaseSelect = document.getElementById('phaseSelect');
    const firstSkin = weaponData.skins[0];
    
    if (firstSkin.phases && firstSkin.phases.length > 0) {
        phaseSelector.style.display = 'block';
        phaseSelect.innerHTML = `<option value="">Select Phase</option>` + 
            firstSkin.phases.map(phase => `<option value="${phase}">${phase}</option>`).join('');
    } else {
        phaseSelector.style.display = 'none';
    }
    
    // Setup StatTrak selector
    const statTrakSelector = document.getElementById('statTrakSelector');
    if (firstSkin.statTrak) {
        statTrakSelector.style.display = 'block';
    } else {
        statTrakSelector.style.display = 'none';
    }
    
    // Set image
    document.getElementById('skinModalImage').src = firstSkin.image || '';
    
    modal.style.display = 'flex';
}

// Select a skin
function selectSkin(skinIndex, weaponName) {
    const weaponData = skinDatabase.weapons[weaponName];
    const skin = weaponData.skins[skinIndex];
    
    // Update image
    document.getElementById('skinModalImage').src = skin.image || '';
    
    // Highlight selected skin
    document.querySelectorAll('.skin-list-item').forEach((item, i) => {
        if (i === skinIndex) {
            item.classList.add('selected');
        } else {
            item.classList.remove('selected');
        }
    });
    
    // Update phase selector
    const phaseSelector = document.getElementById('phaseSelector');
    const phaseSelect = document.getElementById('phaseSelect');
    
    if (skin.phases && skin.phases.length > 0) {
        phaseSelector.style.display = 'block';
        phaseSelect.innerHTML = `<option value="">Select Phase</option>` + 
            skin.phases.map(phase => `<option value="${phase}">${phase}</option>`).join('');
    } else {
        phaseSelector.style.display = 'none';
    }
    
    // Update StatTrak selector
    const statTrakSelector = document.getElementById('statTrakSelector');
    if (skin.statTrak) {
        statTrakSelector.style.display = 'block';
    } else {
        statTrakSelector.style.display = 'none';
        document.getElementById('statTrakCheck').checked = false;
    }
    
    // Store selected skin
    window.selectedSkinIndex = skinIndex;
}

// Confirm skin selection
async function confirmSkinSelection() {
    if (window.selectedSkinIndex === undefined) {
        showError('Please select a skin first');
        return;
    }
    
    const weaponName = selectedWeapon;
    const weaponData = skinDatabase.weapons[weaponName];
    const skin = weaponData.skins[window.selectedSkinIndex];
    
    const phase = document.getElementById('phaseSelect').value || null;
    const floatCondition = document.getElementById('floatSelect').value || null;
    const hasStatTrak = document.getElementById('statTrakCheck').checked;
    
    const config = {
        weapon_name: weaponName,
        weapon_id: weaponData.id,
        weapon_category: weaponData.category,
        skin_name: skin.name,
        paint_kit: 0, // This would be the actual paint kit value
        phase: phase,
        float_condition: floatCondition,
        stattrak: hasStatTrak,
        rarity: skin.rarity
    };
    
    const btn = document.getElementById('confirmSkinBtn');
    btn.disabled = true;
    btn.innerHTML = '<span class="loading"></span> Applying...';
    
    try {
        // Check if config already exists
        const existingConfig = currentConfigs.find(c => c.weapon_name === weaponName);
        
        if (existingConfig) {
            await ConfigAPI.update(existingConfig.id, config);
        } else {
            await ConfigAPI.create(config);
        }
        
        showSuccess(`${skin.name} applied to ${weaponName}!`);
        
        // Close modal
        document.getElementById('skinSelectModal').style.display = 'none';
        
        // Reload and re-render
        await loadConfigurations();
        renderWeaponsGrid();
        
        window.selectedSkinIndex = undefined;
    } catch (error) {
        showError(error.message);
    } finally {
        btn.disabled = false;
        btn.innerHTML = 'Apply This Skin';
    }
}

// Load user configurations
async function loadConfigurations() {
    try {
        const response = await ConfigAPI.getAll();
        currentConfigs = response.configs || [];
    } catch (error) {
        console.error('Failed to load configurations:', error);
    }
}

// Setup event listeners
function setupEventListeners() {
    // Modal close buttons
    document.querySelectorAll('.modal-close').forEach(btn => {
        btn.addEventListener('click', (e) => {
            e.target.closest('.modal').style.display = 'none';
            window.selectedSkinIndex = undefined;
        });
    });
    
    // Modal outside click to close
    document.querySelectorAll('.modal').forEach(modal => {
        modal.addEventListener('click', (e) => {
            if (e.target === modal) {
                modal.style.display = 'none';
                window.selectedSkinIndex = undefined;
            }
        });
    });
    
    // Confirm skin button
    document.getElementById('confirmSkinBtn').addEventListener('click', confirmSkinSelection);
    
    // Reset all
    document.getElementById('resetAllBtn').addEventListener('click', async () => {
        if (!confirm('Reset all weapon skins?')) return;
        
        try {
            await ConfigAPI.deleteAll();
            showSuccess('All configurations reset!');
            currentConfigs = [];
            renderWeaponsGrid();
        } catch (error) {
            showError(error.message);
        }
    });
    
    // Generate token
    document.getElementById('generateTokenBtn').addEventListener('click', generateApiToken);
    
    // Copy token
    document.getElementById('copyTokenBtn').addEventListener('click', () => {
        const token = document.getElementById('apiTokenText').textContent;
        navigator.clipboard.writeText(token);
        
        const btn = document.getElementById('copyTokenBtn');
        const originalText = btn.textContent;
        btn.textContent = 'Copied!';
        
        setTimeout(() => {
            btn.textContent = originalText;
        }, 2000);
    });
    
    // Logout
    document.getElementById('logoutBtn').addEventListener('click', () => {
        removeToken();
        localStorage.removeItem('username');
        
        document.getElementById('dashboard').style.display = 'none';
        document.getElementById('loginPage').style.display = 'flex';
        
        document.getElementById('loginForm').reset();
    });
}

// Get rarity color
function getRarityColor(rarity) {
    const colors = {
        'Consumer Grade': '#B0C3D9',
        'Industrial Grade': '#5E98D9',
        'Mil-Spec': '#4B69FF',
        'Restricted': '#8847FF',
        'Classified': '#D32EE6',
        'Covert': '#EB4B4B'
    };
    return colors[rarity] || '#FFFFFF';
}

// Generate API token
async function generateApiToken() {
    const btn = document.getElementById('generateTokenBtn');
    btn.disabled = true;
    btn.innerHTML = '<span class="loading"></span> Generating...';
    
    try {
        const response = await AuthAPI.generateApiToken();
        const apiToken = response.apiToken;
        
        document.getElementById('apiTokenText').textContent = apiToken;
        document.getElementById('copyTokenBtn').style.display = 'inline-flex';
        
        localStorage.setItem('apiToken', apiToken);
        showSuccess('API token generated!');
    } catch (error) {
        showError(error.message);
    } finally {
        btn.disabled = false;
        btn.textContent = 'Generate Token';
    }
}

// Show success message
function showSuccess(message) {
    const container = document.querySelector('.container');
    const successDiv = document.createElement('div');
    successDiv.className = 'success-message';
    successDiv.textContent = message;
    
    container.insertBefore(successDiv, container.firstChild);
    
    setTimeout(() => {
        successDiv.remove();
    }, 5000);
}

// Show error message
function showError(message) {
    const container = document.querySelector('.container');
    const errorDiv = document.createElement('div');
    errorDiv.className = 'error-message show';
    errorDiv.textContent = message;
    
    container.insertBefore(errorDiv, container.firstChild);
    
    setTimeout(() => {
        errorDiv.remove();
    }, 5000);
}
