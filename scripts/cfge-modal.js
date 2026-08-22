// Config Editor Modal functionality
(function() {
    'use strict';
    
    function openConfigEditorModal() {
        const configEditorModal = document.getElementById('configEditorModal');
        if (configEditorModal) {
            configEditorModal.classList.add('active');
            document.body.style.overflow = 'hidden';
            // Update URL hash
            if (history.pushState) {
                history.pushState(null, null, '#config-editor');
            } else {
                window.location.hash = '#config-editor';
            }
            // Сбросить изменения при открытии
            if (window.ChangeTracker) {
                window.ChangeTracker.resetChanges('config-editor');
            }
        }
    }
    
    function closeConfigEditorModalFunc() {
        doCloseConfigEditorModal();
    }
    
    function doCloseConfigEditorModal() {
        const configEditorModal = document.getElementById('configEditorModal');
        if (configEditorModal) {
            configEditorModal.classList.remove('active');
            document.body.style.overflow = '';
            // Remove URL hash
            if (history.pushState) {
                history.pushState(null, null, window.location.pathname + window.location.search);
            } else {
                window.location.hash = '';
            }
            // Сбросить изменения при закрытии
            if (window.ChangeTracker) {
                window.ChangeTracker.resetChanges('config-editor');
            }
        }
    }
    
    document.addEventListener('DOMContentLoaded', function() {
        const openConfigEditorBtn = document.getElementById('openConfigEditorModal');
        const configEditorModal = document.getElementById('configEditorModal');
        const closeConfigEditorModal = document.getElementById('closeConfigEditorModal');
        
        if (openConfigEditorBtn && configEditorModal && closeConfigEditorModal) {
            // Open modal on section click
            openConfigEditorBtn.addEventListener('click', function() {
                openConfigEditorModal();
            });
            
            // Close modal
            closeConfigEditorModal.addEventListener('click', function() {
                closeConfigEditorModalFunc();
            });
            
            // Close on background click (only if both mousedown and mouseup were outside content)
            var mouseDownOnBackground = false;
            configEditorModal.addEventListener('mousedown', function(e) {
                mouseDownOnBackground = (e.target === configEditorModal);
            });
            var handleMouseUp = function(e) {
                if (mouseDownOnBackground) {
                    if (e.target === configEditorModal || !configEditorModal.contains(e.target)) {
                        closeConfigEditorModalFunc();
                    }
                }
                mouseDownOnBackground = false;
            };
            configEditorModal.addEventListener('mouseup', handleMouseUp);
            document.addEventListener('mouseup', handleMouseUp);
            
            // Close on Escape key
            document.addEventListener('keydown', function(e) {
                if (e.key === 'Escape' && configEditorModal.classList.contains('active')) {
                    e.preventDefault();
                    if (window.ChangeTracker) {
                        window.ChangeTracker.handleEscPress('config-editor', function() {
                            closeConfigEditorModalFunc();
                        });
                    } else {
                        closeConfigEditorModalFunc();
                    }
                }
            });
            
            // Check URL hash on page load
            if (window.location.hash === '#config-editor') {
                openConfigEditorModal();
            }
            
            // Listen for hash changes (back/forward buttons)
            window.addEventListener('hashchange', function() {
                if (window.location.hash === '#config-editor') {
                    openConfigEditorModal();
                } else if (configEditorModal.classList.contains('active')) {
                    closeConfigEditorModalFunc();
                }
            });
        }
    });
})();

