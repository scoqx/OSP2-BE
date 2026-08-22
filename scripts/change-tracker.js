// Change Tracker - отслеживание изменений в инструментах
(function() {
    'use strict';
    
    // Хранилище состояния изменений для каждого инструмента
    const changeState = {
        'skybox': {
            hasChanges: false,
            isCompleted: false
        },
        'q3gfx': {
            hasChanges: false,
            isCompleted: false
        },
        'md3model': {
            hasChanges: false,
            isCompleted: false
        }
    };
    
    
    
    // Отметить изменения в инструменте
    function markChanges(toolId) {
        if (changeState[toolId]) {
            changeState[toolId].hasChanges = true;
            changeState[toolId].isCompleted = false;
            updateBeforeUnload();
        }
    }
    
    // Отметить, что работа завершена
    function markCompleted(toolId) {
        if (changeState[toolId]) {
            changeState[toolId].isCompleted = true;
            changeState[toolId].hasChanges = false;
            updateBeforeUnload();
        }
    }
    
    // Сбросить изменения
    function resetChanges(toolId) {
        if (changeState[toolId]) {
            changeState[toolId].hasChanges = false;
            changeState[toolId].isCompleted = false;
            updateBeforeUnload();
        }
    }
    
    // Проверить, есть ли изменения
    function hasChanges(toolId) {
        return changeState[toolId] ? changeState[toolId].hasChanges : false;
    }
    
    // Проверить, завершена ли работа
    function isCompleted(toolId) {
        return changeState[toolId] ? changeState[toolId].isCompleted : false;
    }
    
    // Обработка ESC для закрытия (просто закрывает окно)
    function handleEscPress(toolId, closeCallback) {
        if (!changeState[toolId]) return false;
        
        // Просто закрыть окно
        if (closeCallback) {
            closeCallback();
        }
        
        return true;
    }
    
    // Обновить обработчик beforeunload (отключено по требованию пользователя)
    function updateBeforeUnload() {
        // Убрано предупреждение при закрытии вкладки
        // Всегда удаляем обработчик, если он был установлен
        if (window._changeTrackerBeforeUnload) {
            window.removeEventListener('beforeunload', window._changeTrackerBeforeUnload);
            window._changeTrackerBeforeUnload = null;
        }
    }
    
    // Экспорт API
    window.ChangeTracker = {
        markChanges: markChanges,
        markCompleted: markCompleted,
        resetChanges: resetChanges,
        hasChanges: hasChanges,
        isCompleted: isCompleted,
        handleEscPress: handleEscPress
    };
})();

