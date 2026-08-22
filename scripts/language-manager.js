// Language Manager - handles language preference storage and switching
(function() {
    'use strict';
    
    // Save language preference when clicking on language switcher
    function saveLanguagePreference() {
        const langLinks = document.querySelectorAll('.lang-link');
        
        langLinks.forEach(link => {
            link.addEventListener('click', function(e) {
                const href = this.getAttribute('href');
                
                // Determine language from href
                let lang = 'en';
                if (href.includes('/ru/')) {
                    lang = 'ru';
                } else if (href.includes('/en/')) {
                    lang = 'en';
                }
                
                // Save to localStorage
                localStorage.setItem('preferredLanguage', lang);
            });
        });
    }
    
    // Initialize on page load
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', saveLanguagePreference);
    } else {
        saveLanguagePreference();
    }
})();

