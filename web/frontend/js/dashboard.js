// Dashboard Logic
let skinDatabase = null;
let currentConfigs = [];

async function initDashboard() {
    // Set username
    const username = localStorage.getItem('username');
    document.getElementById('usernameDisplay').textContent = username || 'User';
    
    // Load skin database
    await loadSkinDatabase();
    
    // Populate category dropdown
    populateCategories();
    
    // Load user configurations
    await loadConfigurations();
    
    // Setup event listeners
    setupDashboardListeners();
}

// Load skin database from backend config
async function loadSkinDatabase() {
    try {
        const response = await fetch('/config/skins.json');
        const data = await response.json();
        skinDatabase = data;
    } catch (error) {
        console.error('Failed to load skin database:', error);
        showError('Failed to load skin database');
    }
}

// Populate category dropdown
function populateCategories() {
    const categorySelect = document.getElementById('categorySelect');
    categorySelect.innerHTML = '<option value="">-- Select Category --</option>';
    
    if (!skinDatabase || !skinDatabase.categories) return;
    
    Object.keys(skinDatabase.categories).forEach(category => {
        const option = document.createElement('option');
        option.value = category;
        option.textContent = category;
        categorySelect.appendChild(option);
    });
}

// Setup dashboard event listeners
function setupDashboardListeners() {
    const categorySelect = document.getElementById('categorySelect');
    const weaponSelect = document.getElementById('weaponSelect');
    const skinSelect = document.getElementById('skinSelect');
    const applySkinBtn = document.getElementById('applySkinBtn');
    const resetAllBtn = document.getElementById('resetAllBtn');
    const generateTokenBtn = document.getElementById('generateTokenBtn');
    const copyTokenBtn = document.getElementById('copyTokenBtn');
    
    // Category change
    categorySelect.addEventListener('change', (e) => {
        const category = e.target.value;
        
        if (!category) {
            weaponSelect.disabled = true;
            weaponSelect.innerHTML = '<option value="">-- Select Weapon --</option>';
            skinSelect.disabled = true;
            skinSelect.innerHTML = '<option value="">-- Select Skin --</option>';
            applySkinBtn.disabled = true;
            return;
        }
        
        populateWeapons(category);
    });
    
    // Weapon change
    weaponSelect.addEventListener('change', (e) => {
        const category = categorySelect.value;
        const weapon = e.target.value;
        
        if (!weapon) {
            skinSelect.disabled = true;
            skinSelect.innerHTML = '<option value="">-- Select Skin --</option>';
            applySkinBtn.disabled = true;
            return;
        }
        
        populateSkins(category, weapon);
    });
    
    // Skin change
    skinSelect.addEventListener('change', (e) => {
        applySkinBtn.disabled = !e.target.value;
    });
    
    // Apply skin
    applySkinBtn.addEventListener('click', async () => {
        await applySkin();
    });
    
    // Reset all configurations
    resetAllBtn.addEventListener('click', async () => {
        if (!confirm('Are you sure you want to reset all skin configurations?')) {
            return;
        }
        
        await resetAllConfigurations();
    });
    
    // Generate API token
    generateTokenBtn.addEventListener('click', async () => {
        await generateApiToken();
    });
    
    // Copy token
    copyTokenBtn.addEventListener('click', () => {
        const tokenText = document.getElementById('apiTokenText').textContent;
        navigator.clipboard.writeText(tokenText);
        
        const btn = copyTokenBtn;
        const originalText = btn.textContent;
        btn.textContent = 'Copied!';
        
        setTimeout(() => {
            btn.textContent = originalText;
        }, 2000);
    });
}

// Populate weapons for selected category
function populateWeapons(category) {
    const weaponSelect = document.getElementById('weaponSelect');
    weaponSelect.innerHTML = '<option value="">-- Select Weapon --</option>';
    
    if (!skinDatabase || !skinDatabase.categories[category]) return;
    
    const weapons = skinDatabase.categories[category];
    Object.keys(weapons).forEach(weaponName => {
        const option = document.createElement('option');
        option.value = weaponName;
        option.textContent = weaponName;
        weaponSelect.appendChild(option);
    });
    
    weaponSelect.disabled = false;
}

// Populate skins for selected weapon
function populateSkins(category, weaponName) {
    const skinSelect = document.getElementById('skinSelect');
    skinSelect.innerHTML = '<option value="">-- Select Skin --</option>';
    
    if (!skinDatabase || !skinDatabase.categories[category] || !skinDatabase.categories[category][weaponName]) return;
    
    const weapon = skinDatabase.categories[category][weaponName];
    const skins = weapon.skins;
    
    skins.forEach(skin => {
        const option = document.createElement('option');
        option.value = JSON.stringify({
            name: skin.name,
            paintKit: skin.paintKit
        });
        option.textContent = skin.name;
        skinSelect.appendChild(option);
    });
    
    skinSelect.disabled = false;
}

// Apply selected skin
async function applySkin() {
    const categorySelect = document.getElementById('categorySelect');
    const weaponSelect = document.getElementById('weaponSelect');
    const skinSelect = document.getElementById('skinSelect');
    
    const category = categorySelect.value;
    const weaponName = weaponSelect.value;
    const skinData = JSON.parse(skinSelect.value);
    
    const weapon = skinDatabase.categories[category][weaponName];
    
    const config = {
        weapon_category: category,
        weapon_name: weaponName,
        weapon_id: weapon.id,
        skin_name: skinData.name,
        paint_kit: skinData.paintKit
    };
    
    const btn = document.getElementById('applySkinBtn');
    btn.disabled = true;
    btn.innerHTML = '<span class="loading"></span> Applying...';
    
    try {
        await ConfigAPI.create(config);
        showSuccess('Skin configuration saved successfully!');
        await loadConfigurations();
    } catch (error) {
        showError(error.message);
    } finally {
        btn.disabled = false;
        btn.textContent = 'Apply Skin';
    }
}

// Load user configurations
async function loadConfigurations() {
    try {
        const response = await ConfigAPI.getAll();
        currentConfigs = response.configs || [];
        renderConfigurations();
    } catch (error) {
        console.error('Failed to load configurations:', error);
    }
}

// Render configurations list
function renderConfigurations() {
    const configsList = document.getElementById('configsList');
    
    if (currentConfigs.length === 0) {
        configsList.innerHTML = '<p class="empty-state">No configurations yet. Start by selecting a weapon and skin above.</p>';
        return;
    }
    
    configsList.innerHTML = currentConfigs.map(config => `
        <div class="config-item">
            <div class="config-info">
                <div class="config-weapon">${config.weapon_name}</div>
                <div class="config-skin">${config.skin_name}</div>
                <div class="config-category">${config.weapon_category} • Paint Kit: ${config.paint_kit}</div>
            </div>
            <div class="config-actions">
                <button class="btn btn-danger btn-small" onclick="deleteConfiguration(${config.id})">Delete</button>
            </div>
        </div>
    `).join('');
}

// Delete configuration
async function deleteConfiguration(id) {
    if (!confirm('Are you sure you want to delete this configuration?')) {
        return;
    }
    
    try {
        await ConfigAPI.delete(id);
        showSuccess('Configuration deleted successfully!');
        await loadConfigurations();
    } catch (error) {
        showError(error.message);
    }
}

// Reset all configurations
async function resetAllConfigurations() {
    try {
        await ConfigAPI.deleteAll();
        showSuccess('All configurations reset successfully!');
        await loadConfigurations();
    } catch (error) {
        showError(error.message);
    }
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
        showSuccess('API token generated successfully!');
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
