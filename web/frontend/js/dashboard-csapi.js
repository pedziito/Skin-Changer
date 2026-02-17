// CS2 Skin Changer Dashboard - CS API Integration
// Loads 13,412+ skins from qwkdev csapi database

let skinDatabase = [];
let filteredSkins = [];
let selectedSkin = null;
let selectedWeapon = null;
let selectedFloat = "Factory New";
let selectedStatTrak = false;

// Rarity colors for borders
const rarityColors = {
    'Consumer Grade': '#b0e0e6',     // Light blue
    'Industrial Grade': '#95b8d1',   // Steel blue
    'Mil-Spec': '#4b7ba7',           // Blue
    'Restricted': '#8847ff',         // Purple
    'Classified': '#d32ce6',          // Magenta
    'Covert': '#eb4b4b',              // Red
    'Contraband': '#e4ae39'           // Gold
};

const rarityRarityClasses = {
    'Consumer Grade': 'rarity-blue',
    'Industrial Grade': 'rarity-blue',
    'Mil-Spec': 'rarity-blue',
    'Restricted': 'rarity-purple',
    'Classified': 'rarity-magenta',
    'Covert': 'rarity-red',
    'Contraband': 'rarity-gold'
};

/**
 * Initialize the dashboard
 */
async function initDashboard() {
    console.log('🎮 Initializing CS2 Inventory Dashboard...');
    
    try {
        // Load skin database
        await loadSkinDatabase();
        
        // Set up event listeners
        setupFilters();
        setupAPIToken();
        
        // Initial render
        filterAndRender();
        
        console.log('✅ Dashboard initialized successfully');
    } catch (error) {
        console.error('❌ Dashboard initialization error:', error);
        showErrorState('Failed to load skin database');
    }
}

/**
 * Load skin database from csapi GitHub repo
 */
async function loadSkinDatabase() {
    console.log('📥 Loading skin database from csapi...');
    const url = 'https://raw.githubusercontent.com/qqwkk/csapi/main/data2.json';
    
    try {
        const response = await fetch(url);
        if (!response.ok) throw new Error(`HTTP ${response.status}`);
        
        const data = await response.json();
        
        // Parse and normalize skin data
        skinDatabase = [];
        const weaponsSet = new Set();
        
        for (const [weaponName, skins] of Object.entries(data)) {
            weaponsSet.add(weaponName);
            
            if (Array.isArray(skins)) {
                skins.forEach(skin => {
                    skinDatabase.push({
                        weapon: weaponName,
                        name: skin.name || 'Unknown',
                        rarity: skin.rarity || 'Consumer Grade',
                        image: skin.image || '',
                        phase: skin.phase || null,
                        collection: skin.collection || ''
                    });
                });
            }
        }
        
        console.log(`✅ Loaded ${skinDatabase.length} skins from ${weaponsSet.size} weapons`);
        console.log('📊 Weapons:', Array.from(weaponsSet).sort());
        
        // Populate weapon filter
        const weaponFilter = document.getElementById('weaponFilter');
        Array.from(weaponsSet).sort().forEach(weapon => {
            const option = document.createElement('option');
            option.value = weapon;
            option.textContent = weapon;
            weaponFilter.appendChild(option);
        });
        
    } catch (error) {
        console.error('❌ Failed to load skin database:', error);
        throw error;
    }
}

/**
 * Set up filter event listeners
 */
function setupFilters() {
    document.getElementById('searchInput').addEventListener('input', filterAndRender);
    document.getElementById('weaponFilter').addEventListener('change', filterAndRender);
    document.getElementById('rarityFilter').addEventListener('change', filterAndRender);
}

/**
 * Filter skins based on search and dropdowns
 */
