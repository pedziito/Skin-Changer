// Admin Panel Logic

document.addEventListener('DOMContentLoaded', () => {
    const adminLoginPage = document.getElementById('adminLoginPage');
    const adminPanel = document.getElementById('adminPanel');
    const adminLoginForm = document.getElementById('adminLoginForm');
    const adminLoginError = document.getElementById('adminLoginError');

    // Check if user is already logged in as admin
    checkAdminAuth();

    // Admin login form submission
    adminLoginForm.addEventListener('submit', async (e) => {
        e.preventDefault();
        
        const username = document.getElementById('adminLoginUsername').value;
        const password = document.getElementById('adminLoginPassword').value;
        
        const submitBtn = adminLoginForm.querySelector('button[type="submit"]');
        submitBtn.disabled = true;
        submitBtn.innerHTML = '<span class="loading"></span> Signing in...';

        try {
            const response = await AuthAPI.login(username, password);
            
            // Check if user is admin
            if (!response.user.is_admin) {
                throw new Error('Admin access required');
            }
            
            setToken(response.token);
            localStorage.setItem('username', response.user.username);
            localStorage.setItem('is_admin', 'true');
            
            // Show admin panel
            adminLoginPage.style.display = 'none';
            adminPanel.style.display = 'block';
            
            // Initialize admin panel
            initAdminPanel();
        } catch (error) {
            adminLoginError.textContent = error.message || 'Invalid credentials or insufficient permissions';
            adminLoginError.classList.add('show');
            
            setTimeout(() => {
                adminLoginError.classList.remove('show');
            }, 5000);
        } finally {
            submitBtn.disabled = false;
            submitBtn.textContent = 'Admin Sign In';
        }
    });

    // Admin logout
    document.getElementById('adminLogoutBtn')?.addEventListener('click', () => {
        removeToken();
        localStorage.removeItem('username');
        localStorage.removeItem('is_admin');
        
        adminPanel.style.display = 'none';
        adminLoginPage.style.display = 'flex';
        
        adminLoginForm.reset();
    });

    // Tab switching
    document.querySelectorAll('.tab-btn').forEach(btn => {
        btn.addEventListener('click', () => {
            const tabName = btn.getAttribute('data-tab');
            
            // Update active tab button
            document.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('active'));
            btn.classList.add('active');
            
            // Update active tab content
            document.querySelectorAll('.tab-content').forEach(content => content.classList.remove('active'));
            document.getElementById(tabName + 'Tab').classList.add('active');
            
            // Load data for the selected tab
            if (tabName === 'users') {
                loadUsers();
            } else if (tabName === 'licenses') {
                loadLicenses();
            }
        });
    });

    // Generate license form toggle
    document.getElementById('showGenerateLicenseForm')?.addEventListener('click', () => {
        document.getElementById('generateLicenseForm').style.display = 'block';
    });

    document.getElementById('cancelGenerateBtn')?.addEventListener('click', () => {
        document.getElementById('generateLicenseForm').style.display = 'none';
    });

    // Generate licenses
    document.getElementById('generateLicensesBtn')?.addEventListener('click', async () => {
        await generateLicenses();
    });
});

// Check if user is authenticated as admin
async function checkAdminAuth() {
    const token = getToken();
    const isAdmin = localStorage.getItem('is_admin') === 'true';
    
    if (!token || !isAdmin) {
        document.getElementById('adminLoginPage').style.display = 'flex';
        return;
    }

    try {
        await AuthAPI.verify();
        
        // User is authenticated, show admin panel
        document.getElementById('adminLoginPage').style.display = 'none';
        document.getElementById('adminPanel').style.display = 'block';
        
        // Initialize admin panel
        initAdminPanel();
    } catch (error) {
        // Token is invalid, show login
        removeToken();
        localStorage.removeItem('is_admin');
        document.getElementById('adminLoginPage').style.display = 'flex';
    }
}

