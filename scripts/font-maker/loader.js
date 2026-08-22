(function(global) {
  'use strict';

  const state = global.FontMakerTool.state;

  async function loadFontFromFile(fileBuffer, fileName) {
    try {
      const font = opentype.parse(fileBuffer);
      state.setCurrentFont(font);
      const currentFont = state.getCurrentFont();

      const fileInfo = document.getElementById('fileInfo');
      if (fileInfo) {
        fileInfo.style.display = 'block';
        const fileNameLabel = state.isRu() ? 'Файл:' : 'File:';
        const fileSizeLabel = state.isRu() ? 'Размер:' : 'Size:';
        const fontNameLabel = state.isRu() ? 'Имя шрифта:' : 'Font name:';
        const unknownLabel = state.isRu() ? 'Неизвестно' : 'Unknown';
        fileInfo.innerHTML =
          '<strong>' + fileNameLabel + '</strong> ' + fileName + '<br>' +
          '<strong>' + fileSizeLabel + '</strong> ' + (fileBuffer.byteLength / 1024).toFixed(2) + ' KB<br>' +
          '<strong>' + fontNameLabel + '</strong> ' + (currentFont.names.fullName ? currentFont.names.fullName.en : unknownLabel) + '<br>' +
          '<strong>Units per EM:</strong> ' + currentFont.unitsPerEm;
      }

      const fontNameInput = document.getElementById('fontName');
      if (fontNameInput) {
        fontNameInput.value = currentFont.names.fullName ? currentFont.names.fullName.en : 'CustomFont';
      }

      const generateBtn = document.getElementById('generateBtn');
      const previewBtn = document.getElementById('previewBtn');
      if (generateBtn) generateBtn.disabled = false;
      if (previewBtn) previewBtn.disabled = false;

      state.showStatus(state.isRu() ? 'Шрифт загружен успешно!' : 'Font loaded successfully!', 'success');
    } catch (error) {
      state.showStatus((state.isRu() ? 'Ошибка загрузки шрифта: ' : 'Error loading font: ') + error.message, 'error');
    }
  }

  async function handleZipFile(zipBuffer) {
    try {
      const zip = await JSZip.loadAsync(zipBuffer);
      const fontFiles = [];

      zip.forEach(function(relativePath, file) {
        const fileName = relativePath.split('/').pop().toLowerCase();
        if (fileName.endsWith('.ttf') || fileName.endsWith('.otf')) {
          fontFiles.push({ path: relativePath, name: fileName });
        }
      });

      if (fontFiles.length === 0) {
        state.showStatus(state.isRu() ? 'В ZIP архиве не найдено TTF/OTF файлов' : 'No TTF/OTF files found in ZIP archive', 'error');
        return;
      }

      let selectedFontFile = null;

      if (fontFiles.length === 1) {
        selectedFontFile = fontFiles[0];
      } else {
        const fontList = fontFiles.map((f, i) => (i + 1) + '. ' + f.name).join('\n');
        const choice = prompt((state.isRu() ? 'Найдено несколько шрифтов. Выберите номер:\n\n' : 'Multiple fonts found. Select number:\n\n') + fontList + '\n\n' + (state.isRu() ? 'Введите номер (1-' : 'Enter number (1-') + fontFiles.length + '):');
        const index = parseInt(choice) - 1;
        if (index >= 0 && index < fontFiles.length) {
          selectedFontFile = fontFiles[index];
        } else {
          state.showStatus(state.isRu() ? 'Неверный выбор' : 'Invalid selection', 'error');
          return;
        }
      }

      if (selectedFontFile) {
        const fontData = await zip.file(selectedFontFile.path).async('arraybuffer');
        await loadFontFromFile(fontData, selectedFontFile.name);

        let copyrightFile = null;
        zip.forEach(function(relativePath, file) {
          const fileName = relativePath.split('/').pop().toLowerCase();
          if (fileName === 'copyright.txt' && !copyrightFile) {
            copyrightFile = file;
          }
        });

        if (copyrightFile) {
          const text = await copyrightFile.async('string');
          state.setCopyrightText(text);
          state.showStatus((state.isRu() ? 'Copyright.txt найден и будет добавлен в CFG' : 'Copyright.txt found and will be added to CFG'), 'info');
        } else {
          state.setCopyrightText('');
        }
      }
    } catch (error) {
      state.showStatus((state.isRu() ? 'Ошибка обработки ZIP: ' : 'Error processing ZIP: ') + error.message, 'error');
    }
  }

  function bindFontFileInput() {
    const input = document.getElementById('ttfFile');
    if (!input) return;

    input.addEventListener('change', async function(e) {
      const file = e.target.files[0];
      if (!file) return;

      state.setCopyrightText('');

      const reader = new FileReader();
      reader.onload = async function(event) {
        try {
          const fileBuffer = event.target.result;
          const fileName = file.name.toLowerCase();

          if (fileName.endsWith('.zip')) {
            if (typeof JSZip === 'undefined') {
              state.showStatus(state.isRu() ? 'JSZip не загружен. Пожалуйста, подождите...' : 'JSZip not loaded. Please wait...', 'info');
              setTimeout(() => {
                reader.onload();
              }, 500);
              return;
            }
            await handleZipFile(fileBuffer);
          } else {
            await loadFontFromFile(fileBuffer, file.name);
          }
        } catch (error) {
          state.showStatus((state.isRu() ? 'Ошибка загрузки файла: ' : 'Error loading file: ') + error.message, 'error');
        }
      };
      reader.readAsArrayBuffer(file);
    });
  }

  global.FontMakerTool = global.FontMakerTool || {};
  global.FontMakerTool.loader = {
    loadFontFromFile,
    handleZipFile,
    bindFontFileInput
  };
})(window);