function filterAndRender() {
    const searchTerm = document.getElementById('searchInput').value.toLowerCase();
    const weaponFilter = document.getElementById('weaponFilter').value;
    const rarityFilter = document.getElementById('rarityFilter').value;
    
    filteredSkins = skinDatabase.filter(skin => {
        const matchesSearch = skin.name.toLowerCase().includes(searchTerm) ||
                             skin.weapon.toLowerCase().includes(searchTerm);
        const matchesWeapon = !weaponFilter || skin.weapon === weaponFilter;
        const matchesRarity = !rarityFilter || skin.rarity === rarityFilter;
        
        return matchesSearch && matchesWeapon && matchesRarity;
    });
    
    console.log(`🔍 Filtered to ${filteredSkins.length} skins`);
    renderInventory();
}

/**
 * Render the inventory grid
 */
function renderInventory() {
    const grid = document.getElementById('inventoryGrid');
    
    if (filteredSkins.length === 0) {
        grid.innerHTML = '<div class="loading-state"><p>No skins found</p></div>';
        return;
    }
    
    grid.innerHTML = filteredSkins.map(skin => `
        <div class="skin-card ${rarityRarityClasses[skin.rarity] || 'rarity-blue'}" 
             onclick="selectSkinForConfig(this, '${skin.weapon}', '${skin.name.replace(/'/g, "\\'")}')">
            <img class="skin-image" src="${sanitizeImageUrl(skin.image)}" alt="${skin.name}" onerror="this.src='data:image/svg+xml,%3Csvg xmlns=%22http://www.w3.org/2000/svg%22 width=%22512%22 height=%22384%22%3E%3Crect fill=%22%23333%22 width=%22512%22 height=%22384%22/%3E%3C/svg%3E'">
            <div class="skin-card-info">
                <div class="weapon-name">${skin.weapon}</div>
                <div class="skin-name">${skin.name}</div>
                <div class="rarity-badge">${skin.rarity}</div>
            </div>
        </div>
    `).join('');
}

/**
 * Sanitize image URLs
 */
function sanitizeImageUrl(url) {
    if (!url) return 'data:image/svg+xml,%3Csvg xmlns=%22http://www.w3.org/2000/svg%22 width=%22512%22 height=%22384%22%3E%3Crect fill=%22%23333%22 width=%22512%22 height=%22384%22/%3E%3C/svg%3E';
    
    // If it's a Steam CDN URL, ensure proper format
    if (url.includes('steamcommunity') || url.includes('cloudflare')) {
        // Already valid
        return url;
    }
    
    // If it's just an ID, construct the full Steam CDN URL
    if (!url.startsWith('http')) {
        return `https://community.cloudflare.steamstatic.com/economy/image/${url}/512fx384f`;
    }
    
    return url;
}

/**
 * Open skin modal for configuration
 */
function selectSkinForConfig(cardElement, weapon, skinName) {
    selectedSkin = skinDatabase.find(s => s.weapon === weapon && s.name === skinName);
    selectedWeapon = weapon;
    selectedFloat = "Factory New";
    selectedStatTrak = false;
    
    if (!selectedSkin) {
        console.error('Skin not found');
        return;
    }
    
    // Update modal content
    document.getElementById('modalSkinImage').src = sanitizeImageUrl(selectedSkin.image);
    document.getElementById('modalWeaponName').textContent = selectedSkin.weapon;
    document.getElementById('modalSkinName').textContent = selectedSkin.name;
    document.getElementById('modalRarity').textContent = selectedSkin.rarity;
    document.getElementById('modalRarity').style.color = rarityColors[selectedSkin.rarity] || '#fff';
    
    // Reset float selection
    document.querySelectorAll('.float-btn').forEach(btn => {
        btn.classList.remove('active');
        if (btn.dataset.float === 'Factory New') {
            btn.classList.add('active');
        }
    });
    
    // Reset StatTrak
    document.getElementById('statTrakCheck').checked = false;
    const statTrakOption = document.getElementById('statTrakOption');
    statTrakOption.style.display = selectedSkin.rarity !== 'Consumer Grade' ? 'block' : 'none';
    
    // Show modal
    document.getElementById('skinModal').style.display = 'flex';
    document.body.style.overflow = 'hidden';
    
    // Set up float buttons
    document.querySelectorAll('.float-btn').forEach(btn => {
        btn.onclick = function() {
            document.querySelectorAll('.float-btn').forEach(b => b.classList.remove('active'));
            this.classList.add('active');
            selectedFloat = this.dataset.float;
        };
    });
    
    // Set up StatTrak checkbox
    document.getElementById('statTrakCheck').onchange = function() {
        selectedStatTrak = this.checked;
    };
    
    // Set up apply button
    document.getElementById('applyBtn').onclick = applySkinConfiguration;
}