// Initialize admin panel
async function initAdminPanel() {
    const username = localStorage.getItem('username');
    document.getElementById('adminUsername').textContent = username || 'Admin';
    
    // Load dashboard stats
    await loadStats();
    
    // Load users by default
    await loadUsers();
}

// Load dashboard statistics
async function loadStats() {
    try {
        const response = await apiRequest('/admin/stats', { method: 'GET' });
        const stats = response.stats;
        
        document.getElementById('statTotalUsers').textContent = stats.total_users || 0;
        document.getElementById('statActiveUsers').textContent = stats.active_users || 0;
        document.getElementById('statTotalLicenses').textContent = stats.total_licenses || 0;
        document.getElementById('statUnusedLicenses').textContent = stats.unused_licenses || 0;
    } catch (error) {
        console.error('Failed to load stats:', error);
    }
}

// Load users
async function loadUsers() {
    try {
        const response = await apiRequest('/admin/users', { method: 'GET' });
        const users = response.users;
        
        const tbody = document.getElementById('usersTableBody');
        
        if (users.length === 0) {
            tbody.innerHTML = '<tr><td colspan="7" class="empty-state">No users found</td></tr>';
            return;
        }
        
        tbody.innerHTML = users.map(user => `
            <tr>
                <td>${user.id}</td>
                <td>
                    ${user.username}
                    ${user.is_admin ? '<span class="status-badge status-admin">ADMIN</span>' : ''}
                </td>
                <td>${user.email}</td>
                <td>${user.license_key || '<em>None</em>'}</td>
                <td>
                    <span class="status-badge ${user.is_active ? 'status-active' : 'status-inactive'}">
                        ${user.is_active ? 'Active' : 'Inactive'}
                    </span>
                </td>
                <td>${new Date(user.created_at).toLocaleDateString()}</td>
                <td>
                    ${!user.is_admin ? `
                        <button class="action-btn ${user.is_active ? 'action-btn-deactivate' : 'action-btn-activate'}" 
                                onclick="toggleUserStatus(${user.id}, ${!user.is_active})">
                            ${user.is_active ? 'Deactivate' : 'Activate'}
                        </button>
                        <button class="action-btn action-btn-delete" onclick="deleteUser(${user.id})">
                            Delete
                        </button>
                    ` : '<em>Protected</em>'}
                </td>
            </tr>
        `).join('');
    } catch (error) {
        console.error('Failed to load users:', error);
        showError('Failed to load users');
    }
}

// Load licenses
async function loadLicenses() {
    try {
        const response = await apiRequest('/admin/licenses', { method: 'GET' });
        const licenses = response.licenses;
        
        const tbody = document.getElementById('licensesTableBody');
        
        if (licenses.length === 0) {
            tbody.innerHTML = '<tr><td colspan="7" class="empty-state">No license keys found</td></tr>';
            return;
        }
        
        tbody.innerHTML = licenses.map(license => `
            <tr>
                <td>${license.id}</td>
                <td>
                    <code class="key-text">${license.key}</code>
                </td>
                <td>
                    <span class="status-badge ${license.is_used ? 'status-used' : 'status-unused'}">
                        ${license.is_used ? 'Used' : 'Available'}
                    </span>
                </td>
                <td>${license.used_by_username || '<em>None</em>'}</td>
                <td>${new Date(license.created_at).toLocaleDateString()}</td>
                <td>${license.expires_at ? new Date(license.expires_at).toLocaleDateString() : '<em>Never</em>'}</td>
                <td>
                    <button class="action-btn action-btn-copy" onclick="copyLicenseKey('${license.key}')">
                        Copy
                    </button>
                    ${!license.is_used ? `
                        <button class="action-btn action-btn-delete" onclick="deleteLicense(${license.id})">
                            Delete
                        </button>
                    ` : ''}
                </td>
            </tr>
        `).join('');
    } catch (error) {
        console.error('Failed to load licenses:', error);
        showError('Failed to load license keys');
    }
}

