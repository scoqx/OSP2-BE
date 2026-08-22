class FullscreenModule {
    constructor(gallery) {
        this.gallery = gallery;
        this.isOpen = false;
        this.currentIndex = 0;
        this.imageCache = new Map(); // Кэш для предзагруженных изображений
        this.init();
    }
    
    init() {
        this.createFullscreenHTML();
        this.setupEventListeners();
        this.preloadImages();
    }
    
    preloadImages() {
        if (!this.gallery.images || this.gallery.images.length === 0) return;
        
        this.gallery.images.forEach((image, index) => {
            if (!this.imageCache.has(image.src)) {
                const img = new Image();
                img.onload = () => {
                    this.imageCache.set(image.src, true);
                };
                img.onerror = () => {
                    // Failed to preload
                };
                img.src = image.src;
            }
        });
    }
    
    createFullscreenHTML() {
        // Create fullscreen overlay if it doesn't exist
        if (!document.getElementById('fullscreenOverlay')) {
            const overlay = document.createElement('div');
            overlay.id = 'fullscreenOverlay';
            overlay.className = 'fullscreen-overlay hidden';
            overlay.innerHTML = `
                <div class="fullscreen-controls">
                    <button id="fullscreenPrev" class="fullscreen-btn">‹</button>
                    <button id="closeFullscreen" class="fullscreen-btn">×</button>
                    <button id="fullscreenNext" class="fullscreen-btn">›</button>
                </div>
                <img id="fullscreenImage" class="fullscreen-image" alt="Fullscreen image">
            `;
            document.body.appendChild(overlay);
        }
    }
    
    setupEventListeners() {
        const overlay = document.getElementById('fullscreenOverlay');
        const prevBtn = document.getElementById('fullscreenPrev');
        const nextBtn = document.getElementById('fullscreenNext');
        const closeBtn = document.getElementById('closeFullscreen');
        
        if (prevBtn) prevBtn.addEventListener('click', () => this.previousImage());
        if (nextBtn) nextBtn.addEventListener('click', () => this.nextImage());
        if (closeBtn) closeBtn.addEventListener('click', () => this.close());
        
        // Close on overlay click
        if (overlay) {
            overlay.addEventListener('click', (e) => {
                if (e.target === overlay) {
                    this.close();
                }
            });
        }
        
        // Keyboard navigation
        document.addEventListener('keydown', (e) => this.handleKeyboard(e));
    }
    
    open(imageIndex = 0) {
        this.currentIndex = imageIndex;
        this.isOpen = true;
        
        const overlay = document.getElementById('fullscreenOverlay');
        if (overlay) {
            overlay.classList.remove('hidden');
            this.renderImage();
        }
        
        // Prevent body scroll
        document.body.style.overflow = 'hidden';
    }
    
    close() {
        this.isOpen = false;
        
        const overlay = document.getElementById('fullscreenOverlay');
        if (overlay) {
            overlay.classList.add('hidden');
        }
        
        // Restore body scroll
        document.body.style.overflow = '';
        
        // Sync current index with gallery
        if (this.gallery) {
            this.gallery.currentIndex = this.currentIndex;
            this.gallery.renderMainImage();
            
            // Small delay to ensure DOM is updated before scrolling
            setTimeout(() => {
                this.gallery.updateThumbnailSelection();
            }, 50);
        }
    }
    
    renderImage() {
        if (!this.gallery.images || this.gallery.images.length === 0) return;
        
        const image = this.gallery.images[this.currentIndex];
        const fullscreenImage = document.getElementById('fullscreenImage');
        const overlay = document.getElementById('fullscreenOverlay');
        
        if (fullscreenImage && image && overlay) {
            // Check if image is already cached
            if (this.imageCache.has(image.src)) {
                // Image is preloaded, show immediately
                fullscreenImage.src = image.src;
                fullscreenImage.alt = image.title;
                fullscreenImage.classList.remove('loading');
                fullscreenImage.classList.add('loaded');
                this.hideLoadingSpinner(overlay);
                return;
            }
            
            // Clear old image immediately to prevent flicker
            fullscreenImage.src = '';
            fullscreenImage.alt = '';
            
            // Show loading state
            fullscreenImage.classList.add('loading');
            fullscreenImage.classList.remove('loaded');
            
            // Add loading spinner
            this.showLoadingSpinner(overlay);
            
            // Set up image loading
            const img = new Image();
            img.onload = () => {
                // Cache the image
                this.imageCache.set(image.src, true);
                // Only set src after image is fully loaded
                fullscreenImage.src = image.src;
                fullscreenImage.alt = image.title;
                fullscreenImage.classList.remove('loading');
                fullscreenImage.classList.add('loaded');
                this.hideLoadingSpinner(overlay);
            };
            
            img.onerror = () => {
                fullscreenImage.classList.remove('loading');
                this.hideLoadingSpinner(overlay);
                this.showImageError(overlay, image.title);
            };
            
            img.src = image.src;
        }
    }
    
    showLoadingSpinner(container) {
        if (!container) return;
        
        // Remove existing loading elements
        this.hideLoadingSpinner(container);
        
        // Create loading spinner
        const spinner = document.createElement('div');
        spinner.className = 'loading-spinner';
        spinner.id = 'fullscreenSpinner';
        spinner.style.position = 'absolute';
        spinner.style.top = '50%';
        spinner.style.left = '50%';
        spinner.style.zIndex = '1001';
        
        // Create loading text
        const loadingText = document.createElement('div');
        loadingText.className = 'loading-text';
        loadingText.id = 'fullscreenLoadingText';
        loadingText.textContent = 'Loading';
        loadingText.style.position = 'absolute';
        loadingText.style.top = '60%';
        loadingText.style.left = '50%';
        loadingText.style.transform = 'translate(-50%, -50%)';
        loadingText.style.zIndex = '1001';
        
        container.appendChild(spinner);
        container.appendChild(loadingText);
    }
    
    hideLoadingSpinner(container) {
        if (!container) return;
        
        const spinner = container.querySelector('#fullscreenSpinner');
        const loadingText = container.querySelector('#fullscreenLoadingText');
        
        if (spinner) spinner.remove();
        if (loadingText) loadingText.remove();
    }
    
    showImageError(container, imageTitle) {
        if (!container) return;
        
        const errorDiv = document.createElement('div');
        errorDiv.className = 'loading-text';
        errorDiv.style.color = '#f44336';
        errorDiv.style.position = 'absolute';
        errorDiv.style.top = '50%';
        errorDiv.style.left = '50%';
        errorDiv.style.transform = 'translate(-50%, -50%)';
        errorDiv.style.zIndex = '1001';
        errorDiv.textContent = `Failed to load: ${imageTitle}`;
        errorDiv.id = 'fullscreenError';
        
        container.appendChild(errorDiv);
        
        // Remove error after 3 seconds
        setTimeout(() => {
            if (errorDiv.parentNode) {
                errorDiv.remove();
            }
        }, 3000);
    }
    
    previousImage() {
        if (!this.gallery.images || this.gallery.images.length === 0) return;
        
        this.currentIndex = this.currentIndex > 0 ? this.currentIndex - 1 : this.gallery.images.length - 1;
        this.renderImage();
    }
    
    nextImage() {
        if (!this.gallery.images || this.gallery.images.length === 0) return;
        
        this.currentIndex = this.currentIndex < this.gallery.images.length - 1 ? this.currentIndex + 1 : 0;
        this.renderImage();
    }
    
    handleKeyboard(e) {
        if (!this.isOpen) return;
        
        switch (e.key) {
            case 'Escape':
                this.close();
                break;
            case 'ArrowLeft':
                this.previousImage();
                break;
            case 'ArrowRight':
                this.nextImage();
                break;
        }
    }
}

