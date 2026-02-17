// CS2 Weapons Inventory Dashboard
let skinDatabase = null;
let currentConfigs = [];
let selectedWeapon = null;
let selectedSkin = null;
let selectedSkinIndex = null;

async function initDashboard() {
    const username = localStorage.getItem('username');
    document.getElementById('usernameDisplay').textContent = username || 'User';
    
    await loadSkinDatabase();
    renderWeaponsInventory();
    await loadConfigurations();
    renderActiveConfig();
    setupEventListeners();
}

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

function renderWeaponsInventory() {
    const inventory = document.getElementById('weaponsInventory');
    
    if (!skinDatabase || !skinDatabase.weapons) {
        inventory.innerHTML = '<p class="weapons-empty">No weapons available</p>';
        return;
    }
    
    const weapons = Object.entries(skinDatabase.weapons);
    
    inventory.innerHTML = weapons.map(([weaponName, weaponData]) => {
        const hasConfig = currentConfigs.some(c => c.weapon_name === weaponName);
        return `
            <div class="weapon-slot ${hasConfig ? 'configured' : ''}" onclick="openWeaponModal('${weaponName}')">
                <div class="weapon-icon">${getWeaponEmoji(weaponName)}</div>
                <div class="weapon-label">${weaponName}</div>
                ${hasConfig ? `<div class="weapon-checkmark">✓</div>` : ''}
            </div>
        `;
    }).join('');
}

function renderActiveConfig() {
    const configDisplay = document.getElementById('configDisplay');
    
    if (currentConfigs.length === 0) {
        configDisplay.innerHTML = '<p class="empty-config">No weapons configured</p>';
        return;
    }
    
    configDisplay.innerHTML = currentConfigs.map(config => `
        <div class="config-item">
            <div class="config-item-header">
                <span class="config-weapon-name">${config.weapon_name}</span>
                <button class="config-remove-btn" onclick="removeConfig(${config.id})">✕</button>
            </div>
            <div class="config-item-details">
                <span class="config-skin-name">${config.skin_name}</span>
                ${config.phase ? `<span class="config-tag">${config.phase}</span>` : ''}
                ${config.float_condition ? `<span class="config-tag">${config.float_condition}</span>` : ''}
                ${config.stattrak ? `<span class="config-tag stattrak">StatTrak™</span>` : ''}
            </div>
        </div>
    `).join('');
}

async function removeConfig(configId) {
    try {
        await ConfigAPI.delete(configId);
        await loadConfigurations();
        renderWeaponsInventory();
        renderActiveConfig();
        showSuccess('Configuration removed');
    } catch (error) {
        showError(error.message);
    }
}

function openWeaponModal(weaponName) {
    selectedWeapon = weaponName;
    const weaponData = skinDatabase.weapons[weaponName];
    
    const modal = document.getElementById('weaponDetailModal');
    document.getElementById('modalWeaponName').textContent = weaponName;
    
    const skinsList = document.getElementById('skinsList');
    skinsList.innerHTML = weaponData.skins.map((skin, index) => `
        <div class="skin-item" onclick="selectSkinForConfig(${index})">
            <img src="${skin.image}" alt="${skin.name}" class="skin-item-image" onerror="this.src='data:image/svg+xml,%3Csvg xmlns=%22http://www.w3.org/2000/svg%22 viewBox=%220 0 200 120%22%3E%3Crect fill=%22%23333%22 width=%22200%22 height=%22120%22/%3E%3Ctext x=%2250%25%22 y=%2250%25%22 dominant-baseline=%22middle%22 text-anchor=%22middle%22 fill=%22%23999%22 font-size=%2216%22%3ENo Image%3C/text%3E%3C/svg%3E'">
            <div class="skin-item-info">
                <div class="skin-item-name">${skin.name}</div>
                <div class="skin-item-rarity" style="color: ${getRarityColor(skin.rarity)}">${skin.rarity}</div>
            </div>
        </div>
    `).join('');
    
    modal.style.display = 'flex';
}

function selectSkinForConfig(skinIndex) {
    selectedSkinIndex = skinIndex;
    const weaponData = skinDatabase.weapons[selectedWeapon];
    selectedSkin = weaponData.skins[skinIndex];
    
    // Close weapon modal and open config modal
    document.getElementById('weaponDetailModal').style.display = 'none';
    openSkinConfigModal();
}

