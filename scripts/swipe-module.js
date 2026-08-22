class SwipeModule {
    constructor(gallery) {
        this.gallery = gallery;
        this.startX = 0;
        this.startY = 0;
        this.endX = 0;
        this.endY = 0;
        this.minSwipeDistance = 50;
        this.maxVerticalDistance = 100;
        
        this.init();
    }
    
    init() {
        this.setupEventListeners();
    }
    
    setupEventListeners() {
        // Add touch events to main image
        const mainImage = document.getElementById('mainImage');
        if (mainImage) {
            mainImage.addEventListener('touchstart', (e) => this.handleTouchStart(e), { passive: true });
            mainImage.addEventListener('touchmove', (e) => this.handleTouchMove(e), { passive: true });
            mainImage.addEventListener('touchend', (e) => this.handleTouchEnd(e), { passive: true });
        }
        
        // Add touch events to fullscreen image
        const fullscreenImage = document.getElementById('fullscreenImage');
        if (fullscreenImage) {
            fullscreenImage.addEventListener('touchstart', (e) => this.handleTouchStart(e), { passive: true });
            fullscreenImage.addEventListener('touchmove', (e) => this.handleTouchMove(e), { passive: true });
            fullscreenImage.addEventListener('touchend', (e) => this.handleTouchEnd(e), { passive: true });
        }
    }
    
    handleTouchStart(e) {
        if (e.touches.length === 1) {
            this.startX = e.touches[0].clientX;
            this.startY = e.touches[0].clientY;
        }
    }
    
    handleTouchMove(e) {
        if (e.touches.length === 1) {
            this.endX = e.touches[0].clientX;
            this.endY = e.touches[0].clientY;
        }
    }
    
    handleTouchEnd(e) {
        if (!this.startX || !this.startY || !this.endX || !this.endY) {
            return;
        }
        
        const deltaX = this.endX - this.startX;
        const deltaY = this.endY - this.startY;
        const absDeltaX = Math.abs(deltaX);
        const absDeltaY = Math.abs(deltaY);
        
        // Check if it's a horizontal swipe (not vertical scroll)
        if (absDeltaX > this.minSwipeDistance && absDeltaX > absDeltaY && absDeltaY < this.maxVerticalDistance) {
            if (deltaX > 0) {
                // Swipe right - previous image
                this.handleSwipeRight();
            } else {
                // Swipe left - next image
                this.handleSwipeLeft();
            }
        }
        
        // Reset values
        this.startX = 0;
        this.startY = 0;
        this.endX = 0;
        this.endY = 0;
    }
    
    handleSwipeLeft() {
        // Check if we're in fullscreen mode
        const fullscreenOverlay = document.getElementById('fullscreenOverlay');
        if (fullscreenOverlay && !fullscreenOverlay.classList.contains('hidden')) {
            // In fullscreen mode, use fullscreen module
            if (this.gallery.fullscreenModule) {
                this.gallery.fullscreenModule.nextImage();
            }
        } else if (!this.gallery.isThumbnailMode) {
            // In main view mode, use gallery navigation
            this.gallery.nextImage();
        }
    }
    
    handleSwipeRight() {
        // Check if we're in fullscreen mode
        const fullscreenOverlay = document.getElementById('fullscreenOverlay');
        if (fullscreenOverlay && !fullscreenOverlay.classList.contains('hidden')) {
            // In fullscreen mode, use fullscreen module
            if (this.gallery.fullscreenModule) {
                this.gallery.fullscreenModule.previousImage();
            }
        } else if (!this.gallery.isThumbnailMode) {
            // In main view mode, use gallery navigation
            this.gallery.previousImage();
        }
    }
}

