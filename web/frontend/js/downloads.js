// Downloads Page Logic

function downloadExe() {
    // Trigger download of the exe file
    window.location.href = '/api/downloads/client';
    showDownloadMessage('Desktop client download started...');
}

function downloadWebServer() {
    // Trigger download of the web server package
    window.location.href = '/api/downloads/web-server';
    showDownloadMessage('Web server package download started...');
}

function showDownloadMessage(message) {
    // Create and show a notification
    const notification = document.createElement('div');
    notification.className = 'success-message';
    notification.textContent = message;
    notification.style.position = 'fixed';
    notification.style.top = '20px';
    notification.style.right = '20px';
    notification.style.zIndex = '10000';
    
    document.body.appendChild(notification);
    
    setTimeout(() => {
        notification.remove();
    }, 3000);
}