function openSkinConfigModal() {
    const modal = document.getElementById('skinConfigModal');
    
    // Set preview image
    document.getElementById('skinPreviewImage').src = selectedSkin.image;
    document.getElementById('skinPreviewName').textContent = selectedSkin.name;
    document.getElementById('skinPreviewRarity').textContent = selectedSkin.rarity;
    document.getElementById('skinPreviewCollection').textContent = selectedSkin.caseCollection || '';
    
    // Setup phase selector
    const phaseSection = document.getElementById('phaseSection');
    if (selectedSkin.phases && selectedSkin.phases.length > 0) {
        phaseSection.style.display = 'block';
        const phaseSelect = document.getElementById('phaseSelect');
        phaseSelect.innerHTML = '<option value="">Select Phase</option>' +
            selectedSkin.phases.map(p => `<option value="${p}">${p}</option>`).join('');
    } else {
        phaseSection.style.display = 'none';
    }
    
    // Setup StatTrak selector
    const statTrakSection = document.getElementById('statTrakSection');
    if (selectedSkin.statTrak) {
        statTrakSection.style.display = 'block';
        document.getElementById('statTrakCheck').checked = false;
    } else {
        statTrakSection.style.display = 'none';
    }
    
    // Reset condition selector
    document.querySelectorAll('.condition-btn').forEach(btn => {
        btn.classList.remove('active');
    });
    document.querySelectorAll('.condition-btn')[0].classList.add('active');
    
    modal.style.display = 'flex';
}

async function applySkinConfiguration() {
    // Get selected values
    const phase = document.getElementById('phaseSelect').value || null;
    const condition = document.querySelector('.condition-btn.active').dataset.condition;
    const hasStatTrak = document.getElementById('statTrakCheck').checked;
    
    const weaponData = skinDatabase.weapons[selectedWeapon];
    
    const config = {
        weapon_name: selectedWeapon,
        weapon_id: weaponData.id,
        weapon_category: weaponData.category,
        skin_name: selectedSkin.name,
        paint_kit: 0,
        phase: phase,
        float_condition: condition,
        stattrak: hasStatTrak,
        rarity: selectedSkin.rarity
    };
    
    const btn = document.getElementById('applySkinBtn');
    btn.disabled = true;
    btn.innerHTML = '<span class="loading"></span> Applying...';
    
    try {
        const existingConfig = currentConfigs.find(c => c.weapon_name === selectedWeapon);
        
        if (existingConfig) {
            await ConfigAPI.update(existingConfig.id, config);
            showSuccess(`${selectedSkin.name} updated on ${selectedWeapon}`);
        } else {
            await ConfigAPI.create(config);
            showSuccess(`${selectedSkin.name} applied to ${selectedWeapon}`);
        }
        
        closeSkinModal();
        await loadConfigurations();
        renderWeaponsInventory();
        renderActiveConfig();
    } catch (error) {
        showError(error.message);
    } finally {
        btn.disabled = false;
        btn.innerHTML = 'Apply Skin';
    }
}

async function loadConfigurations() {
    try {
        const response = await ConfigAPI.getAll();
        currentConfigs = response.configs || [];
    } catch (error) {
        console.error('Failed to load configurations:', error);
    }
}

function setupEventListeners() {
    // Condition buttons
    document.querySelectorAll('.condition-btn').forEach(btn => {
        btn.addEventListener('click', (e) => {
            document.querySelectorAll('.condition-btn').forEach(b => b.classList.remove('active'));
            e.target.classList.add('active');
        });
    });
    
    // Apply skin button
    document.getElementById('applySkinBtn').addEventListener('click', applySkinConfiguration);
    
    // Reset all
    document.getElementById('resetAllBtn').addEventListener('click', async () => {
        if (!confirm('Reset all weapon configurations?')) return;
        try {
            await ConfigAPI.deleteAll();
            currentConfigs = [];
            renderWeaponsInventory();
            renderActiveConfig();
            showSuccess('All configurations reset');
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

function closeModal(event) {
    if (event && event.target !== event.currentTarget) return;
    document.getElementById('weaponDetailModal').style.display = 'none';
    selectedWeapon = null;
}

function closeSkinModal(event) {
    if (event && event.target !== event.currentTarget) return;
    document.getElementById('skinConfigModal').style.display = 'none';
    selectedSkin = null;
    selectedSkinIndex = null;
}

async function generateApiToken() {
    const btn = document.getElementById('generateTokenBtn');
    btn.disabled = true;
    btn.innerHTML = '<span class="loading"></span> Generating...';
    
    try {
        const response = await AuthAPI.generateApiToken();
        document.getElementById('apiTokenText').textContent = response.apiToken;
        document.getElementById('copyTokenBtn').style.display = 'inline-flex';
        localStorage.setItem('apiToken', response.apiToken);
        showSuccess('API token generated');
    } catch (error) {
        showError(error.message);
    } finally {
        btn.disabled = false;
        btn.textContent = 'Generate';
    }
}

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

function showSuccess(message) {
    const container = document.querySelector('.container');
    const div = document.createElement('div');
    div.className = 'success-message';
    div.textContent = message;
    container.insertBefore(div, container.firstChild);
    setTimeout(() => div.remove(), 5000);
}

function showError(message) {
    const container = document.querySelector('.container');
    const div = document.createElement('div');
    div.className = 'error-message show';
    div.textContent = message;
    container.insertBefore(div, container.firstChild);
    setTimeout(() => div.remove(), 5000);
}