/**
 * Close skin modal
 */
function closeSkinModal(event) {
    if (event && event.target.id !== 'skinModal') return;
    
    document.getElementById('skinModal').style.display = 'none';
    document.body.style.overflow = 'auto';
    selectedSkin = null;
}

/**
 * Apply skin configuration
 */
async function applySkinConfiguration() {
    if (!selectedSkin || !selectedWeapon) {
        alert('Please select a skin first');
        return;
    }
    
    try {
        const response = await fetch('/api/apply-skin', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
                'Authorization': `Bearer ${localStorage.getItem('token')}`
            },
            body: JSON.stringify({
                weapon: selectedWeapon,
                skin: selectedSkin.name,
                condition: selectedFloat,
                statTrak: selectedStatTrak,
                rarity: selectedSkin.rarity,
                image: selectedSkin.image
            })
        });
        
        if (!response.ok) {
            const error = await response.json();
            throw new Error(error.message || 'Failed to apply skin');
        }
        
        alert(`✅ Applied ${selectedSkin.name} to ${selectedWeapon}!`);
        closeSkinModal();
        
    } catch (error) {
        console.error('Error applying skin:', error);
        alert(`❌ Error: ${error.message}`);
    }
}

/**
 * Set up API token generation
 */
function setupAPIToken() {
    document.getElementById('generateTokenBtn').addEventListener('click', generateAPIToken);
    document.getElementById('copyTokenBtn').addEventListener('click', copyAPIToken);
}

/**
 * Generate API token
 */
async function generateAPIToken() {
    try {
        const response = await fetch('/api/generate-token', {
            method: 'POST',
            headers: {
                'Authorization': `Bearer ${localStorage.getItem('token')}`
            }
        });
        
        if (!response.ok) throw new Error('Failed to generate token');
        
        const data = await response.json();
        displayAPIToken(data.token);
        
    } catch (error) {
        console.error('Error generating token:', error);
        alert('Failed to generate token');
    }
}

/**
 * Display API token
 */
function displayAPIToken(token) {
    const tokenText = document.getElementById('apiTokenText');
    const tokenDisplay = document.getElementById('apiTokenDisplay');
    const copyBtn = document.getElementById('copyTokenBtn');
    
    tokenText.textContent = token;
    tokenDisplay.classList.add('visible');
    copyBtn.style.display = 'inline-block';
    
    // Store for copying
    tokenDisplay.dataset.token = token;
}

/**
 * Copy API token to clipboard
 */
function copyAPIToken() {
    const token = document.getElementById('apiTokenDisplay').dataset.token;
    navigator.clipboard.writeText(token).then(() => {
        alert('Token copied to clipboard!');
    }).catch(err => {
        console.error('Failed to copy:', err);
    });
}

/**
 * Show error state
 */
function showErrorState(message) {
    const grid = document.getElementById('inventoryGrid');
    grid.innerHTML = `<div class="loading-state"><p>❌ ${message}</p></div>`;
}

/**
 * Handle logout
 */
function logout() {
    localStorage.removeItem('token');
    window.location.href = '/';
}

// Initialize dashboard when page loads
document.addEventListener('DOMContentLoaded', initDashboard);

// Close modal on outside click
document.getElementById('skinModal')?.addEventListener('click', function(e) {
    if (e.target === this) closeSkinModal(e);
});
