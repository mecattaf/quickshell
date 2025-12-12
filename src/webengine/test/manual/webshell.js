/**
 * WebShell - JavaScript client for Quickshell WebChannel integration
 * 
 * Usage:
 *   initWebShell().then(shell => {
 *       shell.log('info', 'Connected!');
 *       console.log('Dark mode:', shell.darkMode);
 *   });
 */

/**
 * Initialize the WebShell connection.
 * Returns a Promise that resolves with the shell bridge object.
 */
function initWebShell() {
    return new Promise((resolve, reject) => {
        // Check if QWebChannel is available
        if (typeof QWebChannel === 'undefined') {
            reject(new Error('QWebChannel not available. Make sure qwebchannel.js is loaded.'));
            return;
        }

        // Check if the transport is available
        if (typeof qt === 'undefined' || !qt.webChannelTransport) {
            reject(new Error('qt.webChannelTransport not available. Are you running in QtWebEngine?'));
            return;
        }

        // Create the channel
        new QWebChannel(qt.webChannelTransport, function (channel) {
            const shell = channel.objects.shell;

            if (!shell) {
                reject(new Error('Shell bridge not found in channel objects.'));
                return;
            }

            // Add convenience methods
            shell.debug = function (msg) { shell.log('debug', msg); };
            shell.info = function (msg) { shell.log('info', msg); };
            shell.warn = function (msg) { shell.log('warn', msg); };
            shell.error = function (msg) { shell.log('error', msg); };

            // Wrap getThemeTokens to return a proper Promise
            const originalGetThemeTokens = shell.getThemeTokens;
            shell.getThemeTokens = function () {
                return new Promise((resolve) => {
                    originalGetThemeTokens.call(shell, resolve);
                });
            };

            resolve(shell);
        });
    });
}

/**
 * Apply theme tokens to CSS custom properties.
 * Call this after getting tokens from shell.getThemeTokens().
 * 
 * @param {Object} tokens - Theme tokens object from getThemeTokens()
 */
function applyThemeTokens(tokens) {
    const root = document.documentElement;

    // Apply colors
    if (tokens.colors) {
        for (const [key, value] of Object.entries(tokens.colors)) {
            root.style.setProperty(`--color-${key}`, value);
        }
    }

    // Apply spacing
    if (tokens.spacing) {
        for (const [key, value] of Object.entries(tokens.spacing)) {
            root.style.setProperty(`--spacing-${key}`, `${value}px`);
        }
    }

    // Apply radius
    if (tokens.radius) {
        for (const [key, value] of Object.entries(tokens.radius)) {
            root.style.setProperty(`--radius-${key}`, `${value}px`);
        }
    }

    // Apply dark mode class
    if (tokens.darkMode) {
        document.body.classList.add('dark-mode');
        document.body.classList.remove('light-mode');
    } else {
        document.body.classList.add('light-mode');
        document.body.classList.remove('dark-mode');
    }
}
