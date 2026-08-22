// CFG Injector - extracts commands from source files and injects them into target config
(function() {
    'use strict';
    
    const sourceFilesInput = document.getElementById('sourceFiles');
    const targetFileInput = document.getElementById('targetFile');
    const processButton = document.getElementById('processButton');
    const sourceFileList = document.getElementById('sourceFileList');
    const targetFileList = document.getElementById('targetFileList');
    const statusMessage = document.getElementById('statusMessage');
    const previewContainer = document.getElementById('previewContainer');
    const previewContent = document.getElementById('previewContent');
    const previewHeaderControls = document.getElementById('previewHeaderControls');
    const sortPreviewButton = document.getElementById('sortPreviewButton');
    const hideIgnoredButton = document.getElementById('hideIgnoredButton');
    const clearButton = document.getElementById('clearConfigEditorButton');
    
    let sourceFiles = [];
    let targetFile = null;
    let excludedCommands = new Set(); // Commands excluded from processing
    let previewItems = []; // Store preview items for sorting
    let isPreviewSorted = false; // Track if preview is currently sorted
    let previewUpdateTimeout = null; // Debounce timer for preview updates
    let renderPreviewTimeout = null; // Debounce timer for render preview
    let ignoredCategories = new Set(); // Categories to ignore
    let ignoreNewCommands = false; // Ignore new commands (not in target)
    let ignoreUpdatedCommands = false; // Ignore updated commands (changed values)
    let hideIgnoredInPreview = false; // Hide ignored categories from preview (visual only)
    let previewEventDelegationSetup = false; // Track if event delegation is set up
    
    // Check if we're on the tools page
    if (!sourceFilesInput || !targetFileInput || !processButton) {
        return;
    }
    
    // Setup ignore options checkboxes
    function setupIgnoreOptions() {
        const ignoreNewCheckbox = document.getElementById('ignoreNewCommands');
        const ignoreUpdatedCheckbox = document.getElementById('ignoreUpdatedCommands');
        
        if (ignoreNewCheckbox) {
            ignoreNewCheckbox.addEventListener('change', function() {
                ignoreNewCommands = this.checked;
                // Debounce render to avoid lag
                scheduleRenderPreview();
                // Отметить изменения
                if (window.ChangeTracker) {
                    window.ChangeTracker.markChanges('config-editor');
                }
            });
        }
        
        if (ignoreUpdatedCheckbox) {
            ignoreUpdatedCheckbox.addEventListener('change', function() {
                ignoreUpdatedCommands = this.checked;
                // Debounce render to avoid lag
                scheduleRenderPreview();
                // Отметить изменения
                if (window.ChangeTracker) {
                    window.ChangeTracker.markChanges('config-editor');
                }
            });
        }
    }
    
    // Setup ignore categories checkboxes with debouncing
    function setupIgnoreCategories() {
        const checkboxes = document.querySelectorAll('.ignore-category-item input[type="checkbox"]');
        checkboxes.forEach(checkbox => {
            checkbox.addEventListener('change', function() {
                const category = this.getAttribute('data-category');
                if (this.checked) {
                    ignoredCategories.add(category);
                } else {
                    ignoredCategories.delete(category);
                }
                // Debounce render preview to avoid lag when clicking multiple checkboxes
                scheduleRenderPreview();
                // Отметить изменения
                if (window.ChangeTracker) {
                    window.ChangeTracker.markChanges('config-editor');
                }
            });
        });
    }
    
    // Schedule render preview with debounce
    function scheduleRenderPreview() {
        // Clear existing timeout
        if (renderPreviewTimeout) {
            clearTimeout(renderPreviewTimeout);
        }
        
        // Schedule render after 50ms of inactivity (fast enough for responsive UI)
        renderPreviewTimeout = setTimeout(() => {
            renderPreview();
            renderPreviewTimeout = null;
        }, 50);
    }
    
    // Determine command category
    // Returns category name if command belongs to a filterable category
    // User-defined variables (like CrouchON, speed, is_pause) return 'custom'
    function getCommandCategory(commandName, prefix) {
        // Handle bind commands (stored as bind_KEY)
        if (commandName.startsWith('bind_')) {
            return 'bind';
        }
        
        // Check for cvar prefixes (standard Quake 3 cvars)
        // Works for both "seta r_something" and "r_something 0" syntax
        if (commandName.startsWith('r_')) return 'r_';
        if (commandName.startsWith('cg_')) return 'cg_';
        if (commandName.startsWith('s_')) return 's_';
        if (commandName.startsWith('cl_')) return 'cl_';
        if (commandName.startsWith('com_')) return 'com_';
        if (commandName.startsWith('ch_')) return 'ch_';
        if (commandName.startsWith('sv_')) return 'sv_';
        if (commandName.startsWith('ui_')) return 'ui_';
        if (commandName.startsWith('x_')) return 'x_';
        if (commandName.startsWith('g_')) return 'g_'; // Server game variables
        if (commandName.startsWith('pmove_')) return 'pmove_'; // Physics engine
        
        // Check for special commands
        if (commandName === 'vstr' || commandName.startsWith('vstr_')) return 'vstr';
        
        // Check for server game commands without prefix
        const serverGameCommands = ['capturelimit', 'fraglimit', 'timelimit', 'map_restart', 'fast_restart', 'shuffle', 'reset', 'kick', 'ban', 'map', 'nextmap', 'addbot', 'removebot', 'team', 'spectator', 'callvote', 'vote'];
        if (serverGameCommands.includes(commandName.toLowerCase())) {
            return 'server_game';
        }
        
        // Check for set command (not seta)
        // This is handled separately by checking prefix
        
        // User-defined variables (CrouchON, CrouchOff, speed, is_pause, etc.)
        // These are custom variables created with seta/set that don't match standard cvar prefixes
        // If command has seta/set prefix and doesn't match any standard category, it's custom
        if (prefix === 'seta' || prefix === 'set') {
            return 'custom';
        }
        
        // Commands without prefix that don't match categories are not custom variables
        // (they might be regular commands like exec, echo, etc.)
        return null;
    }
    
    // Check if command should be ignored based on category
    function shouldIgnoreCommand(commandName, prefix) {
        // Check if it's a 'set' command (not 'seta')
        // 'set' commands create temporary variables that are reset on map load
        if (prefix === 'set' && ignoredCategories.has('set')) {
            return true;
        }
        
        // Get category from command name and prefix
        const category = getCommandCategory(commandName, prefix);
        
        // If command has no category, it's not filtered
        if (!category) {
            return false;
        }
        
        // Check if category is ignored
        if (ignoredCategories.has(category)) {
            return true;
        }
        
        return false;
    }
    
    // Initialize ignore options and categories
    setupIgnoreOptions();
    setupIgnoreCategories();
    
    // ========== Helper Functions ==========
    
    // Extract comment from line (everything after //, but not inside quotes)
    function extractComment(line) {
        let commentIndex = -1;
        let inQuotes = false;
        let quoteChar = null;
        
        for (let i = 0; i < line.length - 1; i++) {
            const char = line[i];
            const nextChar = line[i + 1];
            
            if ((char === '"' || char === "'") && (i === 0 || line[i - 1] !== '\\')) {
                if (!inQuotes) {
                    inQuotes = true;
                    quoteChar = char;
                } else if (char === quoteChar) {
                    inQuotes = false;
                    quoteChar = null;
                }
            } else if (!inQuotes && char === '/' && nextChar === '/') {
                commentIndex = i;
                break;
            }
        }
        
        return commentIndex !== -1 ? line.substring(commentIndex) : '';
    }
    
    // Get language
    function getLang() {
        return document.documentElement.lang || 'en';
    }
    
    // Translations
    const translations = {
        ru: {
            analyzing: 'Анализ...',
            error_reading_file: 'Ошибка чтения файла',
            error_reading_target: 'Ошибка чтения целевого файла',
            no_changes: 'Нет изменений для отображения',
            processing: 'Обработка...',
            select_files: 'Пожалуйста, выберите исходные файлы и целевой файл',
            file_downloaded: 'Файл {name} успешно загружен!',
            restore_change: 'Вернуть изменение',
            exclude_change: 'Отменить изменение',
            move_up: 'Вверх',
            move_down: 'Вниз',
            remove: 'Удалить',
            sort: 'Сортировать',
            unsort: 'Отменить сортировку',
            hide_ignored: 'Скрыть игнорируемые',
            show_ignored: 'Показать игнорируемые',
            confirm_clear: 'Вы уверены?'
        },
        en: {
            analyzing: 'Analyzing...',
            error_reading_file: 'Error reading file',
            error_reading_target: 'Error reading target file',
            no_changes: 'No changes to display',
            processing: 'Processing...',
            select_files: 'Please select source files and target file',
            file_downloaded: 'File {name} downloaded successfully!',
            restore_change: 'Restore change',
            exclude_change: 'Exclude change',
            move_up: 'Move up',
            move_down: 'Move down',
            remove: 'Remove',
            sort: 'Sort',
            unsort: 'Unsort',
            hide_ignored: 'Hide ignored',
            show_ignored: 'Show ignored',
            confirm_clear: 'Are you sure?'
        }
    };
    
    function t(key, params = {}) {
        const lang = getLang();
        let text = translations[lang]?.[key] || translations.en[key] || key;
        // Replace placeholders like {name}
        Object.entries(params).forEach(([k, v]) => {
            text = text.replace(`{${k}}`, v);
        });
        return text;
    }
    
    // Create DataTransfer from files
    function createDataTransferFromFiles(files) {
        const dataTransfer = new DataTransfer();
        files.forEach(file => dataTransfer.items.add(file));
        return dataTransfer;
    }
    
    // Update file input with files
    function updateFileInput(input, files) {
        input.files = createDataTransferFromFiles(files).files;
    }
    
    // Read and extract commands from files
    async function readAndExtractCommands(files) {
        const allCommands = new Map();
        for (const file of files) {
            try {
                const content = await readFileAsText(file);
                const commands = extractCommandsRaw(content);
                for (const [name, cmd] of commands) {
                    allCommands.set(name, cmd);
                }
            } catch (error) {
                console.error(`Error reading file ${file.name}:`, error);
                throw error;
            }
        }
        return allCommands;
    }
    
    // Replace value in existing command line, preserving formatting
    function replaceValueInLine(originalLine, newValue, newHasQuotes, commandName) {
        // Extract comment first
        const comment = extractComment(originalLine);
        const lineWithoutComment = comment ? originalLine.substring(0, originalLine.length - comment.length) : originalLine;
        
        // Preserve leading whitespace from original line
        const leadingWhitespaceMatch = originalLine.match(/^[\s\t]*/);
        const leadingWhitespace = leadingWhitespaceMatch ? leadingWhitespaceMatch[0] : '';
        
        // Handle bind commands (stored as bind_KEY, but displayed as "bind KEY VALUE")
        if (commandName && commandName.startsWith('bind_')) {
            // Parse "bind KEY OLDVALUE" format
            const trimmedLine = lineWithoutComment.trim();
            const bindMatch = trimmedLine.match(/^bind\s+("([^"]*)"|'([^']*)'|(\S+))\s+(.+)$/);
            if (bindMatch) {
                const keyPattern = bindMatch[1]; // The key as it appears in the line (may be quoted)
                const oldValuePart = bindMatch[5].trim(); // Everything after key is the old value
                
                // Find where key ends in the original line (preserving whitespace)
                const bindIndex = lineWithoutComment.toLowerCase().indexOf('bind');
                if (bindIndex >= 0) {
                    const afterBind = lineWithoutComment.substring(bindIndex + 4); // After "bind"
                    const keyMatch = afterBind.match(/^\s*("([^"]*)"|'([^']*)'|(\S+))/);
                    if (keyMatch) {
                        const keyEndIndex = bindIndex + 4 + keyMatch.index + keyMatch[0].length;
                        const afterKey = lineWithoutComment.substring(keyEndIndex);
                        
                        // Find where old value ends (whitespace after value or comment)
                        let valueEndIndex = afterKey.length;
                        const valueMatch = afterKey.trim().match(/^(\S+|"([^"]*)"|'([^']*)')/);
                        if (valueMatch) {
                            const valueInLine = afterKey.substring(0, afterKey.trimStart().indexOf(valueMatch[0]) + valueMatch[0].length);
                            valueEndIndex = valueInLine.length;
                        }
                        
                        // Extract whitespace before value
                        const beforeValue = afterKey.substring(0, afterKey.length - afterKey.trimStart().length);
                        const afterValue = afterKey.substring(valueEndIndex);
                        
                        // Format new value
                        const needsQuotes = newHasQuotes !== undefined ? newHasQuotes : (newValue.includes(' ') || newValue === '');
                        const valueStr = needsQuotes ? `"${newValue}"` : newValue;
                        
                        // Reconstruct: everything before key + key + new value + after value
                        const beforeBind = lineWithoutComment.substring(0, bindIndex + 4);
                        const keyPart = lineWithoutComment.substring(bindIndex + 4, keyEndIndex);
                        const newLine = beforeBind + keyPart + beforeValue + valueStr + afterValue;
                        const lineWithComment = comment ? newLine + comment : newLine;
                        
                        return leadingWhitespace + lineWithComment;
                    }
                }
            }
        }
        
        // Find the command pattern to locate where the value starts
        // We need to find: prefix (optional) + command name + whitespace + old value
        const prefixMatch = lineWithoutComment.match(/^(seta|set)\s+/);
        const prefix = prefixMatch ? prefixMatch[1] : '';
        const commandPart = prefix ? lineWithoutComment.substring(prefixMatch[0].length) : lineWithoutComment;
        
        // Find command name (first word, may be directly followed by quote/value without space)
        let nameMatch = commandPart.match(/^(\S+)[\s\t]+/);
        let parsedCommandName, afterCommandName;
        
        if (!nameMatch) {
            // Try to match command name directly followed by quote or value (no space)
            const noSpaceMatch = commandPart.match(/^(\S+?)(["']|$)/);
            if (noSpaceMatch && noSpaceMatch[1]) {
                parsedCommandName = noSpaceMatch[1];
                afterCommandName = commandPart.substring(parsedCommandName.length);
            } else {
                // If we can't parse, fall back to reconstruction
                const cmdObj = { value: newValue, hasQuotes: newHasQuotes, prefix: prefix };
                return reconstructCommandLine(
                    cmdObj,
                    commandName || commandPart.trim(),
                    leadingWhitespace,
                    comment
                );
            }
        } else {
            parsedCommandName = nameMatch[1];
            afterCommandName = commandPart.substring(nameMatch[0].length);
        }
        
        // Find where the old value starts and ends
        let oldValueStart = 0;
        let oldValueEnd = afterCommandName.length;
        
        // Check if value is quoted
        const quotedMatch = afterCommandName.match(/^[\s\t]*("([^"]*)"|'([^']*)')/);
        if (quotedMatch) {
            oldValueStart = (afterCommandName.match(/^[\s\t]*/) || [''])[0].length;
            oldValueEnd = oldValueStart + quotedMatch[0].trim().length;
        } else {
            // Unquoted value - find first non-whitespace sequence
            const valueMatch = afterCommandName.match(/^[\s\t]*(\S+)/);
            if (valueMatch) {
                oldValueStart = (afterCommandName.match(/^[\s\t]*/) || [''])[0].length;
                oldValueEnd = oldValueStart + valueMatch[1].length;
            }
        }
        
        // Extract whitespace before and after value
        let beforeValue = afterCommandName.substring(0, oldValueStart);
        const afterValue = afterCommandName.substring(oldValueEnd);
        
        // Ensure there's at least one space before value (fix cases where there was no space)
        if (!beforeValue || beforeValue.trim().length === 0) {
            beforeValue = ' ';
        }
        
        // Format new value
        const needsQuotes = newHasQuotes !== undefined ? newHasQuotes : (newValue.includes(' ') || newValue === '');
        const valueStr = needsQuotes ? `"${newValue}"` : newValue;
        
        // Reconstruct line with new value
        const prefixStr = prefix ? prefix + ' ' : '';
        const newLine = prefixStr + parsedCommandName + beforeValue + valueStr + afterValue;
        const lineWithComment = comment ? newLine + comment : newLine;
        
        return leadingWhitespace + lineWithComment;
    }
    
    // Reconstruct command line from command object
    function reconstructCommandLine(cmd, commandName, leadingWhitespace = '', comment = '') {
        // Check if this is a bind command (stored as bind_KEY)
        let displayCommandName = commandName;
        let keyPart = '';
        if (commandName.startsWith('bind_')) {
            displayCommandName = 'bind';
            keyPart = commandName.substring(5); // Remove "bind_" prefix
        }
        
        // Commands that should not have seta/set prefix
        const noPrefixCommands = ['bind', 'exec', 'vstr', 'unbind', 'unbindall', 'cvar_restart', 'clear', 'echo'];
        const isNoPrefixCommand = noPrefixCommands.includes(displayCommandName);
        
        // Use prefix from command, but don't add seta for special commands
        let prefix = cmd.prefix || '';
        if (!prefix && !isNoPrefixCommand) {
            prefix = 'seta';
        }
        const prefixStr = prefix ? prefix + ' ' : '';
        
        // Determine if value should have quotes
        const valueAlreadyQuoted = (cmd.value.length > 0 && 
            ((cmd.value[0] === '"' && cmd.value[cmd.value.length - 1] === '"') ||
             (cmd.value[0] === "'" && cmd.value[cmd.value.length - 1] === "'")));
        const needsQuotes = valueAlreadyQuoted 
            ? false 
            : (cmd.hasQuotes !== undefined 
                ? cmd.hasQuotes 
                : (cmd.value.includes(' ') || cmd.value.includes(';') || cmd.value === ''));
        const valueStr = needsQuotes ? `"${cmd.value}"` : cmd.value;
        
        // For bind commands, reconstruct as "bind KEY VALUE"
        let commandPart = '';
        if (displayCommandName === 'bind' && keyPart) {
            const keyWasQuoted = cmd.bindKeyWasQuoted !== undefined ? cmd.bindKeyWasQuoted : false;
            const keyStr = keyWasQuoted ? `"${keyPart}"` : keyPart;
            commandPart = `${displayCommandName} ${keyStr} ${valueStr}`;
        } else {
            // Always add space between command name and value
            commandPart = `${displayCommandName} ${valueStr}`;
        }
        
        return `${leadingWhitespace}${prefixStr}${commandPart}${comment ? ' ' + comment : ''}`;
    }
    
    // ========== End Helper Functions ==========
    
    // Update file list display
    function updateSourceFileList() {
        const sourceDropZone = document.getElementById('sourceDropZone');
        if (sourceFiles.length === 0) {
            sourceFileList.innerHTML = '';
            if (sourceFileList) {
                sourceFileList.classList.add('hidden');
            }
            if (sourceDropZone) {
                sourceDropZone.classList.remove('hidden');
            }
            return;
        }
        
        // Hide drop zone when files are loaded
        if (sourceDropZone) {
            sourceDropZone.classList.add('hidden');
        }
        
        // Show file list when files are loaded
        if (sourceFileList) {
            sourceFileList.classList.remove('hidden');
        }
        
        const upText = '↑';
        const downText = '↓';
        const removeText = '×';
        const showControls = sourceFiles.length > 1; // Only show controls if more than 1 file
        
        sourceFileList.innerHTML = sourceFiles.map((file, index) => {
            const canMoveUp = index > 0;
            const canMoveDown = index < sourceFiles.length - 1;
            
            return `<div class="file-item" data-index="${index}">
                <span class="file-item-name">${escapeHtml(file.name)}</span>
                <div class="file-item-controls">
                    ${showControls ? `
                        <button class="file-move-btn" data-action="up" data-index="${index}" ${!canMoveUp ? 'disabled' : ''} title="${t('move_up')}">${upText}</button>
                        <button class="file-move-btn" data-action="down" data-index="${index}" ${!canMoveDown ? 'disabled' : ''} title="${t('move_down')}">${downText}</button>
                    ` : ''}
                    <button class="file-remove-btn" data-action="remove" data-index="${index}" title="${t('remove')}">${removeText}</button>
                </div>
            </div>`;
        }).join('');
        
        // Attach event listeners to control buttons
        const controlButtons = sourceFileList.querySelectorAll('.file-move-btn, .file-remove-btn');
        controlButtons.forEach(button => {
            button.addEventListener('click', function(e) {
                e.preventDefault();
                e.stopPropagation();
                const index = parseInt(this.getAttribute('data-index'));
                const action = this.getAttribute('data-action');
                
                if (action === 'remove') {
                    // Remove file from array
                    sourceFiles.splice(index, 1);
                    // Update file input
                    if (sourceFiles.length > 0) {
                        updateFileInput(sourceFilesInput, sourceFiles);
                    } else {
                        // Clear input
                        sourceFilesInput.value = '';
                    }
                    // Update UI
                    updateSourceFileList();
                    updateProcessButton();
                    // Reset preview
                    setPreviewVisibility(false);
                    if (previewContent) {
                        previewContent.innerHTML = '';
                    }
                    excludedCommands.clear();
                } else if (action === 'up' && index > 0) {
                    // Move file up
                    [sourceFiles[index - 1], sourceFiles[index]] = [sourceFiles[index], sourceFiles[index - 1]];
                    updateSourceFileList();
                    // Debounce preview update to avoid flickering
                    schedulePreviewUpdate();
                } else if (action === 'down' && index < sourceFiles.length - 1) {
                    // Move file down
                    [sourceFiles[index], sourceFiles[index + 1]] = [sourceFiles[index + 1], sourceFiles[index]];
                    updateSourceFileList();
                    // Debounce preview update to avoid flickering
                    schedulePreviewUpdate();
                }
            });
        });
    }
    
    function updateTargetFileList() {
        const targetDropZone = document.getElementById('targetDropZone');
        if (!targetFile) {
            targetFileList.innerHTML = '';
            if (targetFileList) {
                targetFileList.classList.add('hidden');
            }
            if (targetDropZone) {
                targetDropZone.classList.remove('hidden');
            }
            return;
        }
        
        // Hide drop zone when file is loaded
        if (targetDropZone) {
            targetDropZone.classList.add('hidden');
        }
        
        // Show file list when file is loaded
        if (targetFileList) {
            targetFileList.classList.remove('hidden');
        }
        
        const removeText = '×';
        
        targetFileList.innerHTML = `<div class="file-item">
            <span class="file-item-name">${escapeHtml(targetFile.name)}</span>
            <div class="file-item-controls">
                <button class="file-remove-btn" data-action="remove" title="${t('remove')}">${removeText}</button>
            </div>
        </div>`;
        
        // Attach event listener to remove button
        const removeButton = targetFileList.querySelector('.file-remove-btn');
        if (removeButton) {
            removeButton.addEventListener('click', function(e) {
                e.preventDefault();
                e.stopPropagation();
                
                // Clear target file
                targetFile = null;
                targetFileInput.value = '';
                
                // Update UI
                updateTargetFileList();
                updateProcessButton();
                // Reset preview
                setPreviewVisibility(false);
                if (previewContent) {
                    previewContent.innerHTML = '';
                }
                excludedCommands.clear();
            });
        }
    }
    
    // Schedule preview update with debounce to avoid flickering when moving files
    function schedulePreviewUpdate() {
        // Clear existing timeout
        if (previewUpdateTimeout) {
            clearTimeout(previewUpdateTimeout);
        }
        
        // Schedule update after 300ms of inactivity
        previewUpdateTimeout = setTimeout(() => {
            updatePreview();
            previewUpdateTimeout = null;
        }, 300);
    }
    
    // Handle source files selection
    sourceFilesInput.addEventListener('change', function(e) {
        sourceFiles = Array.from(e.target.files);
        excludedCommands.clear(); // Reset excluded commands when files change
        updateSourceFileList();
        updateProcessButton();
        // Reset preview before updating
        setPreviewVisibility(false);
        if (previewContent) {
            previewContent.innerHTML = '';
        }
        // Update preview after a short delay to ensure files are ready
        setTimeout(() => {
            updatePreview();
        }, 100);
        
        // Отметить изменения
        if (window.ChangeTracker && sourceFiles.length > 0) {
            window.ChangeTracker.markChanges('config-editor');
        }
        
        // Categories will be updated after preview is loaded in updatePreview()
    });
    
    // Handle target file selection
    targetFileInput.addEventListener('change', function(e) {
        if (e.target.files.length > 0) {
            targetFile = e.target.files[0];
        } else {
            targetFile = null;
        }
        excludedCommands.clear(); // Reset excluded commands when files change
        updateTargetFileList();
        updateProcessButton();
        // Reset preview before updating
        setPreviewVisibility(false);
        if (previewContent) {
            previewContent.innerHTML = '';
        }
        // Update preview after a short delay to ensure files are ready
        setTimeout(() => {
            updatePreview();
        }, 100);
        
        // Отметить изменения
        if (window.ChangeTracker && targetFile) {
            window.ChangeTracker.markChanges('config-editor');
        }
        
        // Categories will be updated after preview is loaded in updatePreview()
    });
    
    // Toggle command exclusion
    function toggleCommandExclusion(commandName) {
        if (excludedCommands.has(commandName)) {
            excludedCommands.delete(commandName);
        } else {
            excludedCommands.add(commandName);
        }
        
        // Отметить изменения
        if (window.ChangeTracker) {
            window.ChangeTracker.markChanges('config-editor');
        }
        
        // Update only the specific item instead of refreshing entire preview
        const escapedCommandName = commandName.replace(/"/g, '&quot;').replace(/'/g, '&#39;');
        const button = previewContent.querySelector(`button[data-command="${escapedCommandName}"]`);
        if (button) {
            const item = button.closest('.preview-item');
            if (item) {
                const isExcluded = excludedCommands.has(commandName);
                
                // Find the item in previewItems to check if it's also ignored
                const previewItem = previewItems.find(p => p.name === commandName);
                const itemPrefix = previewItem ? (previewItem.prefix !== undefined ? previewItem.prefix : '') : '';
                const isIgnored = previewItem ? shouldIgnoreCommand(commandName, itemPrefix) : false;
                
                const buttonClass = isExcluded ? 'preview-toggle-btn excluded' : 'preview-toggle-btn';
                const buttonText = isExcluded ? '+' : '×';
                // Item is excluded if manually excluded OR ignored by category
                const itemClass = (isExcluded || isIgnored) ? 'preview-item excluded' : 'preview-item';
                
                // Update button
                button.className = buttonClass;
                button.textContent = buttonText;
                button.title = isExcluded ? t('restore_change') : t('exclude_change');
                
                // Update item class
                item.className = itemClass;
            }
        } else {
            // Fallback: update entire preview if button not found
            scheduleRenderPreview();
        }
    }
    
    // Update process button state
    function updateProcessButton() {
        processButton.disabled = sourceFiles.length === 0 || !targetFile;
        
        const hasFiles = sourceFiles.length > 0 && targetFile;
        
        // Show/hide clear button
        if (clearButton) {
            if (hasFiles) {
                clearButton.style.display = 'block';
            } else {
                clearButton.style.display = 'none';
            }
        }
        
        // Show/hide filter section
        const filterSection = document.querySelector('.filter-section');
        if (filterSection) {
            if (hasFiles) {
                filterSection.classList.add('visible');
                // Categories will be updated after preview is loaded in updatePreview()
            } else {
                filterSection.classList.remove('visible');
                // Hide all category items when no files
                const categoryItems = document.querySelectorAll('.ignore-category-item');
                categoryItems.forEach(item => {
                    item.style.display = 'none';
                });
            }
        }
    }
    
    // Parse command from a line
    function parseCommand(line) {
        // Store original line for comment extraction
        const originalLine = line;
        
        // Remove comments (everything after //)
        const comment = extractComment(line);
        if (comment) {
            line = line.substring(0, line.length - comment.length);
        }
        
        // Trim whitespace
        line = line.trim();
        
        // Skip empty lines
        if (!line) {
            return null;
        }
        
        // Match command patterns:
        // seta command "value"
        // seta command value
        // command "value"
        // command value
        // set command "value"
        // set command value
        // bind key "value"
        // bind key value
        // vstr variable
        // exec filename
        
        // Extract prefix (seta/set)
        const prefixMatch = line.match(/^(seta|set)\s+/);
        const prefix = prefixMatch ? prefixMatch[1] : '';
        const commandPart = prefix ? line.substring(prefixMatch[0].length) : line;
        
        // Find command name (first word, can be tab-separated or directly followed by quote/value)
        // Handle cases like: "seta name"value"" or "seta name value"
        let nameMatch = commandPart.match(/^(\S+)[\s\t]+/);
        let commandName, commandValue;
        
        if (!nameMatch) {
            // Try to match command name directly followed by quote or value (no space)
            // Example: seta name"value" or seta namevalue
            const noSpaceMatch = commandPart.match(/^(\S+?)(["']|$)/);
            if (noSpaceMatch && noSpaceMatch[1]) {
                commandName = noSpaceMatch[1];
                commandValue = commandPart.substring(commandName.length);
            } else {
                // Try to extract first word as command name
                const firstWordMatch = commandPart.match(/^(\S+)/);
                if (firstWordMatch) {
                    commandName = firstWordMatch[1];
                    commandValue = commandPart.substring(commandName.length);
                } else {
                    return null;
                }
            }
        } else {
            commandName = nameMatch[1];
            commandValue = commandPart.substring(nameMatch[0].length).trim();
        }
        
        // Clean up commandValue - remove leading whitespace if any
        commandValue = commandValue.trim();
        
        // Handle quoted values - need to handle nested quotes properly
        // Also handle cases where value might be duplicated (take only first occurrence)
        let hasQuotes = false;
        if (commandValue.length > 0) {
            // Check if value starts with quotes
            const firstChar = commandValue[0];
            
            if (firstChar === '"' || firstChar === "'") {
                // Find matching closing quote
                let quoteEnd = -1;
                for (let i = 1; i < commandValue.length; i++) {
                    if (commandValue[i] === firstChar && commandValue[i - 1] !== '\\') {
                        quoteEnd = i;
                        break;
                    }
                }
                
                if (quoteEnd !== -1) {
                    // Extract only the first quoted value (ignore duplicates)
                    commandValue = commandValue.substring(0, quoteEnd + 1);
                    // Remove outer quotes
                    commandValue = commandValue.slice(1, -1);
                    hasQuotes = true;
                } else {
                    // No closing quote found - treat as unquoted
                    // Take only first word if there are multiple values
                    const firstValueMatch = commandValue.match(/^([^\s]+)/);
                    if (firstValueMatch) {
                        commandValue = firstValueMatch[1];
                    }
                }
            } else {
                // Unquoted value - take only first word if there are multiple values
                const firstValueMatch = commandValue.match(/^([^\s]+)/);
                if (firstValueMatch) {
                    commandValue = firstValueMatch[1];
                }
            }
        }
        
        // Special handling for bind command - key is part of command name
        // bind KEY VALUE should be stored as "bind_KEY" to avoid conflicts
        let finalCommandName = commandName;
        if (commandName === 'bind' && commandValue.length > 0) {
            // Extract the key (first part of value, before space)
            // Handle both quoted and unquoted keys
            let key = '';
            let keyLength = 0;
            let keyWasQuoted = false;
            
            // Trim leading whitespace
            commandValue = commandValue.trim();
            
            // Check if key starts with quotes (but not if the key itself IS a quote)
            const firstChar = commandValue[0];
            const secondChar = commandValue.length > 1 ? commandValue[1] : '';
            
            // Key is quoted if: first char is quote AND second char is not space AND there's a matching quote
            if ((firstChar === '"' || firstChar === "'") && secondChar !== ' ' && secondChar !== '\t') {
                // Find matching quote
                let quoteEnd = -1;
                for (let i = 1; i < commandValue.length; i++) {
                    if (commandValue[i] === firstChar && commandValue[i - 1] !== '\\') {
                        quoteEnd = i;
                        break;
                    }
                }
                if (quoteEnd !== -1 && quoteEnd > 1) {
                    // Extract key without quotes
                    key = commandValue.substring(1, quoteEnd);
                    keyLength = quoteEnd + 1;
                    keyWasQuoted = true;
                } else {
                    // No closing quote or key is just a quote character, treat as unquoted
                    const keyMatch = commandValue.match(/^([^\s]+)/);
                    if (keyMatch) {
                        key = keyMatch[1];
                        keyLength = keyMatch[0].length;
                    }
                }
            } else {
                // Unquoted key - extract first word (up to space or end)
                const keyMatch = commandValue.match(/^([^\s]+)/);
                if (keyMatch) {
                    key = keyMatch[1];
                    keyLength = keyMatch[0].length;
                }
            }
            
            if (key) {
                finalCommandName = `bind_${key}`;
                // Remove key from value and trim
                let remainingValue = commandValue.substring(keyLength).trim();
                
                // Handle case where value might be duplicated (e.g., "+forward" "+forward")
                // Take only the first value
                if (remainingValue.length > 0) {
                    // Check if remaining value starts with quotes
                    const remFirstChar = remainingValue[0];
                    if (remFirstChar === '"' || remFirstChar === "'") {
                        // Find matching quote
                        let remQuoteEnd = -1;
                        for (let i = 1; i < remainingValue.length; i++) {
                            if (remainingValue[i] === remFirstChar && remainingValue[i - 1] !== '\\') {
                                remQuoteEnd = i;
                                break;
                            }
                        }
                        if (remQuoteEnd !== -1) {
                            // Extract only the first quoted value
                            remainingValue = remainingValue.substring(0, remQuoteEnd + 1);
                        }
                    } else {
                        // Unquoted value - take first word only
                        const firstValueMatch = remainingValue.match(/^([^\s]+)/);
                        if (firstValueMatch) {
                            remainingValue = firstValueMatch[1];
                        }
                    }
                }
                
                commandValue = remainingValue;
                // Update hasQuotes based on final value
                if (commandValue.length > 0) {
                    const valFirst = commandValue[0];
                    const valLast = commandValue[commandValue.length - 1];
                    hasQuotes = (valFirst === '"' && valLast === '"') || (valFirst === "'" && valLast === "'");
                    if (hasQuotes) {
                        commandValue = commandValue.slice(1, -1);
                    }
                }
                
                // Store if key was quoted for later reconstruction
                return {
                    name: finalCommandName,
                    value: commandValue,
                    hasQuotes: hasQuotes,
                    prefix: prefix,
                    originalLine: originalLine,
                    originalCommandName: commandName,
                    bindKeyWasQuoted: keyWasQuoted
                };
            }
        }
        
        return {
            name: finalCommandName,
            value: commandValue,
            hasQuotes: hasQuotes,
            prefix: prefix,
            originalLine: originalLine,
            originalCommandName: commandName // Keep original for reconstruction
        };
    }
    
    // Extract commands from file content (without filtering by ignored categories)
    function extractCommandsRaw(content) {
        const lines = content.split('\n');
        const commands = new Map();
        
        for (const line of lines) {
            const command = parseCommand(line);
            if (command) {
                // If command already exists, keep the last one
                commands.set(command.name, {
                    value: command.value,
                    hasQuotes: command.hasQuotes,
                    prefix: command.prefix,
                    originalCommandName: command.originalCommandName,
                    bindKeyWasQuoted: command.bindKeyWasQuoted
                });
            }
        }
        
        return commands;
    }
    
    // Extract commands from file content
    function extractCommands(content) {
        const lines = content.split('\n');
        const commands = new Map();
        
        for (const line of lines) {
            const command = parseCommand(line);
            if (command) {
                // Check if command should be ignored based on category
                if (shouldIgnoreCommand(command.name, command.prefix)) {
                    continue;
                }
                
                // If command already exists, keep the last one
                commands.set(command.name, {
                    value: command.value,
                    hasQuotes: command.hasQuotes,
                    prefix: command.prefix,
                    originalCommandName: command.originalCommandName,
                    bindKeyWasQuoted: command.bindKeyWasQuoted
                });
            }
        }
        
        return commands;
    }
    
    // Update visible category checkboxes based on commands that have changes
    function updateVisibleCategories() {
        if (sourceFiles.length === 0 || !targetFile) {
            // Hide all checkboxes if no files
            const categoryItems = document.querySelectorAll('.ignore-category-item');
            categoryItems.forEach(item => {
                item.style.display = 'none';
            });
            return;
        }
        
        try {
            // Collect categories only from commands that have changes (in previewItems)
            // This shows categories only when they are actually being changed
            const foundCategories = new Set();
            
            // Analyze previewItems to find categories that have changes
            // Cache category lookups for better performance
            for (const item of previewItems) {
                const commandName = item.name;
                const prefix = item.prefix || '';
                
                // Get category from command name and prefix
                const category = getCommandCategory(commandName, prefix);
                if (category) {
                    foundCategories.add(category);
                }
                
                // Check for 'set' prefix (not 'seta')
                if (prefix === 'set') {
                    foundCategories.add('set');
                }
            }
            
            // Batch DOM updates using requestAnimationFrame for smoother performance
            requestAnimationFrame(() => {
                const categoryItems = document.querySelectorAll('.ignore-category-item');
                categoryItems.forEach(item => {
                    const checkbox = item.querySelector('input[type="checkbox"]');
                    if (checkbox) {
                        const category = checkbox.getAttribute('data-category');
                        if (foundCategories.has(category)) {
                            item.style.display = 'flex';
                        } else {
                            item.style.display = 'none';
                            // Uncheck and remove from ignored if not found
                            checkbox.checked = false;
                            ignoredCategories.delete(category);
                        }
                    }
                });
            });
        } catch (error) {
            console.error('Error updating visible categories:', error);
        }
    }
    
    // Read file as text
    function readFileAsText(file) {
        return new Promise((resolve, reject) => {
            const reader = new FileReader();
            reader.onload = (e) => resolve(e.target.result);
            reader.onerror = (e) => reject(e);
            reader.readAsText(file);
        });
    }
    
    // Escape HTML
    function escapeHtml(text) {
        const div = document.createElement('div');
        div.textContent = text;
        return div.innerHTML;
    }
    
    // Helper function to show/hide preview header and container
    function setPreviewVisibility(visible) {
        if (previewHeaderControls) {
            previewHeaderControls.style.display = visible ? 'flex' : 'none';
        }
        if (previewContainer) {
            previewContainer.style.display = visible ? 'block' : 'none';
        }
        // Update hide ignored button text when preview becomes visible
        if (visible && hideIgnoredButton) {
            hideIgnoredButton.textContent = hideIgnoredInPreview ? t('show_ignored') : t('hide_ignored');
        }
    }
    
    // Update preview
    async function updatePreview() {
        if (!previewContainer || !previewContent) {
            return;
        }
        
        // Check if files are still selected
        if (sourceFiles.length === 0 || !targetFile) {
            setPreviewVisibility(false);
            previewContent.innerHTML = '';
            return;
        }
        
        // Verify files are still valid
        if (!sourceFilesInput.files || sourceFilesInput.files.length === 0) {
            setPreviewVisibility(false);
            previewContent.innerHTML = '';
            return;
        }
        
        if (!targetFileInput.files || targetFileInput.files.length === 0) {
            setPreviewVisibility(false);
            previewContent.innerHTML = '';
            return;
        }
        
        try {
            // Keep preview visible if it was already shown (for smooth updates)
            const wasHidden = previewContainer.style.display === 'none' || previewContainer.style.display === '';
            if (wasHidden) {
                setPreviewVisibility(true);
                previewContent.innerHTML = `<div style="color: var(--text-color); opacity: 0.7;">${t('analyzing')}</div>`;
            }
            // If preview was already visible, don't show "Analyzing..." - just update content smoothly
            
            // Read all source files and extract commands (raw - without filtering)
            let allCommands;
            try {
                allCommands = await readAndExtractCommands(sourceFiles);
                } catch (error) {
                previewContent.innerHTML = `<div style="color: #ff6666;">${t('error_reading_file')}: ${error.message}</div>`;
                    return;
            }
            
            // Read target file - use current file from input
            const currentTargetFile = targetFileInput.files[0];
            if (!currentTargetFile) {
                setPreviewVisibility(false);
                previewContent.innerHTML = '';
                return;
            }
            
            let targetContent;
            try {
                targetContent = await readFileAsText(currentTargetFile);
            } catch (error) {
                console.error('Error reading target file:', error);
                previewContent.innerHTML = `<div style="color: #ff6666;">${t('error_reading_target')}</div>`;
                return;
            }
            
            // Parse target file
            const targetLines = targetContent.split('\n');
            const targetCommands = new Map();
            
            for (const line of targetLines) {
                const command = parseCommand(line);
                if (command) {
                    targetCommands.set(command.name, {
                        value: command.value,
                        originalLine: line
                    });
                }
            }
            
            // Helper function to convert internal command name to display name
            function getDisplayName(internalName, cmd) {
                if (internalName.startsWith('bind_')) {
                    const key = internalName.substring(5); // Remove "bind_" prefix
                    const keyWasQuoted = cmd && cmd.bindKeyWasQuoted !== undefined ? cmd.bindKeyWasQuoted : false;
                    const keyStr = keyWasQuoted ? `"${key}"` : key;
                    return `bind ${keyStr}`;
                }
                return internalName;
            }
            
            // Build preview
            previewItems = [];
            const addedCommands = new Set();
            
            // Show updated commands (including ignored ones - they will be strikethrough)
            for (const [name, sourceCmd] of allCommands) {
                // Check if command should be ignored based on category
                const isIgnoredByCategory = shouldIgnoreCommand(name, sourceCmd.prefix);
                
                if (targetCommands.has(name)) {
                    const targetCmd = targetCommands.get(name);
                    if (targetCmd.value !== sourceCmd.value) {
                        // Check if this item should be ignored by filter (updated commands)
                        const isIgnoredByFilter = ignoreUpdatedCommands;
                        // Item is ignored if ignored by category OR by filter
                        const isIgnored = isIgnoredByCategory || isIgnoredByFilter;
                        
                        previewItems.push({
                            type: 'updated',
                            name: name,
                            displayName: getDisplayName(name, sourceCmd),
                            oldValue: targetCmd.value,
                            newValue: sourceCmd.value,
                            prefix: sourceCmd.prefix,
                            isIgnored: isIgnored
                        });
                        addedCommands.add(name);
                    }
                } else {
                    // Check if this item should be ignored by filter (new commands)
                    const isIgnoredByFilter = ignoreNewCommands;
                    // Item is ignored if ignored by category OR by filter
                    const isIgnored = isIgnoredByCategory || isIgnoredByFilter;
                    
                    previewItems.push({
                        type: 'added',
                        name: name,
                        displayName: getDisplayName(name, sourceCmd),
                        value: sourceCmd.value,
                        prefix: sourceCmd.prefix,
                        isIgnored: isIgnored
                    });
                    addedCommands.add(name);
                }
            }
            
            // Reset sort state when preview is updated
            isPreviewSorted = false;
            
            // Render preview first
            renderPreview();
            
            // Update visible categories based on previewItems
            updateVisibleCategories();
            
        } catch (error) {
            console.error('Error updating preview:', error);
            previewContent.innerHTML = `<div style="color: #ff6666;">Error: ${error.message}</div>`;
        }
    }
    
    // Setup event delegation for preview buttons (only once)
    function setupPreviewEventDelegation() {
        if (previewEventDelegationSetup || !previewContent) {
            return;
        }
        
        // Use event delegation on previewContent container
        previewContent.addEventListener('click', function(e) {
            const button = e.target.closest('.preview-toggle-btn');
            if (button) {
                e.preventDefault();
                e.stopPropagation();
                const commandName = button.getAttribute('data-command');
                if (commandName) {
                    // Decode HTML entities
                    const decodedName = commandName
                        .replace(/&quot;/g, '"')
                        .replace(/&#39;/g, "'")
                        .replace(/&amp;/g, '&');
                    toggleCommandExclusion(decodedName);
                }
            }
        });
        
        previewEventDelegationSetup = true;
    }
    
    // Render preview items
    function renderPreview() {
        if (!previewContent) {
            return;
        }
        
        // Setup event delegation once
        setupPreviewEventDelegation();
        
        if (previewItems.length === 0) {
            previewContent.innerHTML = `<div style="color: var(--text-color); opacity: 0.7;">${t('no_changes')}</div>`;
            return;
        }
        
        // Use sorted items if sorted, otherwise use original order
        const itemsToRender = isPreviewSorted ? [...previewItems].sort((a, b) => {
            // Extract command name ignoring set/seta prefix
            const getSortName = (displayName) => {
                // Remove set/seta prefix if present
                let name = displayName.trim();
                if (name.startsWith('seta ')) {
                    name = name.substring(5).trim();
                } else if (name.startsWith('set ')) {
                    name = name.substring(4).trim();
                }
                // For bind commands, extract the key part and prepend "bind" for sorting
                if (name.startsWith('bind ')) {
                    let key = name.substring(5).trim();
                    // Remove quotes if present
                    if ((key.startsWith('"') && key.endsWith('"')) || 
                        (key.startsWith("'") && key.endsWith("'"))) {
                        key = key.slice(1, -1);
                    }
                    // Sort by "bind" + key to keep bind commands together but sorted by key
                    return ('bind ' + key).toLowerCase();
                }
                // Extract first word (command name or cvar name)
                const firstSpace = name.indexOf(' ');
                if (firstSpace > 0) {
                    name = name.substring(0, firstSpace);
                }
                return name.toLowerCase();
            };
            
            const nameA = getSortName(a.displayName);
            const nameB = getSortName(b.displayName);
            return nameA.localeCompare(nameB);
        }) : previewItems;
        
        // Recalculate isIgnored for each item based on current filter and category settings
        // Filters now work like categories - they mark items as ignored (strikethrough)
        const filteredItems = itemsToRender.map(item => {
            // Get prefix from item (may be undefined, which is fine)
            const itemPrefix = item.prefix !== undefined ? item.prefix : '';
            
            // Check if item should be ignored by category
            const isIgnoredByCategory = shouldIgnoreCommand(item.name, itemPrefix);
            
            // Check if item should be ignored by filter
            const isIgnoredByFilter = (ignoreNewCommands && item.type === 'added') || 
                                     (ignoreUpdatedCommands && item.type === 'updated');
            
            // Item is ignored if ignored by category OR by filter
            const isIgnored = isIgnoredByCategory || isIgnoredByFilter;
            
            // Return item with updated isIgnored flag
            return {
                ...item,
                isIgnored: isIgnored
            };
        }).filter(item => {
            // Hide ignored items (categories or filters) from preview if option is enabled (visual only)
            if (hideIgnoredInPreview && item.isIgnored) {
                return false; // Hide ignored items visually
            }
            return true;
        });
        
        // Use DocumentFragment for faster DOM updates
        const fragment = document.createDocumentFragment();
        const tempDiv = document.createElement('div');
        
        const html = filteredItems.map(item => {
                const isExcluded = excludedCommands.has(item.name);
                const isIgnored = item.isIgnored || false;
                const buttonClass = isExcluded ? 'preview-toggle-btn excluded' : 'preview-toggle-btn';
                const buttonText = isExcluded ? '+' : '×';
                // Item is excluded if manually excluded OR ignored by category
                const itemClass = (isExcluded || isIgnored) ? 'preview-item excluded' : 'preview-item';
                
                // Store original command name in data attribute (will be decoded when reading)
                const commandNameForData = item.name.replace(/"/g, '&quot;').replace(/'/g, '&#39;');
                
                const buttonTitle = isExcluded ? t('restore_change') : t('exclude_change');
                if (item.type === 'updated') {
                    return `<div class="${itemClass}">
                        <button class="${buttonClass}" data-command="${commandNameForData}" title="${buttonTitle}">${buttonText}</button>
                        <span class="preview-command">${escapeHtml(item.displayName)}</span>
                        <span class="preview-arrow">→</span>
                        <span class="preview-old">${escapeHtml(item.oldValue)}</span>
                        <span class="preview-arrow">→</span>
                        <span class="preview-new">${escapeHtml(item.newValue)}</span>
                    </div>`;
                } else if (item.type === 'added') {
                    return `<div class="${itemClass}">
                        <button class="${buttonClass}" data-command="${commandNameForData}" title="${buttonTitle}">${buttonText}</button>
                        <span class="preview-added">+ ${escapeHtml(item.displayName)} = ${escapeHtml(item.value)}</span>
                    </div>`;
                }
            }).join('');
        
        // Batch DOM update - set innerHTML once
        previewContent.innerHTML = html;
    }
    
    // Sort preview items
    function sortPreview() {
        isPreviewSorted = !isPreviewSorted;
        scheduleRenderPreview(); // Debounce render for smoother performance
        
        // Update button text
        if (sortPreviewButton) {
            sortPreviewButton.textContent = isPreviewSorted ? t('unsort') : t('sort');
        }
    }
    
    // Process files
    async function processFiles() {
        if (sourceFiles.length === 0 || !targetFile) {
            showStatus('error', t('select_files'));
            return;
        }
        
        try {
            processButton.disabled = true;
            showStatus('', t('processing'));
            
            // Read all source files and extract commands (raw - will be filtered later)
            let allCommands;
            try {
                allCommands = await readAndExtractCommands(sourceFiles);
                } catch (error) {
                showStatus('error', `${t('error_reading_file')}: ${error.message}`);
                    processButton.disabled = false;
                    return;
            }
            
            // Read target file
            let targetContent;
            try {
                targetContent = await readFileAsText(targetFile);
            } catch (error) {
                console.error('Error reading target file:', error);
                showStatus('error', t('error_reading_target'));
                processButton.disabled = false;
                return;
            }
            
            // Parse target file to preserve structure
            const targetLines = targetContent.split('\n');
            const targetCommands = new Map();
            const targetStructure = [];
            
            // First pass: extract existing commands from target
            for (let i = 0; i < targetLines.length; i++) {
                const line = targetLines[i];
                const command = parseCommand(line);
                
                if (command) {
                    targetCommands.set(command.name, {
                        value: command.value,
                        hasQuotes: command.hasQuotes,
                        prefix: command.prefix,
                        originalCommandName: command.originalCommandName,
                        bindKeyWasQuoted: command.bindKeyWasQuoted,
                        lineIndex: i,
                        originalLine: line
                    });
                }
                
                targetStructure.push({
                    line: line,
                    isCommand: !!command,
                    commandName: command ? command.name : null
                });
            }
            
            // Merge commands: source commands override target commands (excluding excluded ones)
            for (const [name, sourceCmd] of allCommands) {
                if (excludedCommands.has(name)) {
                    continue; // Skip excluded commands (manually excluded)
                }
                
                // Check if command should be ignored based on category
                // Categories mark items as ignored but don't skip processing (unless manually excluded)
                if (shouldIgnoreCommand(name, sourceCmd.prefix)) {
                    continue; // Skip ignored category commands
                }
                
                const isNewCommand = !targetCommands.has(name);
                const isUpdatedCommand = targetCommands.has(name) && 
                    targetCommands.get(name).value !== sourceCmd.value;
                
                // Skip new commands if filter is enabled (like categories, filters skip during processing)
                if (ignoreNewCommands && isNewCommand) {
                    continue;
                }
                
                // Skip updated commands if filter is enabled (like categories, filters skip during processing)
                if (ignoreUpdatedCommands && isUpdatedCommand) {
                    continue;
                }
                
                if (targetCommands.has(name)) {
                    // Update existing command
                    const targetCmd = targetCommands.get(name);
                    targetCmd.value = sourceCmd.value;
                    targetCmd.hasQuotes = sourceCmd.hasQuotes;
                    targetCmd.prefix = sourceCmd.prefix;
                    targetCmd.originalCommandName = sourceCmd.originalCommandName;
                    targetCmd.bindKeyWasQuoted = sourceCmd.bindKeyWasQuoted;
                } else {
                    // Add new command
                    targetCommands.set(name, {
                        value: sourceCmd.value,
                        hasQuotes: sourceCmd.hasQuotes,
                        prefix: sourceCmd.prefix,
                        originalCommandName: sourceCmd.originalCommandName,
                        bindKeyWasQuoted: sourceCmd.bindKeyWasQuoted,
                        lineIndex: -1,
                        originalLine: null
                    });
                }
            }
            
            // Build output content with grouping by prefix
            const outputLines = [];
            const addedCommands = new Set();
            
            // Track last position of each category/prefix for grouping new commands
            const lastCategoryPosition = new Map(); // category -> last line index
            
            // First, process existing structure - modify values in place and track category positions
            for (let i = 0; i < targetStructure.length; i++) {
                const item = targetStructure[i];
                if (item.isCommand && item.commandName) {
                    const cmd = targetCommands.get(item.commandName);
                    if (cmd && cmd.lineIndex !== undefined && cmd.lineIndex >= 0) {
                        // Command exists in target - replace value in place, preserving formatting
                        const originalLine = item.line;
                        const newLine = replaceValueInLine(originalLine, cmd.value, cmd.hasQuotes, item.commandName);
                        outputLines.push(newLine);
                        addedCommands.add(item.commandName);
                        
                        // Track category position for grouping new commands
                        const category = getCommandCategory(item.commandName, cmd.prefix);
                        if (category) {
                            lastCategoryPosition.set(category, outputLines.length - 1);
                        }
                    } else if (!cmd) {
                        // Command was removed or ignored - keep original line as is
                        outputLines.push(item.line);
                    } else {
                        // Command was not in target originally (shouldn't happen here, but fallback)
                        outputLines.push(item.line);
                    }
                } else {
                    // Keep non-command lines as is
                    outputLines.push(item.line);
                }
            }
            
            // Group new commands by category and insert them after the last command of the same category
            const newCommandsByCategory = new Map(); // category -> array of [name, cmd]
            const newCommandsWithoutCategory = []; // commands without category
            
            // First, collect new commands and group them by category
            for (const [name, cmd] of targetCommands) {
                if (!addedCommands.has(name)) {
                    const category = getCommandCategory(name, cmd.prefix);
                    if (category) {
                        if (!newCommandsByCategory.has(category)) {
                            newCommandsByCategory.set(category, []);
                        }
                        newCommandsByCategory.get(category).push([name, cmd]);
                    } else {
                        newCommandsWithoutCategory.push([name, cmd]);
                    }
                }
            }
            
            // Sort new commands within each category alphabetically
            for (const [category, commands] of newCommandsByCategory) {
                commands.sort((a, b) => {
                    const nameA = a[0].toLowerCase();
                    const nameB = b[0].toLowerCase();
                    return nameA.localeCompare(nameB);
                });
            }
            
            // Sort commands without category
            newCommandsWithoutCategory.sort((a, b) => {
                const nameA = a[0].toLowerCase();
                const nameB = b[0].toLowerCase();
                return nameA.localeCompare(nameB);
            });
            
            // Build insertion map: line index -> array of commands to insert after that line
            const insertions = new Map(); // line index -> array of command lines
            
            // Insert commands after their category's last position
            for (const [category, commands] of newCommandsByCategory) {
                const insertAfterIndex = lastCategoryPosition.has(category) 
                    ? lastCategoryPosition.get(category) 
                    : -1; // -1 means insert at end
                
                const commandLines = commands.map(([name, cmd]) => 
                    reconstructCommandLine(cmd, name)
                );
                
                if (!insertions.has(insertAfterIndex)) {
                    insertions.set(insertAfterIndex, []);
                }
                insertions.get(insertAfterIndex).push(...commandLines);
            }
            
            // Add commands without category at the end
            if (newCommandsWithoutCategory.length > 0) {
                const commandLines = newCommandsWithoutCategory.map(([name, cmd]) => 
                    reconstructCommandLine(cmd, name)
                );
                if (!insertions.has(-1)) {
                    insertions.set(-1, []);
                }
                insertions.get(-1).push(...commandLines);
            }
            
            // Build final output with insertions
            const finalOutput = [];
            for (let i = 0; i < outputLines.length; i++) {
                finalOutput.push(outputLines[i]);
                
                // Insert new commands after this line if there are any
                if (insertions.has(i)) {
                    const commandsToInsert = insertions.get(i);
                    finalOutput.push(...commandsToInsert);
                }
            }
            
            // Add remaining commands that should go at the end (index -1)
            if (insertions.has(-1)) {
                const commandsToInsert = insertions.get(-1);
                finalOutput.push(...commandsToInsert);
            }
            
            // Create output file
            const outputContent = finalOutput.join('\n');
            const targetFileName = targetFile.name;
            const baseName = targetFileName.replace(/\.(cfg|txt)$/i, '');
            const extension = targetFileName.match(/\.(cfg|txt)$/i)?.[1] || 'cfg';
            const outputFileName = `${baseName}.${extension}`;
            
            // Create zip archive
            const zip = new JSZip();
            zip.file(outputFileName, outputContent);
            
            // Generate zip file
            const zipBlob = await zip.generateAsync({ type: 'blob' });
            const zipFileName = `${baseName}.zip`;
            
            // Download zip file
            const url = URL.createObjectURL(zipBlob);
            const a = document.createElement('a');
            a.href = url;
            a.download = zipFileName;
            document.body.appendChild(a);
            a.click();
            document.body.removeChild(a);
            URL.revokeObjectURL(url);
            
            showStatus('success', t('file_downloaded', { name: zipFileName }));
            processButton.disabled = false;
            
            // Отметить, что работа завершена
            if (window.ChangeTracker) {
                window.ChangeTracker.markCompleted('config-editor');
            }
            
        } catch (error) {
            console.error('Error processing files:', error);
            showStatus('error', `Error: ${error.message}`);
            processButton.disabled = false;
        }
    }
    
    // Show status message
    function showStatus(type, message) {
        statusMessage.className = `status-message ${type}`;
        statusMessage.textContent = message;
        
        if (type === 'success' || type === 'error') {
            setTimeout(() => {
                statusMessage.className = 'status-message';
                statusMessage.textContent = '';
            }, 5000);
        }
    }
    
    // Track which file selector area has focus
    let focusedArea = null; // 'source', 'target', or null
    let hoveredArea = null; // 'source', 'target', or null - tracks mouse hover
    
    // Handle paste event for file insertion (Ctrl+V)
    function handlePaste(e) {
        // Check if modal is open (check if configInjectorModal exists and is active)
        const configInjectorModal = document.getElementById('configInjectorModal');
        if (!configInjectorModal || !configInjectorModal.classList.contains('active')) {
            return; // Don't handle paste if modal is not open
        }
        
        // Check if clipboard contains files
        const items = e.clipboardData?.items;
        if (!items) {
            return;
        }
        
        const files = [];
        for (let i = 0; i < items.length; i++) {
            const item = items[i];
            if (item.kind === 'file') {
                const file = item.getAsFile();
                // Check if file has .cfg or .txt extension
                if (file && /\.(cfg|txt)$/i.test(file.name)) {
                    files.push(file);
                }
            }
        }
        
        if (files.length === 0) {
            return;
        }
        
        // Prevent default paste behavior
        e.preventDefault();
        
        // Determine which input field to use based on hover, focus, or active element
        const activeElement = document.activeElement;
        let targetArea = hoveredArea || focusedArea; // Prefer hover over focus
        
        // If no hover or focus tracked, try to detect from active element
        if (!targetArea) {
            const sourceDropZone = document.getElementById('sourceDropZone');
            const targetDropZone = document.getElementById('targetDropZone');
            
            // Find the file selector containers
            const sourceSelector = sourceFilesInput.closest('.file-selector');
            const targetSelector = targetFileInput.closest('.file-selector');
            
            // Check if active element is a drop zone
            if (activeElement === sourceDropZone || (sourceDropZone && sourceDropZone.contains(activeElement))) {
                targetArea = 'source';
            }
            else if (activeElement === targetDropZone || (targetDropZone && targetDropZone.contains(activeElement))) {
                targetArea = 'target';
            }
            // Check if active element is within source selector
            else if (sourceSelector && sourceSelector.contains(activeElement)) {
                targetArea = 'source';
            }
            // Check if active element is within target selector
            else if (targetSelector && targetSelector.contains(activeElement)) {
                targetArea = 'target';
            }
            // Check if input elements themselves are focused
            else if (activeElement === sourceFilesInput || 
                     sourceFileList && sourceFileList.contains(activeElement)) {
                targetArea = 'source';
            }
            else if (activeElement === targetFileInput ||
                     targetFileList && targetFileList.contains(activeElement)) {
                targetArea = 'target';
            }
        }
        
        // Insert files into the appropriate input
        if (targetArea === 'source') {
            // Add files to source files (multiple files allowed)
            const existingFiles = Array.from(sourceFiles);
            const newFiles = files.filter(file => 
                !existingFiles.some(existing => existing.name === file.name && existing.size === file.size)
            );
            
            if (newFiles.length > 0) {
                const allFiles = [...existingFiles, ...newFiles];
                updateFileInput(sourceFilesInput, allFiles);
                sourceFiles = Array.from(sourceFilesInput.files);
                sourceFilesInput.dispatchEvent(new Event('change', { bubbles: true }));
            }
        } else if (targetArea === 'target') {
            // Set target file (only one file allowed, use the first one)
            if (files.length > 0) {
                updateFileInput(targetFileInput, [files[0]]);
                targetFile = targetFileInput.files[0] || null;
                targetFileInput.dispatchEvent(new Event('change', { bubbles: true }));
            }
        } else {
            // Default: if no specific area is focused, use source files area
            const existingFiles = Array.from(sourceFiles);
            const newFiles = files.filter(file => 
                !existingFiles.some(existing => existing.name === file.name && existing.size === file.size)
            );
            
            if (newFiles.length > 0) {
                const allFiles = [...existingFiles, ...newFiles];
                updateFileInput(sourceFilesInput, allFiles);
                sourceFiles = Array.from(sourceFilesInput.files);
                sourceFilesInput.dispatchEvent(new Event('change', { bubbles: true }));
            }
        }
    }
    
    // Add paste event listener to document
    document.addEventListener('paste', handlePaste);
    
    // Add focus and hover tracking to file selector areas
    function setupFocusTracking() {
        const sourceDropZone = document.getElementById('sourceDropZone');
        const targetDropZone = document.getElementById('targetDropZone');
        
        // Track focus and hover on source files area
        const sourceSelector = sourceFilesInput.closest('.file-selector');
        if (sourceSelector) {
            sourceSelector.addEventListener('click', function() {
                focusedArea = 'source';
            });
            sourceSelector.addEventListener('focusin', function() {
                focusedArea = 'source';
            });
            sourceSelector.addEventListener('mouseenter', function() {
                hoveredArea = 'source';
            });
            sourceSelector.addEventListener('mouseleave', function() {
                hoveredArea = null;
            });
        }
        
        // Track focus and hover on source drop zone
        if (sourceDropZone) {
            sourceDropZone.addEventListener('click', function() {
                focusedArea = 'source';
            });
            sourceDropZone.addEventListener('focus', function() {
                focusedArea = 'source';
            });
            sourceDropZone.addEventListener('mouseenter', function() {
                hoveredArea = 'source';
            });
            sourceDropZone.addEventListener('mouseleave', function() {
                hoveredArea = null;
            });
        }
        
        // Track focus on source input and file list
        if (sourceFilesInput) {
            sourceFilesInput.addEventListener('focus', function() {
                focusedArea = 'source';
            });
            sourceFilesInput.addEventListener('click', function() {
                focusedArea = 'source';
            });
        }
        
        if (sourceFileList) {
            sourceFileList.addEventListener('focus', function() {
                focusedArea = 'source';
            });
            sourceFileList.addEventListener('click', function() {
                focusedArea = 'source';
            });
            sourceFileList.addEventListener('mouseenter', function() {
                hoveredArea = 'source';
            });
            sourceFileList.addEventListener('mouseleave', function() {
                hoveredArea = null;
            });
        }
        
        // Track focus and hover on target file area
        const targetSelector = targetFileInput.closest('.file-selector');
        if (targetSelector) {
            targetSelector.addEventListener('click', function() {
                focusedArea = 'target';
            });
            targetSelector.addEventListener('focusin', function() {
                focusedArea = 'target';
            });
            targetSelector.addEventListener('mouseenter', function() {
                hoveredArea = 'target';
            });
            targetSelector.addEventListener('mouseleave', function() {
                hoveredArea = null;
            });
        }
        
        // Track focus and hover on target drop zone
        if (targetDropZone) {
            targetDropZone.addEventListener('click', function() {
                focusedArea = 'target';
            });
            targetDropZone.addEventListener('focus', function() {
                focusedArea = 'target';
            });
            targetDropZone.addEventListener('mouseenter', function() {
                hoveredArea = 'target';
            });
            targetDropZone.addEventListener('mouseleave', function() {
                hoveredArea = null;
            });
        }
        
        // Track focus on target input and file list
        if (targetFileInput) {
            targetFileInput.addEventListener('focus', function() {
                focusedArea = 'target';
            });
            targetFileInput.addEventListener('click', function() {
                focusedArea = 'target';
            });
        }
        
        if (targetFileList) {
            targetFileList.addEventListener('focus', function() {
                focusedArea = 'target';
            });
            targetFileList.addEventListener('click', function() {
                focusedArea = 'target';
            });
            targetFileList.addEventListener('mouseenter', function() {
                hoveredArea = 'target';
            });
            targetFileList.addEventListener('mouseleave', function() {
                hoveredArea = null;
            });
        }
    }
    
    // Setup drag-and-drop zones and click handlers
    function setupDropZones() {
        const sourceDropZone = document.getElementById('sourceDropZone');
        const targetDropZone = document.getElementById('targetDropZone');
        
        // Setup source drop zone
        if (sourceDropZone) {
            // Click to select files
            sourceDropZone.addEventListener('click', function(e) {
                e.preventDefault();
                e.stopPropagation();
                focusedArea = 'source';
                sourceFilesInput.click();
            });
            
            // Drag and drop handlers
            sourceDropZone.addEventListener('dragenter', function(e) {
                e.preventDefault();
                e.stopPropagation();
                focusedArea = 'source';
                this.classList.add('drag-over');
            });
            
            sourceDropZone.addEventListener('dragover', function(e) {
                e.preventDefault();
                e.stopPropagation();
                this.classList.add('drag-over');
            });
            
            sourceDropZone.addEventListener('dragleave', function(e) {
                e.preventDefault();
                e.stopPropagation();
                // Only remove drag-over if we're leaving the drop zone itself
                if (!this.contains(e.relatedTarget)) {
                    this.classList.remove('drag-over');
                }
            });
            
            sourceDropZone.addEventListener('drop', function(e) {
                e.preventDefault();
                e.stopPropagation();
                this.classList.remove('drag-over');
                focusedArea = 'source';
                
                const files = Array.from(e.dataTransfer.files).filter(file => 
                    /\.(cfg|txt)$/i.test(file.name)
                );
                
                if (files.length > 0) {
                    const existingFiles = Array.from(sourceFiles);
                    const newFiles = files.filter(file => 
                        !existingFiles.some(existing => existing.name === file.name && existing.size === file.size)
                    );
                    
                    if (newFiles.length > 0) {
                        const allFiles = [...existingFiles, ...newFiles];
                        updateFileInput(sourceFilesInput, allFiles);
                        sourceFiles = Array.from(sourceFilesInput.files);
                        sourceFilesInput.dispatchEvent(new Event('change', { bubbles: true }));
                    }
                }
            });
            
            // Focus tracking for paste
            sourceDropZone.addEventListener('focus', function() {
                focusedArea = 'source';
            });
            
            sourceDropZone.setAttribute('tabindex', '0'); // Make it focusable for paste
        }
        
        // Setup target drop zone
        if (targetDropZone) {
            // Click to select file
            targetDropZone.addEventListener('click', function(e) {
                e.preventDefault();
                e.stopPropagation();
                focusedArea = 'target';
                targetFileInput.click();
            });
            
            // Drag and drop handlers
            targetDropZone.addEventListener('dragenter', function(e) {
                e.preventDefault();
                e.stopPropagation();
                focusedArea = 'target';
                this.classList.add('drag-over');
            });
            
            targetDropZone.addEventListener('dragover', function(e) {
                e.preventDefault();
                e.stopPropagation();
                this.classList.add('drag-over');
            });
            
            targetDropZone.addEventListener('dragleave', function(e) {
                e.preventDefault();
                e.stopPropagation();
                // Only remove drag-over if we're leaving the drop zone itself
                if (!this.contains(e.relatedTarget)) {
                    this.classList.remove('drag-over');
                }
            });
            
            targetDropZone.addEventListener('drop', function(e) {
                e.preventDefault();
                e.stopPropagation();
                this.classList.remove('drag-over');
                focusedArea = 'target';
                
                const files = Array.from(e.dataTransfer.files).filter(file => 
                    /\.(cfg|txt)$/i.test(file.name)
                );
                
                if (files.length > 0) {
                    updateFileInput(targetFileInput, [files[0]]);
                    targetFile = targetFileInput.files[0] || null;
                    targetFileInput.dispatchEvent(new Event('change', { bubbles: true }));
                }
            });
            
            // Focus tracking for paste
            targetDropZone.addEventListener('focus', function() {
                focusedArea = 'target';
            });
            
            targetDropZone.setAttribute('tabindex', '0'); // Make it focusable for paste
        }
    }
    
    // Setup focus tracking
    setupFocusTracking();
    
    // Setup drop zones
    setupDropZones();
    
    // Функция очистки всех данных
    function clearAll() {
        // Очистить файлы
        sourceFiles = [];
        targetFile = null;
        sourceFilesInput.value = '';
        targetFileInput.value = '';
        
        // Очистить исключенные команды
        excludedCommands.clear();
        
        // Сбросить фильтры
        ignoreNewCommands = false;
        ignoreUpdatedCommands = false;
        hideIgnoredInPreview = false;
        
        // Сбросить категории
        ignoredCategories.clear();
        
        // Сбросить превью
        previewItems = [];
        isPreviewSorted = false;
        
        // Обновить UI
        updateSourceFileList();
        updateTargetFileList();
        updateProcessButton();
        setPreviewVisibility(false);
        if (previewContent) {
            previewContent.innerHTML = '';
        }
        
        // Сбросить чекбоксы фильтров
        const ignoreNewCheckbox = document.getElementById('ignoreNewCommands');
        const ignoreUpdatedCheckbox = document.getElementById('ignoreUpdatedCommands');
        if (ignoreNewCheckbox) ignoreNewCheckbox.checked = false;
        if (ignoreUpdatedCheckbox) ignoreUpdatedCheckbox.checked = false;
        
        // Сбросить чекбоксы категорий
        const categoryCheckboxes = document.querySelectorAll('.ignore-category-item input[type="checkbox"]');
        categoryCheckboxes.forEach(checkbox => {
            checkbox.checked = false;
        });
        
        // Сбросить изменения в трекере
        if (window.ChangeTracker) {
            window.ChangeTracker.resetChanges('config-editor');
        }
        
        // Обновить текст кнопок
        if (sortPreviewButton) {
            sortPreviewButton.textContent = t('sort');
        }
        if (hideIgnoredButton) {
            hideIgnoredButton.textContent = t('hide_ignored');
        }
    }
    
    // Attach event listeners
    processButton.addEventListener('click', processFiles);
    
    if (sortPreviewButton) {
        sortPreviewButton.addEventListener('click', sortPreview);
    }
    
    // Toggle hide ignored categories in preview
    if (hideIgnoredButton) {
        hideIgnoredButton.addEventListener('click', function() {
            hideIgnoredInPreview = !hideIgnoredInPreview;
            scheduleRenderPreview(); // Debounce render for smoother performance
            // Update button text
            hideIgnoredButton.textContent = hideIgnoredInPreview ? t('show_ignored') : t('hide_ignored');
        });
    }
    
    // Создать модальное окно подтверждения очистки
    function createClearConfirmModal() {
        if (document.getElementById('clearConfirmModal')) {
            return;
        }
        
        const modal = document.createElement('div');
        modal.id = 'clearConfirmModal';
        modal.className = 'clear-confirm-modal';
        modal.style.cssText = 'display: none; position: fixed; z-index: 10000; left: 0; top: 0; width: 100%; height: 100%; background-color: rgba(0, 0, 0, 0.8); backdrop-filter: blur(4px); align-items: center; justify-content: center;';
        modal.innerHTML = `
            <div class="clear-confirm-modal-content" style="background-color: rgba(255, 255, 255, 0.1); border: 2px solid var(--border-color); border-radius: var(--border-radius); padding: var(--spacing-lg); max-width: 400px; width: 90%; text-align: center; box-shadow: 0 0 30px rgba(255, 255, 255, 0.2);">
                <h3 style="margin: 0 0 var(--spacing-md) 0; color: var(--text-color); font-size: 1.2rem; font-weight: bold;">${t('confirm_clear')}</h3>
                <div style="display: flex; gap: var(--spacing-md); justify-content: center;">
                    <button class="clear-confirm-btn clear-confirm-yes" style="padding: var(--spacing-sm) var(--spacing-lg); font-size: 1rem; font-weight: bold; border: 2px solid rgba(255, 0, 0, 0.5); border-radius: var(--border-radius); cursor: pointer; transition: var(--transition); min-width: 100px; background-color: rgba(255, 0, 0, 0.2); color: #ff6666;">${getLang() === 'ru' ? 'Да' : 'Yes'}</button>
                    <button class="clear-confirm-btn clear-confirm-no" style="padding: var(--spacing-sm) var(--spacing-lg); font-size: 1rem; font-weight: bold; border: 2px solid var(--border-color); border-radius: var(--border-radius); cursor: pointer; transition: var(--transition); min-width: 100px; background-color: rgba(255, 255, 255, 0.1); color: var(--text-color);">${getLang() === 'ru' ? 'Нет' : 'No'}</button>
                </div>
            </div>
        `;
        document.body.appendChild(modal);
    }
    
    // Показать модальное окно подтверждения очистки
    function showClearConfirm(callback) {
        createClearConfirmModal();
        const modal = document.getElementById('clearConfirmModal');
        if (!modal) {
            if (callback) callback(false);
            return;
        }
        
        modal.style.display = 'flex';
        
        const yesBtn = modal.querySelector('.clear-confirm-yes');
        const noBtn = modal.querySelector('.clear-confirm-no');
        
        if (!yesBtn || !noBtn) {
            if (callback) callback(false);
            return;
        }
        
        // Удалить старые обработчики
        const newYesBtn = yesBtn.cloneNode(true);
        const newNoBtn = noBtn.cloneNode(true);
        yesBtn.parentNode.replaceChild(newYesBtn, yesBtn);
        noBtn.parentNode.replaceChild(newNoBtn, noBtn);
        
        // Обработчики кнопок
        newYesBtn.addEventListener('click', function() {
            modal.style.display = 'none';
            if (callback) callback(true);
        });
        
        newNoBtn.addEventListener('click', function() {
            modal.style.display = 'none';
            if (callback) callback(false);
        });
        
        // Закрытие по клику на фон
        modal.addEventListener('click', function(e) {
            if (e.target === modal) {
                modal.style.display = 'none';
                if (callback) callback(false);
            }
        });
        
        // Закрытие по ESC
        const escHandler = function(e) {
            if (e.key === 'Escape' && modal.style.display === 'flex') {
                e.preventDefault();
                e.stopPropagation();
                modal.style.display = 'none';
                document.removeEventListener('keydown', escHandler);
                if (callback) callback(false);
            }
        };
        document.addEventListener('keydown', escHandler);
    }
    
    // Clear button
    if (clearButton) {
        clearButton.style.display = 'none'; // Скрыть по умолчанию
        clearButton.addEventListener('click', function() {
            showClearConfirm(function(confirmed) {
                if (confirmed) {
                    clearAll();
                }
            });
        });
    }
    
    // Initialize
    updateSourceFileList();
    updateTargetFileList();
    updateProcessButton();
})();