// Toggle user status
async function toggleUserStatus(userId, activate) {
    if (!confirm(`Are you sure you want to ${activate ? 'activate' : 'deactivate'} this user?`)) {
        return;
    }

    try {
        await apiRequest(`/admin/users/${userId}/status`, {
            method: 'PATCH',
            body: JSON.stringify({ is_active: activate })
        });
        
        showSuccess(`User ${activate ? 'activated' : 'deactivated'} successfully`);
        await loadUsers();
        await loadStats();
    } catch (error) {
        showError(error.message || 'Failed to update user status');
    }
}

// Delete user
async function deleteUser(userId) {
    if (!confirm('Are you sure you want to delete this user? This action cannot be undone.')) {
        return;
    }

    try {
        await apiRequest(`/admin/users/${userId}`, {
            method: 'DELETE'
        });
        
        showSuccess('User deleted successfully');
        await loadUsers();
        await loadStats();
    } catch (error) {
        showError(error.message || 'Failed to delete user');
    }
}

// Generate licenses
async function generateLicenses() {
    const count = parseInt(document.getElementById('licenseCount').value) || 1;
    const expiryDays = document.getElementById('licenseExpiryDays').value;
    const notes = document.getElementById('licenseNotes').value;

    try {
        const response = await apiRequest('/admin/licenses/generate', {
            method: 'POST',
            body: JSON.stringify({
                count,
                expires_days: expiryDays || null,
                notes: notes || null
            })
        });

        // Show generated keys
        const keysHtml = response.keys.map(key => `
            <div class="key-item">
                <span class="key-text">${key}</span>
                <button class="action-btn action-btn-copy" onclick="copyLicenseKey('${key}')">Copy</button>
            </div>
        `).join('');

        const modal = document.createElement('div');
        modal.className = 'modal show';
        modal.innerHTML = `
            <div class="modal-content">
                <div class="modal-header">
                    <h2>Generated License Keys</h2>
                    <button class="modal-close">&times;</button>
                </div>
                <div class="generated-keys">
                    ${keysHtml}
                </div>
                <button class="btn btn-primary" style="width: 100%; margin-top: 1rem;">Done</button>
            </div>
        `;

        document.body.appendChild(modal);

        modal.querySelector('.modal-close').addEventListener('click', () => {
            modal.remove();
        });

        modal.querySelector('.btn-primary').addEventListener('click', () => {
            modal.remove();
        });

        // Reset form
        document.getElementById('licenseCount').value = 1;
        document.getElementById('licenseExpiryDays').value = '';
        document.getElementById('licenseNotes').value = '';
        document.getElementById('generateLicenseForm').style.display = 'none';

        await loadLicenses();
        await loadStats();
    } catch (error) {
        showError(error.message || 'Failed to generate license keys');
    }
}

// Delete license
async function deleteLicense(licenseId) {
    if (!confirm('Are you sure you want to delete this license key?')) {
        return;
    }

    try {
        await apiRequest(`/admin/licenses/${licenseId}`, {
            method: 'DELETE'
        });
        
        showSuccess('License key deleted successfully');
        await loadLicenses();
        await loadStats();
    } catch (error) {
        showError(error.message || 'Failed to delete license key');
    }
}

// Copy license key to clipboard
function copyLicenseKey(key) {
    navigator.clipboard.writeText(key).then(() => {
        showSuccess('License key copied to clipboard!');
    }).catch(() => {
        showError('Failed to copy license key');
    });
}

// Show success message
function showSuccess(message) {
    const container = document.querySelector('.container');
    if (!container) return;
    
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
    if (!container) return;
    
    const errorDiv = document.createElement('div');
    errorDiv.className = 'error-message show';
    errorDiv.textContent = message;
    
    container.insertBefore(errorDiv, container.firstChild);
    
    setTimeout(() => {
        errorDiv.remove();
    }, 5000);
}
