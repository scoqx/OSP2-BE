class ThumbnailOptimizer {
    constructor() {
        this.thumbnailCache = new Map();
        // Адаптивные размеры для мобильных устройств
        const isMobile = this.isMobileDevice();
        this.thumbnailSizes = {
            small: isMobile ? 200 : 150,    // Увеличенные миниатюры для мобильных
            medium: isMobile ? 400 : 300,   // Улучшенные превью для мобильных
            large: isMobile ? 800 : 600     // Высокое качество для мобильных
        };
        // Качество для превью (small) - 75%, для остальных - оригиналы
        this.quality = 0.75; // 75% для превью
    }
    
    /**
     * Определяет мобильное устройство
     */
    isMobileDevice() {
        return /Android|webOS|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini/i.test(navigator.userAgent) ||
               (window.innerWidth <= 768) ||
               ('ontouchstart' in window);
    }
    
    /**
     * Обновляет настройки качества при изменении размера экрана
     */
    updateQualityForScreenSize() {
        const isMobile = this.isMobileDevice();
        // Качество остается 75% для превью
        this.quality = 0.75;
        
        // Обновляем размеры
        this.thumbnailSizes = {
            small: isMobile ? 200 : 150,
            medium: isMobile ? 400 : 300,
            large: isMobile ? 800 : 600
        };
        
    }
    
    /**
     * Проверяет, является ли браузер Firefox
     */
    isFirefox() {
        return navigator.userAgent.toLowerCase().indexOf('firefox') > -1;
    }

    /**
     * Создает оптимизированную миниатюру из исходного изображения
     * @param {string} imageSrc - Путь к исходному изображению
     * @param {string} size - Размер миниатюры ('small', 'medium', 'large')
     * @returns {Promise<string>} - Data URL оптимизированной миниатюры
     */
    async createOptimizedThumbnail(imageSrc, size = 'small') {
        const cacheKey = `${imageSrc}_${size}`;
        
        // Проверяем кэш
        if (this.thumbnailCache.has(cacheKey)) {
            return this.thumbnailCache.get(cacheKey);
        }
        
        try {
            // Для Firefox используем упрощенный подход
            if (this.isFirefox()) {
                return await this.createFirefoxThumbnail(imageSrc, size);
            }
            
            const thumbnailSize = this.thumbnailSizes[size];
            const canvas = document.createElement('canvas');
            const ctx = canvas.getContext('2d');
            
            // Загружаем изображение
            const img = await this.loadImage(imageSrc);
            
            // Вычисляем размеры с сохранением пропорций
            // Для превью (small) используем 75% от оригинального размера
            let width, height;
            if (size === 'small') {
                // Для превью берем 75% от оригинального размера
                width = Math.round(img.width * 0.75);
                height = Math.round(img.height * 0.75);
            } else {
                // Для остальных размеров используем обычное масштабирование
                const { width: calcWidth, height: calcHeight } = this.calculateDimensions(
                    img.width, 
                    img.height, 
                    thumbnailSize
                );
                width = calcWidth;
                height = calcHeight;
            }
            
            canvas.width = width;
            canvas.height = height;
            
            // Рисуем изображение на canvas
            ctx.drawImage(img, 0, 0, width, height);
            
            // Конвертируем в оптимизированный JPEG
            const dataUrl = canvas.toDataURL('image/jpeg', this.quality);
            
            // Кэшируем результат
            this.thumbnailCache.set(cacheKey, dataUrl);
            
            return dataUrl;
        } catch (error) {
            console.warn('Failed to create thumbnail for:', imageSrc, error);
            return imageSrc; // Возвращаем исходное изображение в случае ошибки
        }
    }
    
    /**
     * Загружает изображение с обработкой ошибок
     */
    loadImage(src) {
        return new Promise((resolve, reject) => {
            const img = new Image();
            img.crossOrigin = 'anonymous';
            img.onload = () => resolve(img);
            img.onerror = () => reject(new Error(`Failed to load image: ${src}`));
            img.src = src;
        });
    }
    
    /**
     * Вычисляет размеры с сохранением пропорций
     */
    calculateDimensions(originalWidth, originalHeight, maxSize) {
        const aspectRatio = originalWidth / originalHeight;
        
        let width, height;
        
        // Обычное масштабирование с сохранением пропорций
        if (aspectRatio > 1) {
            // Широкое изображение
            width = Math.min(maxSize, originalWidth);
            height = width / aspectRatio;
        } else {
            // Высокое изображение
            height = Math.min(maxSize, originalHeight);
            width = height * aspectRatio;
        }
        
        return { width: Math.round(width), height: Math.round(height) };
    }
    
    /**
     * Создает lazy loading изображение с оптимизированной миниатюрой и blur-up эффектом
     */
    createOptimizedImageElement(originalSrc, alt, size = 'small', className = '') {
        const img = document.createElement('img');
        img.alt = alt;
        img.className = className + ' loading';
        img.loading = 'lazy';
        
        // Для Firefox используем упрощенный подход
        if (this.isFirefox()) {
            // Для Firefox просто используем оригинальное изображение с параметром
            const timestamp = Date.now();
            img.src = `${originalSrc}?v=${timestamp}&firefox=1`;
            img.classList.remove('loading');
            img.classList.add('loaded');
            return img;
        }
        
        // Для превью (small) используем оптимизированные изображения
        // Для остальных размеров используем оригиналы
        if (size === 'small') {
            // Устанавливаем placeholder пока загружается оптимизированная версия
            img.src = this.createPlaceholder(originalSrc, size);
            
            // Асинхронно загружаем оптимизированную версию для превью
            this.createOptimizedThumbnail(originalSrc, size).then(optimizedSrc => {
                if (img.src === this.createPlaceholder(originalSrc, size)) {
                    img.src = optimizedSrc;
                    img.classList.remove('loading');
                    img.classList.add('loaded');
                }
            }).catch(error => {
                console.warn('Failed to create optimized thumbnail, using original:', error);
                img.src = originalSrc;
                img.classList.remove('loading');
                img.classList.add('loaded');
            });
        } else {
            // Для medium и large используем оригинальные изображения
            img.src = originalSrc;
            img.classList.remove('loading');
            img.classList.add('loaded');
        }
        
        return img;
    }
    
    /**
     * Создает placeholder изображение
     */
    createPlaceholder(originalSrc, size) {
        const thumbnailSize = this.thumbnailSizes[size];
        const canvas = document.createElement('canvas');
        const ctx = canvas.getContext('2d');
        
        canvas.width = thumbnailSize;
        canvas.height = thumbnailSize;
        
        // Создаем черный фон
        ctx.fillStyle = '#000000';
        ctx.fillRect(0, 0, thumbnailSize, thumbnailSize);
        
        // Добавляем серый текст "Loading..."
        ctx.fillStyle = '#808080';
        ctx.font = '14px Arial';
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        ctx.fillText('Loading...', thumbnailSize / 2, thumbnailSize / 2);
        
        return canvas.toDataURL();
    }
    
    /**
     * Очищает кэш миниатюр
     */
    clearCache() {
        this.thumbnailCache.clear();
    }
    
    /**
     * Получает статистику кэша
     */
    getCacheStats() {
        return {
            size: this.thumbnailCache.size,
            keys: Array.from(this.thumbnailCache.keys())
        };
    }
    
    /**
     * Создает миниатюру специально для Firefox
     * Firefox имеет проблемы с canvas и изображениями, поэтому используем упрощенный подход
     */
    async createFirefoxThumbnail(imageSrc, size) {
        const cacheKey = `${imageSrc}_${size}_firefox`;
        
        // Проверяем кэш
        if (this.thumbnailCache.has(cacheKey)) {
            return this.thumbnailCache.get(cacheKey);
        }
        
        try {
            // Для Firefox просто возвращаем оригинальное изображение
            // с добавлением параметра для принудительной перезагрузки
            const timestamp = Date.now();
            const optimizedSrc = `${imageSrc}?v=${timestamp}&firefox=1`;
            
            // Кэшируем результат
            this.thumbnailCache.set(cacheKey, optimizedSrc);
            
            return optimizedSrc;
        } catch (error) {
            console.warn('Failed to create Firefox thumbnail for:', imageSrc, error);
            return imageSrc;
        }
    }
}

// Создаем глобальный экземпляр
window.thumbnailOptimizer = new ThumbnailOptimizer();

// Обновляем качество при изменении размера экрана (поворот устройства)
let resizeTimeout;
window.addEventListener('resize', () => {
    clearTimeout(resizeTimeout);
    resizeTimeout = setTimeout(() => {
        window.thumbnailOptimizer.updateQualityForScreenSize();
    }, 300); // Debounce resize events
});
