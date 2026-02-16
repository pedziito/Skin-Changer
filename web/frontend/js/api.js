// API Configuration
const API_BASE = window.location.origin + '/api';

// Helper function to get token from localStorage
function getToken() {
    return localStorage.getItem('authToken');
}

// Helper function to set token
function setToken(token) {
    localStorage.setItem('authToken', token);
}

// Helper function to remove token
function removeToken() {
    localStorage.removeItem('authToken');
}

// Helper function to make API requests
async function apiRequest(endpoint, options = {}) {
    const token = getToken();
    
    const config = {
        headers: {
            'Content-Type': 'application/json',
            ...(token && { 'Authorization': `Bearer ${token}` })
        },
        ...options
    };

    try {
        const response = await fetch(`${API_BASE}${endpoint}`, config);
        const data = await response.json();

        if (!response.ok) {
            throw new Error(data.error || 'Request failed');
        }

        return data;
    } catch (error) {
        throw error;
    }
}

// Auth API calls
const AuthAPI = {
    async register(username, email, password) {
        return await apiRequest('/auth/register', {
            method: 'POST',
            body: JSON.stringify({ username, email, password })
        });
    },

    async login(username, password) {
        return await apiRequest('/auth/login', {
            method: 'POST',
            body: JSON.stringify({ username, password })
        });
    },

    async verify() {
        return await apiRequest('/auth/verify', {
            method: 'GET'
        });
    },

    async generateApiToken() {
        return await apiRequest('/auth/generate-api-token', {
            method: 'POST'
        });
    }
};

// Config API calls
const ConfigAPI = {
    async getAll() {
        return await apiRequest('/config', {
            method: 'GET'
        });
    },

    async create(config) {
        return await apiRequest('/config', {
            method: 'POST',
            body: JSON.stringify(config)
        });
    },

    async delete(id) {
        return await apiRequest(`/config/${id}`, {
            method: 'DELETE'
        });
    },

    async deleteAll() {
        return await apiRequest('/config', {
            method: 'DELETE'
        });
    }
};
