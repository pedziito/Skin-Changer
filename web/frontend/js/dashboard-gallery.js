// Dashboard Gallery Logic
let skinDatabase = null;
let currentConfigs = [];
let allSkinsList = [];
let currentCategory = 'all';

async function initDashboard() {
    // Set username
    const username = localStorage.getItem('username');
    document.getElementById('usernameDisplay').textContent = username || 'User';
    
    // Load skin database
    await loadSkinDatabase();
    
    // Render skins gallery
    await renderSkinsGallery();
    
    // Load user configurations
    await loadConfigurations();
    
    // Setup event listeners
    setupDashboardListeners();
}

// Load skin database from config
async function loadSkinDatabase() {
    try {
        const response = await fetch('/config/skins.json');
        const data = await response.json();
        skinDatabase = data;
        
        // Build flat list of all skins
        buildSkinsList();
    } catch (error) {
        console.error('Failed to load skin database:', error);
        showError('Failed to load skin database');
    }
}

// Build flat list of all skins with category and weapon info
function buildSkinsList() {
    allSkinsList = [];
    
    if (!skinDatabase || !skinDatabase.categories) return;
    
    Object.keys(skinDatabase.categories).forEach(category => {
        const weapons = skinDatabase.categories[category];
        Object.keys(weapons).forEach(weaponName => {
            const weapon = weapons[weaponName];
            if (weapon.skins && Array.isArray(weapon.skins)) {
                weapon.skins.forEach(skin => {
                    allSkinsList.push({
                        category: category,
                        weaponName: weaponName,
                        weaponId: weapon.id,
                        skinName: skin.name,
                        paintKit: skin.paintKit,
                        emoji: getWeaponEmoji(weaponName)
                    });
                });
            }
        });
    });
}

// Get emoji for weapon type
function getWeaponEmoji(weaponName) {
    const emojis = {
        'AK-47': '🔴',
        'M4A4': '🟦',
        'M4A1-S': '🟫',
        'AWP': '🟪',
        'Desert Eagle': '🟡',
        'P250': '🟠',
        'Glock-18': '🟢',
        'USP-S': '🟥',
        'Karambit': '🔪',
        'M9 Bayonet': '⚔️',
        'Butterfly Knife': '🦋'
    };
    return emojis[weaponName] || '🎮';
}

// Setup category buttons and render gallery
async function renderSkinsGallery(filterCategory = 'all') {
    currentCategory = filterCategory;
    
    // Setup category buttons if not already done
    setupCategoryButtons();
    
    // Filter skins by category
    const filteredSkins = filterCategory === 'all' 
        ? allSkinsList 
        : allSkinsList.filter(s => s.category === filterCategory);
    
    // Render skins grid
    const gallery = document.getElementById('skinsGallery');
    
    if (filteredSkins.length === 0) {
        gallery.innerHTML = '<div class="gallery-loading">No skins found for this category</div>';
        return;
    }
    
    gallery.innerHTML = filteredSkins.map(skin => {
        const isApplied = currentConfigs.some(config => 
            config.weapon_id === skin.weaponId && 
            config.weapon_name === skin.weaponName
        );
        
        return `
            <div class="skin-card" data-weapon="${skin.weaponName}" data-skin="${skin.skinName}">
                <div class="skin-image">
                    <div class="skin-image-placeholder">
                        <span style="font-size: 5rem;">${skin.emoji}</span>
                    </div>
                </div>
                <div class="skin-info">
                    <div class="skin-weapon">${skin.weaponName}</div>
                    <div class="skin-name">${skin.skinName}</div>
                    <div class="skin-category">${skin.category}</div>
                    <button class="skin-apply-btn ${isApplied ? 'applied' : ''}" 
                            onclick="applySkinCard('${skin.category}', '${skin.weaponName}', '${skin.skinName}', ${skin.paintKit}, ${skin.weaponId})">
                        ${isApplied ? '✓ Applied' : 'Apply Skin'}
                    </button>
                </div>
            </div>
        `;
    }).join('');
}

