// Q3GFX Modal functionality
(function() {
    'use strict';
    
    function openQ3GFXModal() {
        const q3gfxModal = document.getElementById('q3gfxModal');
        if (q3gfxModal) {
            q3gfxModal.classList.add('active');
            document.body.style.overflow = 'hidden';
            // Update URL hash
            if (history.pushState) {
                history.pushState(null, null, '#q3gfx');
            } else {
                window.location.hash = '#q3gfx';
            }
            
            // Initialize Q3GFX if not already initialized
            if (!window.q3gfxInstance) {
                initializeQ3GFX();
            }
            
            // Сбросить изменения при открытии
            if (window.ChangeTracker) {
                window.ChangeTracker.resetChanges('q3gfx');
            }
        }
    }
    
    function closeQ3GFXModalFunc() {
        doCloseQ3GFXModal();
    }
    
    function doCloseQ3GFXModal() {
        const q3gfxModal = document.getElementById('q3gfxModal');
        if (q3gfxModal) {
            q3gfxModal.classList.remove('active');
            document.body.style.overflow = '';
            // Remove URL hash
            if (history.pushState) {
                history.pushState(null, null, window.location.pathname + window.location.search);
            } else {
                window.location.hash = '';
            }
            // Сбросить изменения при закрытии
            if (window.ChangeTracker) {
                window.ChangeTracker.resetChanges('q3gfx');
            }
        }
    }
    
    function initializeQ3GFX() {
        const container = document.getElementById('q3gfxContent');
        if (!container) return;
        
        const params = {
            containerId: "q3gfxContent",
            resources: "/assets/q3gfx",
            symbolsMap: "charmap.png",
            nickname: "^b^1A^2n^3a^4r^5k^6i^7",
            width: 1000,
            height: 350,
            modes: [
                {
                    mode: "vq3",
                    maps: [
                        {image: "bg_q3tourney4.jpg", name: "Q3TOURNEY4"},
                        {image: "bg_q3dm6.jpg", name: "Q3DM6"},
                    ],
                },
                {
                    mode: "osp",
                    maps: [
                        {image: "bg_q3dm6.jpg", name: "PRO-Q3DM6"},
                        {image: "bg_p1rate-overek.jpg", name: "P1RATE-OVEREK"},
                        {image: "bg_6++.jpg", name: "6++"},
                        {image: "bg_ts_ca1.jpg", name: "TS_CA1"},
                        {image: "bg_monastery.jpg", name: "MONASTERY"},
                        {image: "bg_uberkill.jpg", name: "UBERKILL"},
                        {image: "bg_oxodm54.jpg", name: "OXODM54"},
                    ],
                    default: true
                },
                {
                    mode: "cpma",
                    maps: [
                        {image: "bg_cpm22.jpg", name: "AEROWALK"},
                        {image: "bg_cpm20.jpg", name: "CPM20"},
                    ],
                },
            ]
        };
        
        window.q3gfxInstance = Q3GFX_Initialize(params);
    }
    
    document.addEventListener('DOMContentLoaded', function() {
        const openQ3GFXBtn = document.getElementById('openQ3GFXModal');
        const q3gfxModal = document.getElementById('q3gfxModal');
        const closeQ3GFXModal = document.getElementById('closeQ3GFXModal');
        
        if (openQ3GFXBtn && q3gfxModal && closeQ3GFXModal) {
            // Open modal on section click - DISABLED
            // openQ3GFXBtn.addEventListener('click', function() {
            //     openQ3GFXModal();
            // });
            
            // Close modal
            closeQ3GFXModal.addEventListener('click', function() {
                closeQ3GFXModalFunc();
            });
            
            // Close on background click (only if both mousedown and mouseup were outside content)
            var mouseDownOnBackground = false;
            q3gfxModal.addEventListener('mousedown', function(e) {
                mouseDownOnBackground = (e.target === q3gfxModal);
            });
            var handleMouseUp = function(e) {
                if (mouseDownOnBackground) {
                    if (e.target === q3gfxModal || !q3gfxModal.contains(e.target)) {
                        closeQ3GFXModalFunc();
                    }
                }
                mouseDownOnBackground = false;
            };
            q3gfxModal.addEventListener('mouseup', handleMouseUp);
            document.addEventListener('mouseup', handleMouseUp);
            
            // Close on Escape key
            document.addEventListener('keydown', function(e) {
                if (e.key === 'Escape' && q3gfxModal.classList.contains('active')) {
                    e.preventDefault();
                    if (window.ChangeTracker) {
                        window.ChangeTracker.handleEscPress('q3gfx', function() {
                            closeQ3GFXModalFunc();
                        });
                    } else {
                        closeQ3GFXModalFunc();
                    }
                }
            });
            
            // Check URL hash on page load - DISABLED
            // if (window.location.hash === '#q3gfx') {
            //     openQ3GFXModal();
            // }
            
            // Listen for hash changes (back/forward buttons) - DISABLED
            // window.addEventListener('hashchange', function() {
            //     if (window.location.hash === '#q3gfx') {
            //         openQ3GFXModal();
            //     } else if (q3gfxModal.classList.contains('active')) {
            //         closeQ3GFXModalFunc();
            //     }
            // });
        }
    });
})();

