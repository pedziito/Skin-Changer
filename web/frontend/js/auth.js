// Authentication Logic
document.addEventListener('DOMContentLoaded', () => {
    const loginPage = document.getElementById('loginPage');
    const registerPage = document.getElementById('registerPage');
    const dashboard = document.getElementById('dashboard');
    
    const loginForm = document.getElementById('loginForm');
    const registerForm = document.getElementById('registerForm');
    
    const showRegisterLink = document.getElementById('showRegister');
    const showLoginLink = document.getElementById('showLogin');
    
    const loginError = document.getElementById('loginError');
    const registerError = document.getElementById('registerError');

    // Check if user is already logged in
    checkAuth();

    // Show/Hide pages
    showRegisterLink.addEventListener('click', (e) => {
        e.preventDefault();
        loginPage.style.display = 'none';
        registerPage.style.display = 'flex';
    });

    showLoginLink.addEventListener('click', (e) => {
        e.preventDefault();
        registerPage.style.display = 'none';
        loginPage.style.display = 'flex';
    });

    // Login form submission
    loginForm.addEventListener('submit', async (e) => {
        e.preventDefault();
        
        const username = document.getElementById('loginUsername').value;
        const password = document.getElementById('loginPassword').value;
        
        const submitBtn = loginForm.querySelector('button[type="submit"]');
        submitBtn.disabled = true;
        submitBtn.innerHTML = '<span class="loading"></span> Signing in...';

        try {
            const response = await AuthAPI.login(username, password);
            setToken(response.token);
            localStorage.setItem('username', response.user.username);
            
            // Show dashboard
            loginPage.style.display = 'none';
            dashboard.style.display = 'block';
            
            // Initialize dashboard
            initDashboard();
        } catch (error) {
            loginError.textContent = error.message;
            loginError.classList.add('show');
            
            setTimeout(() => {
                loginError.classList.remove('show');
            }, 5000);
        } finally {
            submitBtn.disabled = false;
            submitBtn.textContent = 'Sign In';
        }
    });

    // Register form submission
    registerForm.addEventListener('submit', async (e) => {
        e.preventDefault();
        
        const username = document.getElementById('registerUsername').value;
        const email = document.getElementById('registerEmail').value;
        const password = document.getElementById('registerPassword').value;
        const licenseKey = document.getElementById('registerLicenseKey').value;
        
        const submitBtn = registerForm.querySelector('button[type="submit"]');
        submitBtn.disabled = true;
        submitBtn.innerHTML = '<span class="loading"></span> Creating account...';

        try {
            const response = await AuthAPI.register(username, email, password, licenseKey);
            setToken(response.token);
            localStorage.setItem('username', response.user.username);
            
            // Show dashboard
            registerPage.style.display = 'none';
            dashboard.style.display = 'block';
            
            // Initialize dashboard
            initDashboard();
        } catch (error) {
            registerError.textContent = error.message;
            registerError.classList.add('show');
            
            setTimeout(() => {
                registerError.classList.remove('show');
            }, 5000);
        } finally {
            submitBtn.disabled = false;
            submitBtn.textContent = 'Create Account';
        }
    });

    // Check authentication status
    async function checkAuth() {
        const token = getToken();
        
        if (!token) {
            return;
        }

        try {
            await AuthAPI.verify();
            
            // User is authenticated, show dashboard
            loginPage.style.display = 'none';
            registerPage.style.display = 'none';
            dashboard.style.display = 'block';
            
            // Initialize dashboard
            initDashboard();
        } catch (error) {
            // Token is invalid, remove it
            removeToken();
            localStorage.removeItem('username');
        }
    }

    // Logout
    document.getElementById('logoutBtn').addEventListener('click', () => {
        removeToken();
        localStorage.removeItem('username');
        localStorage.removeItem('apiToken');
        
        dashboard.style.display = 'none';
        loginPage.style.display = 'flex';
        
        // Clear form fields
        loginForm.reset();
        registerForm.reset();
    });
});