// Setup category filter buttons
function setupCategoryButtons() {
    const filterContainer = document.querySelector('.category-filter');
    if (!filterContainer || filterContainer.dataset.initialized) return;
    
    const categories = ['all', ...Object.keys(skinDatabase.categories || {})];
    const buttonsHtml = categories.map(cat => {
        const displayName = cat === 'all' ? 'All Weapons' : cat;
        return `<button class="category-btn ${cat === currentCategory ? 'active' : ''}" data-category="${cat}">${displayName}</button>`;
    }).join('');
    
    const categoryButtons = filterContainer.querySelector('.category-buttons');
    if (categoryButtons) {
        categoryButtons.innerHTML = buttonsHtml;
        
        // Add click listeners to category buttons
        categoryButtons.querySelectorAll('.category-btn').forEach(btn => {
            btn.addEventListener('click', (e) => {
                const category = e.target.dataset.category;
                
                // Update active btn
                categoryButtons.querySelectorAll('.category-btn').forEach(b => b.classList.remove('active'));
                e.target.classList.add('active');
                
                // Re-render gallery
                renderSkinsGallery(category);
            });
        });
    }
    
    filterContainer.dataset.initialized = 'true';
}

// Apply skin from gallery card
async function applySkinCard(category, weaponName, skinName, paintKit, weaponId) {
    const btn = event.target;
    btn.disabled = true;
    btn.innerHTML = '<span class="loading"></span> Applying...';
    
    const config = {
        weapon_category: category,
        weapon_name: weaponName,
        weapon_id: weaponId,
        skin_name: skinName,
        paint_kit: paintKit
    };
    
    try {
        // Check if config already exists for this weapon
        const existingConfig = currentConfigs.find(c => c.weapon_id === weaponId && c.weapon_name === weaponName);
        
        if (existingConfig) {
            // Update existing config
            await ConfigAPI.update(existingConfig.id, config);
        } else {
            // Create new config
            await ConfigAPI.create(config);
        }
        
        showSuccess(`${skinName} applied to ${weaponName}!`);
        await loadConfigurations();
        await renderSkinsGallery(currentCategory);
    } catch (error) {
        showError(error.message);
        btn.disabled = false;
        btn.innerHTML = 'Apply Skin';
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
function setupDashboardListeners() {
    const resetAllBtn = document.getElementById('resetAllBtn');
    const generateTokenBtn = document.getElementById('generateTokenBtn');
    const copyTokenBtn = document.getElementById('copyTokenBtn');
    const logoutBtn = document.getElementById('logoutBtn');
    
    // Reset all configurations
    if (resetAllBtn) {
        resetAllBtn.addEventListener('click', async () => {
            if (!confirm('Are you sure you want to reset all skin configurations?')) {
                return;
            }
            
            await resetAllConfigurations();
        });
    }
    
    // Generate API token
    if (generateTokenBtn) {
        generateTokenBtn.addEventListener('click', async () => {
            await generateApiToken();
        });
    }
    
    // Copy token
    if (copyTokenBtn) {
        copyTokenBtn.addEventListener('click', () => {
            const tokenText = document.getElementById('apiTokenText').textContent;
            navigator.clipboard.writeText(tokenText);
            
            const originalText = copyTokenBtn.textContent;
            copyTokenBtn.textContent = 'Copied!';
            
            setTimeout(() => {
                copyTokenBtn.textContent = originalText;
            }, 2000);
        });
    }
    
    // Logout
    if (logoutBtn) {
        logoutBtn.addEventListener('click', () => {
            removeToken();
            localStorage.removeItem('username');
            
            const dashboard = document.getElementById('dashboard');
            const loginPage = document.getElementById('loginPage');
            
            dashboard.style.display = 'none';
            loginPage.style.display = 'flex';
            
            document.getElementById('loginForm').reset();
        });
    }
}

// Reset all configurations
async function resetAllConfigurations() {
    try {
        await ConfigAPI.deleteAll();
        showSuccess('All configurations reset successfully!');
        await loadConfigurations();
        await renderSkinsGallery(currentCategory);
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
