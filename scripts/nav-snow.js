// Легкий эффект снега для панели навигации
class NavSnow {
    constructor() {
        this.navbar = document.querySelector('.navbar');
        this.snowflakes = [];
        this.maxFlakes = 15; // Небольшое количество для легкого эффекта
        this.animationId = null;
        this.init();
    }

    init() {
        if (!this.navbar) return;

        // Создаем контейнер для снежинок внутри навигации
        this.snowContainer = document.createElement('div');
        this.snowContainer.className = 'nav-snow-container';
        this.snowContainer.style.cssText = `
            position: absolute;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            pointer-events: none;
            overflow: hidden;
            z-index: 1;
        `;
        this.navbar.appendChild(this.snowContainer);

        // Создаем снежинки
        this.createSnowflakes();
        this.animate();
    }

    createSnowflakes() {
        for (let i = 0; i < this.maxFlakes; i++) {
            this.createSnowflake();
        }
    }

    createSnowflake() {
        const snowflake = document.createElement('div');
        snowflake.className = 'nav-snowflake';
        
        // Случайный размер (очень маленький для легкого эффекта)
        const size = Math.random() * 3 + 2; // 2-5px
        snowflake.style.width = `${size}px`;
        snowflake.style.height = `${size}px`;
        
        // Начальная позиция по горизонтали (в пикселях для точности)
        const startX = Math.random() * (this.navbar.offsetWidth || window.innerWidth);
        snowflake.dataset.x = startX;
        
        // Начальная позиция по вертикали (выше видимой области)
        snowflake.dataset.y = -10;
        
        // Случайная скорость падения
        snowflake.dataset.speed = Math.random() * 0.8 + 0.4; // 0.4-1.2
        
        // Амплитуда покачивания (разные для каждой снежинки)
        snowflake.dataset.amplitude = Math.random() * 1.5 + 0.5; // 0.5-2px
        
        // Скорость покачивания (разные для каждой снежинки)
        snowflake.dataset.swingSpeed = Math.random() * 0.03 + 0.01; // 0.01-0.04
        
        // Фаза для синусоидального движения
        snowflake.dataset.phase = Math.random() * Math.PI * 2;
        
        // Прозрачность
        const opacity = Math.random() * 0.5 + 0.3; // 0.3-0.8
        snowflake.style.opacity = opacity;
        
        // Используем transform для плавной анимации
        snowflake.style.transform = `translate(${startX}px, -10px)`;
        snowflake.style.willChange = 'transform';
        
        this.snowContainer.appendChild(snowflake);
        this.snowflakes.push(snowflake);
    }

    animate() {
        const animateFrame = () => {
            if (!this.navbar || !this.snowContainer) return;
            
            const navbarHeight = this.navbar.offsetHeight;
            const navbarWidth = this.navbar.offsetWidth || window.innerWidth;
            
            this.snowflakes.forEach(snowflake => {
                if (!snowflake.parentElement) return;
                
                let y = parseFloat(snowflake.dataset.y) || -10;
                let startX = parseFloat(snowflake.dataset.x) || 0;
                const speed = parseFloat(snowflake.dataset.speed) || 0.5;
                const amplitude = parseFloat(snowflake.dataset.amplitude) || 1;
                const swingSpeed = parseFloat(snowflake.dataset.swingSpeed) || 0.02;
                let phase = parseFloat(snowflake.dataset.phase) || 0;
                
                // Движение вниз
                y += speed;
                
                // Синусоидальное покачивание влево/вправо относительно начальной позиции
                phase += swingSpeed;
                const x = startX + Math.sin(phase) * amplitude;
                
                // Если снежинка вышла за пределы навигации, перемещаем её наверх
                if (y > navbarHeight + 10) {
                    y = -10;
                    startX = Math.random() * navbarWidth;
                    phase = Math.random() * Math.PI * 2;
                    
                    // Обновляем все данные
                    snowflake.dataset.y = y;
                    snowflake.dataset.x = startX;
                    snowflake.dataset.phase = phase;
                } else {
                    // Обновляем позицию
                    snowflake.dataset.y = y;
                    snowflake.dataset.phase = phase;
                }
                
                // Применяем transform для плавной анимации
                snowflake.style.transform = `translate(${x}px, ${y}px)`;
            });
            
            this.animationId = requestAnimationFrame(animateFrame);
        };
        
        animateFrame();
    }

    destroy() {
        if (this.animationId) {
            cancelAnimationFrame(this.animationId);
        }
        if (this.snowContainer) {
            this.snowContainer.remove();
        }
    }
}

// Инициализация при загрузке DOM
document.addEventListener('DOMContentLoaded', () => {
    window.navSnow = new NavSnow();
});

// Экспорт для использования в других скриптах
window.NavSnow = NavSnow;

