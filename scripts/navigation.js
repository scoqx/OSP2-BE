// Navigation management script
class NavigationManager {
    constructor() {
        this.currentPage = this.getCurrentPage();
        this.init();
    }
    
    /**
     * Определяет текущую страницу по URL
     */
    getCurrentPage() {
        const path = window.location.pathname;
        const pathSegments = path.split('/').filter(segment => segment);
        
        // Определяем страницу
        if (pathSegments.length === 0 || pathSegments.includes('index.html') || pathSegments.includes('index')) {
            return 'home';
        }
        
        const lastSegment = pathSegments[pathSegments.length - 1];
        
        if (lastSegment.includes('gallery')) {
            return 'gallery';
        } else if (lastSegment.includes('commands')) {
            return 'commands';
        } else if (lastSegment.includes('compilations')) {
            return 'compilations';
        } else if (lastSegment.includes('tools')) {
            return 'tools';
        }
        
        return 'home';
    }
    
    /**
     * Инициализация навигации
     */
    init() {
        this.updateActiveLinks();
        this.setupEventListeners();
    }
    
    /**
     * Обновляет активные ссылки в навигации
     */
    updateActiveLinks() {
        // Удаляем все активные классы
        const allNavLinks = document.querySelectorAll('.nav-link');
        allNavLinks.forEach(link => {
            link.classList.remove('active');
        });
        
        // Добавляем активный класс к соответствующей ссылке
        const activeLink = this.getActiveLink();
        if (activeLink) {
            activeLink.classList.add('active');
        }
    }
    
    /**
     * Находит ссылку, которая должна быть активной
     */
    getActiveLink() {
        const navLinks = document.querySelectorAll('.nav-link');
        
        for (const link of navLinks) {
            const href = link.getAttribute('href');
            if (!href) continue;
            
            // Проверяем соответствие текущей странице
            if (this.isLinkActive(href)) {
                return link;
            }
        }
        
        return null;
    }
    
    /**
     * Проверяет, должна ли ссылка быть активной
     */
    isLinkActive(href) {
        const currentPath = window.location.pathname;
        
        // Специальная обработка для главной страницы
        if (this.currentPage === 'home') {
            return href.includes('index.html') || href === '/' || href === '';
        }
        
        // Для других страниц проверяем соответствие
        switch (this.currentPage) {
            case 'gallery':
                return href.includes('gallery.html');
            case 'commands':
                return href.includes('commands.html');
            case 'compilations':
                return href.includes('compilations.html');
            case 'tools':
                return href.includes('tools.html');
            default:
                return false;
        }
    }
    
    /**
     * Настраивает обработчики событий
     */
    setupEventListeners() {
        // Обновляем активные ссылки при изменении URL (для SPA)
        window.addEventListener('popstate', () => {
            this.currentPage = this.getCurrentPage();
            this.updateActiveLinks();
        });
        
        // Обработчик для кликов по ссылкам навигации
        const navLinks = document.querySelectorAll('.nav-link');
        navLinks.forEach(link => {
            link.addEventListener('click', (e) => {
                // Небольшая задержка для обновления активной ссылки
                setTimeout(() => {
                    this.currentPage = this.getCurrentPage();
                    this.updateActiveLinks();
                }, 100);
            });
        });
    }
    
    /**
     * Принудительно обновляет навигацию
     */
    refresh() {
        this.currentPage = this.getCurrentPage();
        this.updateActiveLinks();
    }
}

// Инициализация при загрузке DOM
document.addEventListener('DOMContentLoaded', () => {
    window.navigationManager = new NavigationManager();
});

// Экспорт для использования в других скриптах
window.NavigationManager = NavigationManager;
